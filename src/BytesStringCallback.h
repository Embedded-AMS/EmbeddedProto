/*
 *  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3 of the license.
 *
 *  Embedded Proto  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
 *
 *  For commercial and closed source application please visit:
 *  <https://embeddedproto.com/pricing/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Atoomweg 2
 *    1627 LE, Hoorn
 *    the Netherlands
 */

#ifndef _BYTES_STRING_CALLBACK_H_
#define _BYTES_STRING_CALLBACK_H_

#include "FieldStringBytes.h"
#include "Functional.h"
#include "Errors.h"
#include "WireFormatter.h"

#include <cstdint>
#include <type_traits>


namespace EmbeddedProto
{

  //! A bytes/string field whose payload is streamed to/from the user, not stored.
  /*!
      BytesStringCallback is a drop-in storage type for a singular bytes or string
      field that owns no payload buffer. Instead of holding an array it moves the
      value across the interface seam one element at a time:

        - on serialize, the length prefix is taken from a user *size* callback and
          the payload is pulled element-by-element from a user *source*;
        - on deserialize, every parsed element is pushed to a user *sink*.

      The length prefix comes from size() without pulling any data, so the field
      stays LEN-framed (a bytes/string value carries its size as a number) while
      the payload itself never becomes resident. This lets a device stream a large
      blob (a firmware image, a log, a picture) through a fixed, tiny RAM footprint
      (just the three Functional handles). It plugs in through the callbackStorage
      generator option: it derives from internal::BaseStringBytes and satisfies the
      same static_assert the customStorage mechanism checks.

      Only the whole-value shape is implemented here: the source produces exactly
      size() elements and the sink consumes the whole parsed value. Window/chunk
      streaming (an array_view negotiated per call) is a separate, later capability
      (see doc/callback_storage_chunked_design.md).

      \tparam DATA_TYPE The element type: char for a string field, uint8_t for a
                        bytes field.
  */
  template<class DATA_TYPE>
  class BytesStringCallback : public internal::BaseStringBytes
  {
    static_assert(std::is_same<uint8_t, DATA_TYPE>::value || std::is_same<char, DATA_TYPE>::value,
                  "This class only supports uint8_t or chars.");

    public:

      //! Size callback used during serialize: return the total number of elements the source will produce (the LEN prefix).
      using SizeCallback = Functional<uint32_t()>;

      //! Pull callback used during serialize: fill \p element, return true when a value was produced, false to signal an early end of the stream.
      using SourceCallback = Functional<bool(DATA_TYPE&)>;

      //! Push callback used during deserialize: consume one parsed \p element, return an Error (NO_ERRORS to continue).
      using SinkCallback = Functional<Error(const DATA_TYPE&)>;

      BytesStringCallback() = default;
      ~BytesStringCallback() override = default;

      // --- Binding (reference-taking, no ownership) --------------------------

      //! Bind the size callback that reports the payload length written as the LEN prefix.
      void set_size(const SizeCallback& size) { size_ = size; }

      //! Bind the source used to pull payload elements during serialization.
      void set_source(const SourceCallback& source) { source_ = source; }

      //! Bind the sink used to push payload elements during deserialization.
      void set_sink(const SinkCallback& sink) { sink_ = sink; }

      //! Remove the bound size callback.
      void clear_size() { size_.clear(); }

      //! Remove the bound source.
      void clear_source() { source_.clear(); }

      //! Remove the bound sink.
      void clear_sink() { sink_.clear(); }

      //! Check whether a size callback is bound.
      bool is_size_set() const { return size_.is_set(); }

      //! Check whether a source (pull) callback is bound.
      bool is_source_set() const { return source_.is_set(); }

      //! Check whether a sink (push) callback is bound.
      bool is_sink_set() const { return sink_.is_set(); }

      //! Require a binding for the direction being used.
      /*!
        When strict, streaming without the relevant callback bound returns
        CALLBACK_NOT_SET instead of silently discarding (deserialize) or
        emitting nothing (serialize).
      */
      void set_strict(bool strict) { strict_ = strict; }

      //! Whether strict (require-binding) mode is enabled.
      bool is_strict() const { return strict_; }

      //! Copy the bindings from another callback field (used by the generated message copy/assignment).
      /*!
        A callback field owns no payload to copy, so copying it copies the size,
        source and sink handles: the copy streams through the same user callbacks.

        \return Always NO_ERRORS, for signature compatibility with FieldStringBytes::set().
      */
      Error set(const BytesStringCallback<DATA_TYPE>& rhs)
      {
        size_ = rhs.size_;
        source_ = rhs.source_;
        sink_ = rhs.sink_;
        strict_ = rhs.strict_;
        return Error::NO_ERRORS;
      }

      // --- FieldStringBytes-shaped interface ---------------------------------

      //! The payload length written as the LEN prefix, taken from the size callback (0 when unbound).
      uint32_t get_length() const
      {
        uint32_t length = 0U;
        static_cast<void>(size_.invoke(length));
        return length;
      }

      //! Streaming is effectively unbounded; report a large sentinel so capacity checks never fire.
      uint32_t get_max_length() const { return UINT32_MAX; }

      //! Required of a customStorage bytes/string type.
      /*!
        A stream has no finite serialized size, so report the unbounded sentinel; a
        message with a callback field therefore cannot be statically buffer-sized,
        which is inherent to streaming.
      */
      static constexpr uint32_t max_serialized_size(const uint32_t field_number)
      {
        static_cast<void>(field_number);
        return UINT32_MAX;
      }

      //! Pull the payload from the bound source and write it (no tag, no size; the caller frames it with serialize_len).
      /*!
          Exactly get_length() (the size callback's value) elements are pulled and
          pushed. If the source ends early the declared prefix can no longer be
          honoured, so CALLBACK_SIZE_MISMATCH is returned. A non-empty value with no
          source bound cannot be produced at all: CALLBACK_NOT_SET.
      */
      Error serialize(WriteBufferInterface& buffer) const override
      {
        Error return_value = Error::NO_ERRORS;
        const uint32_t total = get_length();
        if(0U == total)
        {
          // Nothing declared to emit; the caller wrote an empty (or no) prefix.
        }
        else if(!source_.is_set())
        {
          return_value = Error::CALLBACK_NOT_SET;
        }
        else
        {
          uint32_t written = 0U;
          while((written < total) && (Error::NO_ERRORS == return_value))
          {
            DATA_TYPE element = DATA_TYPE();
            bool produced = false;
            static_cast<void>(source_.invoke(produced, element));
            if(!produced)
            {
              return_value = Error::CALLBACK_SIZE_MISMATCH;
            }
            else if(!buffer.push(static_cast<uint8_t>(element)))
            {
              return_value = Error::BUFFER_FULL;
            }
            else
            {
              ++written;
            }
          }
        }
        return return_value;
      }

      //! Read the LEN prefix, then push each parsed element to the bound sink (drain-and-discard when unbound and not strict).
      Error deserialize(ReadBufferInterface& buffer) override
      {
        uint32_t available = 0U;
        Error return_value = WireFormatter::DeserializeVarint(buffer, available);
        if(Error::NO_ERRORS == return_value)
        {
          uint32_t received = 0U;
          bool more = true;
          while(more && (received < available) && (Error::NO_ERRORS == return_value))
          {
            uint8_t byte = 0U;
            if(!buffer.pop(byte))
            {
              return_value = Error::END_OF_BUFFER;
              more = false;
            }
            else
            {
              return_value = push_to_sink(static_cast<DATA_TYPE>(byte));
              if(Error::NO_ERRORS == return_value)
              {
                ++received;
              }
            }
          }
        }
        return return_value;
      }

      //! Validate the wire type is length-delimited, then deserialize.
      Error deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer,
                                   const ::EmbeddedProto::WireFormatter::WireType& wire_type) override
      {
        Error return_value = ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED == wire_type
                             ? Error::NO_ERRORS : Error::INVALID_WIRETYPE;
        if(Error::NO_ERRORS == return_value)
        {
          return_value = this->deserialize(buffer);
        }
        return return_value;
      }

#ifdef PARTIAL_SERIALIZATION_ENABLED
      //! Resumable pull-serialize for the partial engine.
      /*!
          The TAG and SIZE phases write the tag and the size prefix from
          get_length() (no data pulled), so the field survives a write buffer that
          splits before the payload. The DATA phase pulls one element at a time,
          only ever pulling when the buffer has room, so a not-yet-written element
          is never lost on a BUFFER_FULL resume. The source's own cursor is the
          resumable state; no in-flight element is stored in the field.
      */
      Error serialize_partial_as_field(uint32_t field_number,
                                       WriteBufferInterface& buffer,
                                       MessageState& state,
                                       bool optional) const override
      {
        Error return_value = Error::NO_ERRORS;
        if(::EmbeddedProto::FieldProcessingPhase::DATA != state.phase)
        {
          return_value = serialize_partial_tag_and_size(field_number, get_length(), buffer, state, optional);
        }

        if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
        {
          if(!source_.is_set())
          {
            return_value = Error::CALLBACK_NOT_SET;
          }
          else
          {
            bool more = true;
            while(more && (Error::NO_ERRORS == return_value))
            {
              if(0U == state.bytes_remaining)
              {
                state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
                more = false;
              }
              else if(0U == buffer.get_available_size())
              {
                return_value = Error::BUFFER_FULL;
              }
              else
              {
                DATA_TYPE element = DATA_TYPE();
                bool produced = false;
                static_cast<void>(source_.invoke(produced, element));
                if(!produced)
                {
                  return_value = Error::CALLBACK_SIZE_MISMATCH;
                }
                else if(!buffer.push(static_cast<uint8_t>(element)))
                {
                  return_value = Error::BUFFER_FULL;
                }
                else
                {
                  --state.bytes_remaining;
                }
              }
            }
          }
        }
        return return_value;
      }

      //! Resumable push-deserialize for the partial engine.
      /*!
          The SIZE phase reads the LEN prefix into state.bytes_remaining; the DATA
          phase pops elements and pushes them to the sink, decrementing
          bytes_remaining, and returns END_OF_BUFFER (leaving the phase at DATA) when
          the read buffer drains mid-value so the next call resumes where it stopped.
      */
      Error deserialize_partial_as_field(ReadBufferInterface& buffer,
                                         MessageState& state) override
      {
        Error return_value = Error::NO_ERRORS;
        if((::EmbeddedProto::FieldProcessingPhase::SIZE != state.phase) && (::EmbeddedProto::FieldProcessingPhase::DATA != state.phase))
        {
          return_value = Error::STATE_MISMATCH;
        }

        if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase))
        {
          return_value = deserialize_partial_size_phase(buffer, state);
        }

        if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
        {
          bool more = true;
          while(more && (Error::NO_ERRORS == return_value))
          {
            if(0U == state.bytes_remaining)
            {
              state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
              more = false;
            }
            else
            {
              uint8_t byte = 0U;
              if(!buffer.pop(byte))
              {
                return_value = Error::END_OF_BUFFER;
                more = false;
              }
              else
              {
                return_value = push_to_sink(static_cast<DATA_TYPE>(byte));
                if(Error::NO_ERRORS == return_value)
                {
                  --state.bytes_remaining;
                }
              }
            }
          }
        }
        return return_value;
      }
#endif // PARTIAL_SERIALIZATION_ENABLED

      //! No payload is resident, so there is nothing to reset; bindings are kept.
      void clear() override {}

#ifdef MSG_TO_STRING
      //! A callback field holds no resident value to print, so leave the string unchanged.
      ::EmbeddedProto::string_view to_string(::EmbeddedProto::string_view& str, const uint32_t indent_level, char const* name, const bool first_field) const override
      {
        static_cast<void>(indent_level);
        static_cast<void>(name);
        static_cast<void>(first_field);
        return str;
      }
#endif // MSG_TO_STRING

    private:

      //! Push one parsed element to the sink, or drain-and-discard when no sink is bound (CALLBACK_NOT_SET in strict mode).
      Error push_to_sink(const DATA_TYPE& element)
      {
        Error return_value = Error::NO_ERRORS;
        if(sink_.is_set())
        {
          Error sink_result = Error::NO_ERRORS;
          static_cast<void>(sink_.invoke(sink_result, element));
          return_value = sink_result;
        }
        else if(strict_)
        {
          return_value = Error::CALLBACK_NOT_SET;
        }
        else
        {
          // No sink bound and not strict: drain and discard.
          static_cast<void>(element);
        }
        return return_value;
      }

      //! When true, streaming without a bound callback is an error rather than a silent drain/no-op.
      bool strict_ = false;

      //! Size callback invoked to report the payload length written as the LEN prefix.
      SizeCallback size_{};

      //! Pull callback invoked to produce payload elements while serializing.
      SourceCallback source_{};

      //! Push callback invoked to consume payload elements while deserializing.
      SinkCallback sink_{};
  };

} // End of namespace EmbeddedProto

#endif // End of _BYTES_STRING_CALLBACK_H_
