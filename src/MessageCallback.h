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

#ifndef _MESSAGE_CALLBACK_H_
#define _MESSAGE_CALLBACK_H_

#include "MessageInterface.h"
#include "Functional.h"
#include "Errors.h"
#include "WireFormatter.h"
#include "MessageState.h"

#include <cstdint>
#include <type_traits>


namespace EmbeddedProto
{

  //! A repeated message field whose elements are streamed to/from the user, not stored.
  /*!
      MessageCallback is a drop-in storage type for a repeated message field that
      owns no element array. Instead of holding N sub-messages it moves one across
      the interface seam at a time:

        - on serialize, each element is pulled from a user *source*, filled into a
          single transient sub-message and emitted;
        - on deserialize, every parsed element lands in the transient and is pushed
          to a user *sink*.

      Elements are framed DELIMITED (editions message_encoding = DELIMITED): each is
      a START_GROUP / END_GROUP pair rather than a length-prefixed block. Group
      framing carries no size prefix, so a message can be written in a single pass
      without the serialized_size() pre-pass a LEN frame needs (which for a stream
      would drain the source twice). This lets a device relay arbitrarily many
      sub-messages through a fixed, tiny RAM footprint: the two Functional handles
      plus one transient sub-message (never the whole collection). It plugs in
      through the callbackStorage generator option, which also forces the field to
      DELIMITED encoding.

      Only repeated message fields are supported here; a singular message callback
      is a separate, later capability.

      \tparam MSG_TYPE The generated message type streamed through this field (a
                       MessageInterface).
  */
  template<class MSG_TYPE>
  class MessageCallback : public MessageInterface
  {
    static_assert(std::is_base_of<::EmbeddedProto::MessageInterface, MSG_TYPE>::value,
                  "MessageCallback streams a generated message type (a MessageInterface).");

    public:

      //! Pull callback used during serialize: fill \p element, return true when a value was produced, false to signal the end of the stream.
      using SourceCallback = Functional<bool(MSG_TYPE&)>;

      //! Push callback used during deserialize: consume one parsed \p element, return an Error (NO_ERRORS to continue).
      using SinkCallback = Functional<Error(const MSG_TYPE&)>;

      MessageCallback() = default;
      ~MessageCallback() override = default;

      // --- Binding (reference-taking, no ownership) --------------------------

      //! Bind the source used to pull elements during serialization.
      void set_source(const SourceCallback& source) { source_ = source; }

      //! Bind the sink used to push elements during deserialization.
      void set_sink(const SinkCallback& sink) { sink_ = sink; }

      //! Remove the bound source.
      void clear_source() { source_.clear(); }

      //! Remove the bound sink.
      void clear_sink() { sink_.clear(); }

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
        A callback field owns no elements to copy, so copying it copies the source
        and sink handles: the copy streams through the same user callbacks.

        \return Always NO_ERRORS, for signature compatibility with a resident field's set().
      */
      Error set(const MessageCallback<MSG_TYPE>& rhs)
      {
        source_ = rhs.source_;
        sink_ = rhs.sink_;
        strict_ = rhs.strict_;
        return Error::NO_ERRORS;
      }

      // --- Repeated-field-shaped interface -----------------------------------

      //! Number of elements streamed so far (a running counter, not a capacity). Presence is get_length() > 0.
      uint32_t get_length() const { return length_; }

      //! Required of a callbackStorage message type.
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

      // --- MessageInterface / Field interface --------------------------------

      //! Guard the LEN / size-pass serialization of a streaming field.
      /*!
          A callback message owns no element to write here and is emitted EXPANDED
          as groups only (serialize_expanded). This method is what a size pass
          reaches via Field::serialized_size(), and what serialize_as_group would
          call. Any real call means the field was placed where a length-delimited
          (LEN) framing is required (e.g. under a LEN ancestor), which is
          unsupported for streaming storage: report CALLBACK_SEQUENCE instead of
          draining the source.
      */
      Error serialize(WriteBufferInterface& buffer) const override
      {
        static_cast<void>(buffer);
        return Error::CALLBACK_SEQUENCE;
      }

      //! Pull elements from the bound source and emit each as a DELIMITED group.
      /*!
          One START_GROUP / body / END_GROUP is written per element (the same wire
          form as an EXPANDED repeated message field), so no field-level size prefix
          is needed and the source is drained exactly once in a single pass.
          Production ends when the source returns false. With no source bound
          nothing is emitted (NO_ERRORS), unless strict mode is on, in which case
          CALLBACK_NOT_SET is returned.

          \param field_number The field number written in each element's group tags.
          \param buffer        The destination buffer.
      */
      Error serialize_expanded(uint32_t field_number, WriteBufferInterface& buffer) const
      {
        Error return_value = Error::NO_ERRORS;
        if(!source_.is_set())
        {
          if(strict_)
          {
            return_value = Error::CALLBACK_NOT_SET;
          }
          // Not strict: no source bound, emit nothing.
        }
        else
        {
          bool more = true;
          while(more && (Error::NO_ERRORS == return_value))
          {
            bool produced = false;
            transient_.clear();
            static_cast<void>(source_.invoke(produced, transient_));
            if(produced)
            {
              return_value = transient_.serialize_as_group(field_number, buffer);
              if(Error::NO_ERRORS == return_value)
              {
                ++length_;
              }
            }
            else
            {
              more = false;
            }
          }
        }
        return return_value;
      }

      //! Deserialize one group element and push it to the sink.
      /*!
          The opening START_GROUP tag has been consumed by the caller. The element's
          body is read into the transient (which stops at its matching END_GROUP)
          and then pushed to the bound sink (drain-and-discard when unbound and not
          strict). Called once per element.
      */
      Error deserialize(ReadBufferInterface& buffer) override
      {
        transient_.clear();
        Error return_value = transient_.deserialize(buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = push_to_sink(transient_);
          if(Error::NO_ERRORS == return_value)
          {
            ++length_;
          }
        }
        return return_value;
      }

#ifdef PARTIAL_SERIALIZATION_ENABLED
      //! Guard the whole-message partial serialize path.
      /*!
          A callback message is emitted EXPANDED as groups only (via
          serialize_partial_as_field_expanded, which frames each element and streams
          its body through the transient's own serialize_partial). This entry point
          would LEN/size-pass the field, which a stream cannot provide: report
          CALLBACK_SEQUENCE.
      */
      Error serialize_partial(WriteBufferInterface& buffer, MessageState& state) const override
      {
        static_cast<void>(buffer);
        static_cast<void>(state);
        return Error::CALLBACK_SEQUENCE;
      }

      //! Resumable EXPANDED pull-serialize for the partial engine.
      /*!
          Emits one START_GROUP / body / END_GROUP per element, pulling from the
          source, and survives a write buffer that fills mid-element. The group
          framing is inlined at this field's state level (state.phase drives
          TAG -> DATA -> SIZE(reused for END_GROUP) -> next element; state.child
          streams the element body), so a callback message field consumes no more
          state-stack depth than a resident one. An element is pulled only when a
          new group is started (guarded by element_loaded_), so a BUFFER_FULL resume
          never double-pulls the source.
      */
      Error serialize_partial_as_field_expanded(uint32_t field_number,
                                                WriteBufferInterface& buffer,
                                                MessageState& state,
                                                bool optional) const
      {
        static_cast<void>(optional);
        Error return_value = Error::NO_ERRORS;
        bool more = true;
        while(more && (Error::NO_ERRORS == return_value))
        {
          if(::EmbeddedProto::FieldProcessingPhase::COMPLETE == state.phase)
          {
            // The whole stream has been drained.
            more = false;
          }
          else if(::EmbeddedProto::FieldProcessingPhase::TAG == state.phase)
          {
            return_value = serialize_partial_begin_group(field_number, buffer, state);
          }
          else if(::EmbeddedProto::FieldProcessingPhase::DATA == state.phase)
          {
            if(nullptr != state.child)
            {
              return_value = transient_.serialize_partial(buffer, *state.child);
              if((Error::NO_ERRORS == return_value)
                 && (::EmbeddedProto::FieldProcessingPhase::COMPLETE == state.child->phase))
              {
                // Body complete; the END_GROUP tag still has to be written. Reuse
                // the (otherwise unused) SIZE phase to mark "END_GROUP pending".
                state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
              }
            }
            else
            {
              return_value = Error::NESTING_TOO_DEEP;
            }
          }
          else
          {
            // Phase SIZE (reused): write the END_GROUP tag, then advance to the
            // next element.
            const uint32_t tag = WireFormatter::MakeTag(field_number, WireFormatter::WireType::END_GROUP);
            if(buffer.get_available_size() >= WireFormatter::VarintSize(tag))
            {
              return_value = WireFormatter::SerializeVarint(tag, buffer);
              if(Error::NO_ERRORS == return_value)
              {
                ++length_;
                element_loaded_ = false;
                state.phase = ::EmbeddedProto::FieldProcessingPhase::TAG;
                if(nullptr != state.child)
                {
                  state.child->reset();
                }
              }
            }
            else
            {
              return_value = Error::BUFFER_FULL;
            }
          }
        }
        return return_value;
      }

      //! Resumable push-deserialize of one group element for the partial engine.
      /*!
          Invoked by the inherited deserialize_partial_as_group, which consumes the
          START_GROUP tag and streams the element body here. The body is read into
          the transient; when it consumes its matching END_GROUP the transient's
          state reports COMPLETE, at which point the element is pushed to the sink
          and the transient is reset for the next element. Returns END_OF_BUFFER
          (leaving the state mid-body) when the read buffer drains, so the next call
          resumes where it stopped.
      */
      Error deserialize_partial(ReadBufferInterface& buffer, MessageState& state) override
      {
        Error return_value = transient_.deserialize_partial(buffer, state);
        if((Error::NO_ERRORS == return_value)
           && (::EmbeddedProto::FieldProcessingPhase::COMPLETE == state.phase))
        {
          return_value = push_to_sink(transient_);
          if(Error::NO_ERRORS == return_value)
          {
            ++length_;
          }
          transient_.clear();
        }
        return return_value;
      }
#endif // PARTIAL_SERIALIZATION_ENABLED

      //! Reset the streaming state (the element counter). Bindings are kept.
      void clear() override { length_ = 0U; }

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
      Error push_to_sink(const MSG_TYPE& element)
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

#ifdef PARTIAL_SERIALIZATION_ENABLED
      //! Pull the next element (once, guarded by element_loaded_) and write its START_GROUP tag.
      /*!
          When the source is exhausted (or unbound and not strict) the field is
          marked COMPLETE. Otherwise the pulled element stays resident in the
          transient across a BUFFER_FULL resume so it is never pulled twice.
      */
      Error serialize_partial_begin_group(uint32_t field_number,
                                          WriteBufferInterface& buffer,
                                          MessageState& state) const
      {
        Error return_value = Error::NO_ERRORS;
        if(!element_loaded_)
        {
          if(!source_.is_set())
          {
            if(strict_)
            {
              return_value = Error::CALLBACK_NOT_SET;
            }
            else
            {
              state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
            }
          }
          else
          {
            bool produced = false;
            transient_.clear();
            static_cast<void>(source_.invoke(produced, transient_));
            if(produced)
            {
              element_loaded_ = true;
              if(nullptr != state.child)
              {
                state.child->reset();
              }
            }
            else
            {
              state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
            }
          }
        }

        if((Error::NO_ERRORS == return_value) && element_loaded_)
        {
          const uint32_t tag = WireFormatter::MakeTag(field_number, WireFormatter::WireType::START_GROUP);
          if(buffer.get_available_size() >= WireFormatter::VarintSize(tag))
          {
            return_value = WireFormatter::SerializeVarint(tag, buffer);
            if(Error::NO_ERRORS == return_value)
            {
              state.phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
            }
          }
          else
          {
            return_value = Error::BUFFER_FULL;
          }
        }
        return return_value;
      }
#endif // PARTIAL_SERIALIZATION_ENABLED

      //! Running count of elements streamed through this field. Mutable because serialize is const yet advances the counter as elements are pulled.
      mutable uint32_t length_ = 0U;

      //! Set while a pulled element awaits its group being written, so a BUFFER_FULL resume does not pull it again.
      mutable bool element_loaded_ = false;

      //! When true, streaming without a bound callback is an error rather than a silent drain/no-op.
      bool strict_ = false;

      //! Single sub-message landing slot required to assemble/parse one element. It is not stream storage: the collection owns no per-element buffer.
      mutable MSG_TYPE transient_ = {};

      //! Pull callback invoked to produce elements while serializing.
      SourceCallback source_{};

      //! Push callback invoked to consume elements while deserializing.
      SinkCallback sink_{};
  };

} // End of namespace EmbeddedProto

#endif // End of _MESSAGE_CALLBACK_H_
