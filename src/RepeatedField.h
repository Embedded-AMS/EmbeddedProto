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

#ifndef _REPEATED_FIELD_H_
#define _REPEATED_FIELD_H_

#include "Defines.h"
#include "Fields.h"
#include "MessageInterface.h"
#include "MessageSizeCalculator.h"
#include "ReadBufferSection.h"
#include "FieldStringBytes.h"
#include "Errors.h"

#include <cstdint>
#include <type_traits>


namespace EmbeddedProto
{

  namespace internal
  {
    //! Trait describing whether DATA_TYPE is a packed fixed-width scalar field.
    /*!
        Fixed-width scalar fields (fixed32/sfixed32/float and
        fixed64/sfixed64/double) use the FIXED32 or FIXED64 wire type and store
        their value in little-endian layout on a little-endian target. The trait
        exposes both the detection flag and the raw scalar type so the packed
        serialization paths can batch these values into whole-block / per-element
        buffer writes. For any other type the primary template reports false.
    */
    template<typename T>
    struct PackedFixedTraits
    {
      static constexpr bool is_fixed_width = false;
      using scalar_type = uint8_t;
    };

    template<Field::FieldTypes F, typename V, WireFormatter::WireType W, uint32_t S>
    struct PackedFixedTraits<::EmbeddedProto::FieldTemplate<F, V, W, S>>
    {
      static constexpr bool is_fixed_width =
          (::EmbeddedProto::WireFormatter::WireType::FIXED32 == W)
          || (::EmbeddedProto::WireFormatter::WireType::FIXED64 == W);
      using scalar_type = V;
    };
  } // namespace internal

  //! Class template that specifies the interface of an arry with the data type.
  template<class DATA_TYPE>
  class RepeatedField : public Field
  {

    //! Definition of a trait to check if DATA_TYPE is NOT a specialization of the FieldTemplate.
    template<typename>
    struct is_specialization_of_FieldTemplate : std::false_type {};

    //! Definition of a trait to check if DATA_TYPE is a specialization of the FieldTemplate.
    template<Field::FieldTypes F, typename V, WireFormatter::WireType W, uint32_t S>
    struct is_specialization_of_FieldTemplate<::EmbeddedProto::FieldTemplate<F,V,W,S>> : std::true_type {};

    //! Helper trait to extract wire type from a FieldTemplate specialization.
    template<typename>
    struct fieldtemplate_wire_type;

    template<Field::FieldTypes F, typename V, WireFormatter::WireType W, uint32_t S>
    struct fieldtemplate_wire_type<::EmbeddedProto::FieldTemplate<F, V, W, S>>
    {
      static constexpr ::EmbeddedProto::WireFormatter::WireType value = W;
    };

    //! This class only supports Field and FieldTemplate classes as template parameter.
    static_assert(std::is_base_of<::EmbeddedProto::Field, DATA_TYPE>::value || is_specialization_of_FieldTemplate<DATA_TYPE>::value, 
                  "A Field can only be used as template paramter.");

    public:

      //! Check how this field shoeld be serialized, packed or not.
      static constexpr bool REPEATED_FIELD_IS_PACKED = 
            !(std::is_base_of<MessageInterface, DATA_TYPE>::value
              || std::is_base_of<internal::BaseStringBytes, DATA_TYPE>::value);


      RepeatedField() = default;
      ~RepeatedField() override = default;

      //! Obtain the total number of DATA_TYPE items in the array.
      virtual uint32_t get_length() const = 0;

      //! Obtain the maximum number of DATA_TYPE items which can at most be stored in the array.
      virtual uint32_t get_max_length() const = 0;
      
      //! Obtain the total number of bytes currently stored in the array.
      virtual uint32_t get_size() const = 0;

      //! Obtain the maximum number of bytes which can at most be stored in the array.
      virtual uint32_t get_max_size() const = 0;

      //! Get a reference to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The reference to the value at the given index.
      */
      virtual DATA_TYPE& get(uint32_t index) = 0;

      //! Get a constant reference to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The constant reference to the value at the given index.
      */
      virtual const DATA_TYPE& get_const(uint32_t index) const = 0;

      //! Get a constant reference to the value at the given index.
      /*!
        \param[in] index The desired index to return.
        \param[out] value The value of the desired index is set in this reference.
        \return An error incase of an index out of bound situation.
      */
      virtual Error get_const(const uint32_t index, DATA_TYPE& value) const = 0;

      //! Get a reference to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The reference to the value at the given index.
      */
      DATA_TYPE& operator[](uint32_t index) { return this->get(index); }

      //! Get a reference to the value at the given index. But constant. 
      /*!
        \param[in] index The desired index to return.
        \return The constant reference to the value at the given index.
      */
      const DATA_TYPE& operator[](uint32_t index) const { return this->get_const(index); }

      //! Set the value at the given index.
      /*!
        \param[in] index The desired index to change.
        \param[in] value The value we would like to set.
      */
      virtual void set(uint32_t index, const DATA_TYPE& value) = 0;

      //! Given a different array of known length copy that data into this object.
      /*!
        \param[in] data A pointer the array to copy from.
        \param[in] length The number of value of DATA_TYPE in the array.
        \return Error::NO_ERRORS when every was successful. Error::ARRAY_FULL when there is no space left.
      */
      virtual Error set_data(const DATA_TYPE* data, const uint32_t length) = 0;

      //! Append a value to the end of the array.
      /*!
        \param[in] value The data to add.
        \return Error::NO_ERRORS when every was successful. Error::ARRAY_FULL when there is no space left.
      */
      virtual Error add(const DATA_TYPE& value) = 0;

      //! Remove the element at the given index, moving the elements behind it one place forward.
      /*!
          The default implementation reports INDEX_OUT_OF_BOUND. Storage types which hold no
          resident collection, like the streaming callback storage, keep that behaviour; storage
          types which can shrink override this function. Used among others to remove a key from a
          map field.

          \param[in] index The index of the element to remove.
          \return Error::NO_ERRORS when the element was removed. Error::INDEX_OUT_OF_BOUND when the
                  index is outside of the data held or this storage type can not remove elements.
      */
      virtual Error erase(const uint32_t index)
      {
        static_cast<void>(index);
        return Error::INDEX_OUT_OF_BOUND;
      }

      //! Remove all data in the array and set it to the default value.
      virtual void clear() override = 0;

      //! Serialize all elements in the array (used for packed serialization).
      /*!
          Not final: RepeatedFieldFixedSize overrides this to batch packed
          fixed-width payloads into a single whole-block buffer write.
      */
      Error serialize(WriteBufferInterface& buffer) const override
      {
        Error return_value = Error::NO_ERRORS;
        for(uint32_t i = 0; (i < this->get_length()) && (Error::NO_ERRORS == return_value); ++i)
        {
          return_value = this->get_const(i).serialize(buffer);
        }
        return return_value;
      }

      //! Function to deserialize this array.
      /*!
          From a buffer of data fill this array with data.
          Not final: RepeatedFieldFixedSize overrides this to read a packed
          fixed-width payload out of the buffer in a single batched block read
          instead of one virtual call per byte.
          \param buffer [in]  The memory from which the message is obtained.
          \return Error::NO_ERRORS when every was successful.
      */
      Error deserialize(::EmbeddedProto::ReadBufferInterface& buffer) override
      {
        Error return_value = Error::NO_ERRORS;
        if(REPEATED_FIELD_IS_PACKED)
        {              
          return_value = deserialize_packed(buffer);
        }
        else 
        {
          return_value = deserialize_unpacked(buffer);
        }
        return return_value;
      }

      //! Not final: RepeatedFieldCallback overrides this so expanded
      //! (one-tag-per-element) scalar input is funnelled through the virtual
      //! add() hook instead of the random-access get(index) slot it lacks.
      Error deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer,
                                   const ::EmbeddedProto::WireFormatter::WireType& wire_type) override
      {
        const bool is_length_delimited =
            ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED == wire_type;
        Error return_value = Error::NO_ERRORS;

        if(REPEATED_FIELD_IS_PACKED)
        {
          // Scalar / enum elements may be received either as a single packed block
          // (length-delimited) or as expanded one-tag-per-element values (the
          // element's own wire type). Both forms are accepted regardless of which
          // form this side emits (repeated_field_encoding feature).
          if(is_length_delimited)
          {
            // Route through the virtual deserialize() so RepeatedFieldFixedSize's
            // whole-block fast path is reached on the generated receive path.
            return_value = this->deserialize(buffer);
          }
          else
          {
            return_value = deserialize_unpacked(buffer);
          }
        }
        else
        {
          // Message / string / bytes elements are always length-delimited, one per
          // tag.
          return_value = is_length_delimited ? deserialize_unpacked(buffer)
                                             : Error::INVALID_WIRETYPE;
        }

        return return_value;
      }

#ifdef PARTIAL_SERIALIZATION_ENABLED
      Error deserialize_partial_as_field(ReadBufferInterface& buffer,
                                         MessageState& state) override
      {
        Error return_value = Error::NO_ERRORS;

        if(REPEATED_FIELD_IS_PACKED)
        {
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
            ReadBufferSection section(buffer, state.bytes_remaining);
            const uint32_t section_size_before = section.get_size();
            DATA_TYPE element;
            Error element_result = deserialize_packed_partial_element(element, section);

            while(Error::NO_ERRORS == element_result)
            {
              return_value = this->add(element);
              if(Error::NO_ERRORS == return_value)
              {
                ++state.element_index;
                element_result = deserialize_packed_partial_element(element, section);
              }
              else
              {
                return_value = Error::ARRAY_FULL;
                element_result = Error::ARRAY_FULL;
              }
            }

            const uint32_t section_size_after = section.get_size();
            const uint32_t bytes_consumed = section_size_before - section_size_after;
            state.bytes_remaining -= bytes_consumed;

            if(Error::NO_ERRORS == return_value)
            {
              if(0U == state.bytes_remaining)
              {
                state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
                return_value = Error::NO_ERRORS;
              }
              else if(Error::END_OF_BUFFER == element_result)
              {
                return_value = Error::END_OF_BUFFER;
              }
              else
              {
                return_value = element_result;
              }
            }
          }
        }
        else
        {
          if((::EmbeddedProto::FieldProcessingPhase::SIZE != state.phase) && (::EmbeddedProto::FieldProcessingPhase::DATA != state.phase))
          {
            return_value = Error::STATE_MISMATCH;
          }

          if(Error::NO_ERRORS == return_value)
          {
            if constexpr(std::is_base_of<Field, DATA_TYPE>::value)
            {
              // Non-packed repeated string/bytes/message element. Each wire
              // occurrence is a separate tag/size/data cycle, and the message loop
              // resets element_index to zero between occurrences. Elements are
              // therefore accumulated based on the current array length rather than
              // element_index. element_index is reused purely as a per-occurrence
              // flag marking that the slot for the current element has already been
              // reserved, so an element split across buffers is not appended twice.
              if(0U == state.element_index)
              {
                if(this->get_max_length() <= this->get_length())
                {
                  return_value = Error::ARRAY_FULL;
                }
                else
                {
                  // Reserve a slot for this new element.
                  (void)this->get(this->get_length());
                  state.element_index = 1U;
                }
              }

              if(Error::NO_ERRORS == return_value)
              {
                return_value = this->get(this->get_length() - 1U).deserialize_partial_as_field(buffer, state);
              }
            }
            else
            {
              if(::EmbeddedProto::FieldProcessingPhase::DATA != state.phase)
              {
                return_value = Error::STATE_MISMATCH;
              }
              else
              {
                DATA_TYPE value;
                return_value = value.deserialize_partial_check_type(
                  buffer,
                  state,
                  fieldtemplate_wire_type<DATA_TYPE>::value);
                if(Error::NO_ERRORS == return_value)
                {
                  return_value = this->add(value);
                  if(Error::NO_ERRORS != return_value)
                  {
                    return_value = Error::ARRAY_FULL;
                  }
                }
              }
            }
          }
        }

        return return_value;
      }

      Error serialize_partial_as_field(uint32_t field_number,
                                     WriteBufferInterface& buffer,
                                     MessageState& state,
                                     bool optional) const override
      {
        // Skip serializing empty non-optional repeated fields (proto3 default behavior).
        if((!optional) && (0U == this->get_length()))
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          return Error::NO_ERRORS;
        }

        return REPEATED_FIELD_IS_PACKED
                 ? serialize_partial_packed(field_number, buffer, state, optional)
                 : serialize_partial_unpacked(field_number, buffer, state);
      }

      //! Serialize a repeated scalar/enum field in EXPANDED form with partial state
      //! support, regardless of the element type's default packing.
      /*!
          Used for the editions repeated_field_encoding = EXPANDED feature so that
          chunked (partial) serialization emits the same one-tag-per-element bytes
          as full serialization instead of a single packed block.
      */
      Error serialize_partial_as_field_expanded(uint32_t field_number,
                                                WriteBufferInterface& buffer,
                                                MessageState& state,
                                                bool optional) const
      {
        // Skip serializing empty non-optional repeated fields (proto3 default behavior).
        if((!optional) && (0U == this->get_length()))
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          return Error::NO_ERRORS;
        }

        return serialize_partial_unpacked(field_number, buffer, state);
      }

    private:
      //! Serialize a single packed element, batching fixed-width values into one push.
      /*!
          For fixed-width scalars the element's bytes are written with a single
          push(bytes, length) call (all-or-nothing). This keeps the resumable
          partial path correct: when the value does not fit, nothing is written
          so bytes_remaining / element_index stay untouched and the element is
          retried cleanly on the next call. Other element types keep their
          existing (byte-at-a-time) serialization.
      */
      Error serialize_packed_element(uint32_t index, WriteBufferInterface& buffer) const
      {
        return serialize_packed_element_(index, buffer,
            std::integral_constant<bool, internal::PackedFixedTraits<DATA_TYPE>::is_fixed_width>{});
      }

      Error serialize_packed_element_(uint32_t index, WriteBufferInterface& buffer,
                                      std::true_type) const
      {
        using VAR = typename internal::PackedFixedTraits<DATA_TYPE>::scalar_type;
        const VAR value = this->get_const(index).get();
        return WireFormatter::SerializeFixedArrayNoTag(&value, 1U, buffer);
      }

      Error serialize_packed_element_(uint32_t index, WriteBufferInterface& buffer,
                                      std::false_type) const
      {
        return this->get_const(index).serialize(buffer);
      }

      //! Packed partial serialization: TAG->SIZE->DATA over one length-delimited block.
      Error serialize_partial_packed(uint32_t field_number,
                                     WriteBufferInterface& buffer,
                                     MessageState& state,
                                     bool optional) const
      {
        Error return_value = Error::NO_ERRORS;

        if(::EmbeddedProto::FieldProcessingPhase::DATA != state.phase)
        {
          // Calculate total packed size
          const uint32_t total_size = serialized_size_packed();
          return_value = serialize_partial_tag_and_size(field_number, total_size, buffer, state, optional);
        }

        if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
        {
          // Serialize elements sequentially
          if(state.element_index < this->get_length())
          {
            const uint32_t initial_size = buffer.get_size();
            return_value = serialize_packed_element(state.element_index, buffer);
            const uint32_t bytes_written = buffer.get_size() - initial_size;
            state.bytes_remaining -= bytes_written;

            if(Error::NO_ERRORS == return_value)
            {
              ++state.element_index;
            }

            if(0 == state.bytes_remaining)
            {
              state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
            }
          }
          else
          {
            state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
          }
        }

        return return_value;
      }

      //! Unpacked (expanded) partial serialization: each element gets its own tag.
      Error serialize_partial_unpacked(uint32_t field_number,
                                       WriteBufferInterface& buffer,
                                       MessageState& state) const
      {
        Error return_value = Error::NO_ERRORS;

        if(state.phase == ::EmbeddedProto::FieldProcessingPhase::COMPLETE)
        {
          // All elements serialized
          return return_value;
        }

        // Handle current element
        if(state.element_index < this->get_length())
        {
          const auto& element = this->get_const(state.element_index);

          // Check if element is a Field-derived type (messages, strings, bytes) or scalar type
          if constexpr(std::is_base_of<Field, DATA_TYPE>::value)
          {
            // Field-derived types (messages, strings, bytes) use serialize_partial_as_field
            return_value = element.serialize_partial_as_field(field_number, buffer, state, true);
          }
          else
          {
            // Scalar types use serialize_partial_with_id
            return_value = element.serialize_partial_with_id(field_number, buffer, state, true);
          }

          if((Error::NO_ERRORS == return_value) && (state.phase == ::EmbeddedProto::FieldProcessingPhase::COMPLETE))
          {
            // Element complete, move to next
            ++state.element_index;
            // Reset child state for next element to avoid stale COMPLETE phase
            // in nested message serialization.
            if(nullptr != state.child)
            {
              state.child->reset();
            }
            state.phase = ::EmbeddedProto::FieldProcessingPhase::TAG; // Reset for next element
          }
        }
        else
        {
          state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
        }

        return return_value;
      }

    public:
#endif


      //! Calculate the size of this field when serialized.
      /*!
          The calculation only includes the data, not the size required by the tag and 
          \return The number of bytes this field will require once serialized.
      */
      uint32_t serialized_size_packed() const 
      {
        ::EmbeddedProto::MessageSizeCalculator calcBuffer;
        serialize(calcBuffer);
        return calcBuffer.get_size();
      }

      //! Calculate the size of this field when serialized.
      /*!
          \return The number of bytes this field will require once serialized.
      */
      uint32_t serialized_size_unpacked(int32_t field_number) const 
      {
        ::EmbeddedProto::MessageSizeCalculator calcBuffer;
        serialize_unpacked(field_number, calcBuffer);
        return calcBuffer.get_size();
      }

#ifdef MSG_TO_STRING

      ::EmbeddedProto::string_view to_string(::EmbeddedProto::string_view& str, const uint32_t indent_level, char const* name, const bool first_field) const override
      {
        ::EmbeddedProto::string_view left_chars = str;
        int32_t n_chars_used = 0;

        if(!first_field)
        {
          // Add a comma behind the previous field.
          n_chars_used = snprintf(left_chars.data, left_chars.size, ",\n");
          if(0 < n_chars_used)
          {
            // Update the character pointer and characters left in the array.
            const int32_t actual_chars_used = EmbeddedProto::min(n_chars_used, left_chars.size);
            left_chars.data += actual_chars_used;
            left_chars.size -= actual_chars_used;
          }
        }

        n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s\"%s\": [\n", indent_level, " ", name );
        
        if(0 < n_chars_used) 
        {
          const int32_t actual_chars_used = EmbeddedProto::min(n_chars_used, left_chars.size);
          left_chars.data += actual_chars_used;
          left_chars.size -= actual_chars_used;
        }

        for(uint32_t i = 0; i < this->get_length(); ++i)
        {
          left_chars = this->get_const(i).to_string(left_chars, n_chars_used, nullptr, (0 == i));
        }

        n_chars_used = snprintf(left_chars.data, left_chars.size, "\n%*s]", n_chars_used - 2, " ");
        
        if(0 < n_chars_used)
        {
          const int32_t actual_chars_used = EmbeddedProto::min(n_chars_used, left_chars.size);
          left_chars.data += actual_chars_used;
          left_chars.size -= actual_chars_used;
        }

        return left_chars;
      }

#endif // End of MSG_TO_STRING

    private:

      Error serialize_unpacked(uint32_t field_number, WriteBufferInterface& buffer) const
      {
        Error return_value = Error::NO_ERRORS;
        for(uint32_t i = 0; (i < this->get_length()) && (Error::NO_ERRORS == return_value); ++i)
        {
          // Each element gets its own [tag][size][data]
          // Note: serialize_len() is only available on Field-derived types (messages, strings, bytes)
          // which is exactly what unpacked mode is used for.
          const uint32_t size_x = this->get_const(i).serialized_size();
          uint32_t tag = WireFormatter::MakeTag(field_number, 
                                    WireFormatter::WireType::LENGTH_DELIMITED);
          return_value = WireFormatter::SerializeVarint(tag, buffer);
          if(Error::NO_ERRORS == return_value)
          {
            return_value = WireFormatter::SerializeVarint(size_x, buffer);
            if((Error::NO_ERRORS == return_value) && (0 < size_x)) 
            {
              return_value = this->get_const(i).serialize(buffer);
            }
          }
        }
        return return_value;
      }

    protected:

      //! Deserialize one packed length-delimited block into this array.
      /*!
          Non-virtual helper (no vtable slot): the fixed-width whole-block fast
          path lives in RepeatedFieldFixedSize's deserialize() override, which
          reuses the existing Field::deserialize slot.
      */
      Error deserialize_packed(ReadBufferInterface& buffer)
      {
        uint32_t size = 0;
        Error return_value = WireFormatter::DeserializeVarint(buffer, size);
        ReadBufferSection bufferSection(buffer, size);
        return_value = deserialize_packed_section(bufferSection);
        return return_value;
      }

      //! Element-by-element loop over an already length-bounded packed section.
      /*!
          The caller must pass a buffer bounded to the packed block (a
          ReadBufferSection); the loop reads elements until it is exhausted
          (END_OF_BUFFER, the expected clean end, mapped back to NO_ERRORS) or an
          error occurs. Takes the base ReadBufferInterface: the boundary is
          enforced by the section object through virtual dispatch, so the concrete
          type is not needed here. Shared with the fixed-width override for its
          fall-back cases.
      */
      Error deserialize_packed_section(ReadBufferInterface& buffer)
      {
        DATA_TYPE x;

        Error return_value = x.deserialize(buffer);
        while(Error::NO_ERRORS == return_value)
        {
          return_value = this->add(x);
          if(Error::NO_ERRORS == return_value)
          {
            return_value = x.deserialize(buffer);
          }
        }

        // We expect the buffersection to be empty, in that case everything is fine..
        if(Error::END_OF_BUFFER == return_value)
        {
          return_value = Error::NO_ERRORS;
        }

        return return_value;
      }

    private:

      Error deserialize_unpacked(ReadBufferInterface& buffer)
      {
        Error return_value = Error::NO_ERRORS;

        // For repeated messages, strings or bytes
        // First allocate an element in the array.
        const uint32_t index = this->get_length();
        if(this->get_max_length() > index)
        {
          // For messages read the size here, with strings and byte arrays this is include in 
          // deserialize.
          if(std::is_base_of<MessageInterface, DATA_TYPE>::value)
          {
            uint32_t size;
            return_value = WireFormatter::DeserializeVarint(buffer, size);
            if(Error::NO_ERRORS == return_value) 
            {
              ReadBufferSection bufferSection(buffer, size);
              return_value = this->get(index).deserialize(bufferSection);
            }
          }
          else 
          {
            return_value = this->get(index).deserialize(buffer);
          }
        }
        else 
        {
          return_value = Error::ARRAY_FULL;
        }

        return return_value;
      }

#ifdef PARTIAL_SERIALIZATION_ENABLED
      //! Read one packed element, batching fixed-width scalars into a single read.
      /*!
          For fixed-width scalar element types the whole element is read with one
          batched pop (via DeserializeFixedArrayNoTag) instead of a per-byte peek
          loop. The read is all-or-nothing against the section boundary: an element
          that is not fully present in the current section leaves the element and
          the section untouched and returns END_OF_BUFFER, so the resumable partial
          path continues at the same element/bytes_remaining on the next buffer
          refill. All other element types keep their existing deserialization.
      */
      Error deserialize_packed_partial_element(DATA_TYPE& element, ReadBufferSection& section)
      {
        if constexpr(internal::PackedFixedTraits<DATA_TYPE>::is_fixed_width)
        {
          using VAR = typename internal::PackedFixedTraits<DATA_TYPE>::scalar_type;
          return WireFormatter::DeserializeFixedArrayNoTag<VAR>(&element.get(), 1U, section);
        }
        else
        {
          return element.deserialize(section);
        }
      }
#endif // PARTIAL_SERIALIZATION_ENABLED

  };


} // End of namespace EmbeddedProto

#endif // End of _REPEATED_FIELD_H_
