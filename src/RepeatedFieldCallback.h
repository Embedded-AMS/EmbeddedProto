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

#ifndef _REPEATED_FIELD_CALLBACK_H_
#define _REPEATED_FIELD_CALLBACK_H_

#include "RepeatedField.h"
#include "Functional.h"
#include "Errors.h"

#include <cstdint>


namespace EmbeddedProto
{

  //! A repeated field whose elements are streamed to/from the user, not stored.
  /*!
      RepeatedFieldCallback is a drop-in storage type for a repeated scalar field
      that owns no element buffer. Instead of holding an array it forwards each
      element across the interface seam:

        - on deserialize, every parsed element is pushed to a user *sink*;
        - on serialize, elements are pulled one by one from a user *source*.

      This lets a device stream arbitrarily many elements through a fixed, tiny
      RAM footprint (just the two Functional handles plus a single transient
      return slot required by the RepeatedField interface). It plugs in through
      the existing customStorage mechanism: it derives from RepeatedField and
      satisfies the same static_assert.

      This skeleton implements the RepeatedField interface minimally; the
      streaming behaviour is wired up in the following steps of the callback
      storage plan (sink on deserialize, source on serialize, then partial /
      resumable support).

      \tparam DATA_TYPE The element field type (a FieldTemplate scalar such as
                        EmbeddedProto::int32).
  */
  template<class DATA_TYPE>
  class RepeatedFieldCallback : public RepeatedField<DATA_TYPE>
  {
    public:

      //! Pull callback used during serialize: fill \p element, return true when a
      //! value was produced, false to signal the end of the stream.
      using SourceCallback = Functional<bool(DATA_TYPE&)>;

      //! Push callback used during deserialize: consume one parsed \p element,
      //! return an Error (NO_ERRORS to continue).
      using SinkCallback = Functional<Error(const DATA_TYPE&)>;

      RepeatedFieldCallback() = default;
      ~RepeatedFieldCallback() override = default;

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

      //! Require a binding for the direction being used. When strict, streaming
      //! without the relevant callback bound returns CALLBACK_NOT_SET instead of
      //! silently discarding (deserialize) or emitting nothing (serialize).
      void set_strict(bool strict) { strict_ = strict; }

      //! Whether strict (require-binding) mode is enabled.
      bool is_strict() const { return strict_; }

      // --- RepeatedField interface -------------------------------------------

      //! Number of elements streamed so far (a running counter, not a capacity).
      uint32_t get_length() const override { return length_; }

      //! Streaming is effectively unbounded; report a large sentinel so the
      //! engine's "array full" checks never fire.
      uint32_t get_max_length() const override { return UINT32_MAX; }

      //! No elements are stored, so no bytes are resident.
      uint32_t get_size() const override { return 0U; }

      //! Streaming is effectively unbounded.
      uint32_t get_max_size() const override { return UINT32_MAX; }

      //! Random access is unsupported; return the transient slot as a landing
      //! area for the engine (its contents are not retained).
      DATA_TYPE& get(uint32_t index) override
      {
        static_cast<void>(index);
        return transient_;
      }

      //! Random access is unsupported; return the transient slot.
      const DATA_TYPE& get_const(uint32_t index) const override
      {
        static_cast<void>(index);
        return transient_;
      }

      //! Random access is unsupported.
      Error get_const(const uint32_t index, DATA_TYPE& value) const override
      {
        static_cast<void>(index);
        static_cast<void>(value);
        return Error::INDEX_OUT_OF_BOUND;
      }

      //! Random access is unsupported; setting a specific index is a no-op.
      void set(uint32_t index, const DATA_TYPE& value) override
      {
        static_cast<void>(index);
        static_cast<void>(value);
      }

      //! Bulk assignment is unsupported for a streaming field.
      Error set_data(const DATA_TYPE* data, const uint32_t length) override
      {
        static_cast<void>(data);
        static_cast<void>(length);
        return Error::ARRAY_FULL;
      }

      //! Append one element: push it to the bound sink instead of storing it.
      /*!
          Every parsed element funnels through here (the base packed-decode loop
          calls the virtual add(), and the expanded path is routed here by the
          deserialize_check_type override below). With a sink bound, the element
          is forwarded and the sink's Error is propagated. With no sink bound the
          element is drained and discarded (NO_ERRORS), unless strict mode is on,
          in which case CALLBACK_NOT_SET is returned. The element counter is
          advanced only for an element that was accepted.
      */
      Error add(const DATA_TYPE& value) override
      {
        Error return_value = Error::NO_ERRORS;
        if(sink_.is_set())
        {
          Error sink_result = Error::NO_ERRORS;
          static_cast<void>(sink_.invoke(sink_result, value));
          return_value = sink_result;
        }
        else if(strict_)
        {
          return_value = Error::CALLBACK_NOT_SET;
        }
        else
        {
          // No sink bound and not strict: drain and discard.
          static_cast<void>(value);
        }

        if(Error::NO_ERRORS == return_value)
        {
          ++length_;
        }
        return return_value;
      }

      //! Route deserialization through add() for both packed and expanded input.
      /*!
          Length-delimited (packed) input is handled by the base deserialize(),
          whose element loop already calls the virtual add(). Expanded input
          (one tag per element) would otherwise be routed to the random-access
          get(index) slot this field lacks, so it is deserialized into a stack
          local here and pushed through add() as well.
      */
      Error deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer,
                                   const ::EmbeddedProto::WireFormatter::WireType& wire_type) override
      {
        Error return_value = Error::NO_ERRORS;
        const bool is_length_delimited =
            ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED == wire_type;
        if(is_length_delimited)
        {
          return_value = this->deserialize(buffer);
        }
        else
        {
          DATA_TYPE element;
          return_value = element.deserialize_check_type(buffer, wire_type);
          if(Error::NO_ERRORS == return_value)
          {
            return_value = this->add(element);
          }
        }
        return return_value;
      }

      //! Reset the streaming state (the element counter). Bindings are kept.
      void clear() override { length_ = 0U; }

    private:

      //! Running count of elements streamed through this field.
      uint32_t length_ = 0U;

      //! When true, streaming without a bound callback is an error rather than a
      //! silent drain/no-op.
      bool strict_ = false;

      //! Single-element landing slot required by the reference-returning
      //! RepeatedField interface. It is not stream storage: the collection owns
      //! no per-element buffer.
      mutable DATA_TYPE transient_ = {};

      //! Pull callback invoked to produce elements while serializing.
      SourceCallback source_{};

      //! Push callback invoked to consume elements while deserializing.
      SinkCallback sink_{};
  };

} // End of namespace EmbeddedProto

#endif // End of _REPEATED_FIELD_CALLBACK_H_
