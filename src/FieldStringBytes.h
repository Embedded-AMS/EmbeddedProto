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

#ifndef _FIELD_STRING_BYTES_H_
#define _FIELD_STRING_BYTES_H_

#include "Defines.h"
#include "Fields.h"
#include "EmptyArray.h"
#include "Errors.h"

#include <cstdint>
#include <cstring>
#include <type_traits>
#include <array>


namespace EmbeddedProto
{

  namespace internal
  {

    class BaseStringBytes : public Field {};

    template<uint32_t MAX_LENGTH, class DATA_TYPE>
    class FieldStringBytes : public BaseStringBytes
    {
      static_assert(std::is_same<uint8_t, DATA_TYPE>::value || std::is_same<char, DATA_TYPE>::value, 
                    "This class only supports unit8_t or chars.");

      public:

        FieldStringBytes() = default;
        
        ~FieldStringBytes() override = default;
        
        //! Obtain the number of characters in the string right now.
        uint32_t get_length() const { return current_length_; }

        //! Obtain the maximum number characters in the string.
        uint32_t get_max_length() const { return MAX_LENGTH; }

        //! Get a constant pointer to the first element in the array.
        const DATA_TYPE* get_const() const { return data_.data(); }

        //! Get a reference to the value at the given index. 
        /*!
          This function will update the number of elements used in the array/string.

          \param[in] index The desired index to return.
          \return The reference to the value at the given index. Will return the last element if the 
                  index is out of bounds
        */
        DATA_TYPE& get(uint32_t index) 
        { 
          uint32_t limited_index = clamp_index(index);
          // Check if we need to update the number of elements in the array.
          if(limited_index >= current_length_) {
            current_length_ = std::min(limited_index + 1U, MAX_LENGTH);
          }
          return element(limited_index); 
        }

        //! Get a constant reference to the value at the given index. 
        /*!
          \param[in] index The desired index to return.
          \return The reference to the value at the given index. Will return the last element if the 
                  index is out of bounds
        */
        const DATA_TYPE& get_const(uint32_t index) const 
        { 
          return element(clamp_index(index)); 
        }

        //! Get a constant reference to the value at the given index.
        /*!
          \param[in] index The desired index to return.
          \param[out] value The value of the desired index is set in this reference.
          \return An error incase of an index out of bound situation.
        */
        Error get_const(const uint32_t index, DATA_TYPE& value) const
        {
          Error result = Error::NO_ERRORS;
          if(index < current_length_)
          {
            value = data_[index];
          }
          else
          {
            result = Error::INDEX_OUT_OF_BOUND;
          }
          return result;
        }

        //! Get a reference to the value at the given index. 
        /*!
          This function will update the number of elements used in the array/string.

          \param[in] index The desired index to return.
          \return The reference to the value at the given index. Will return the last element if the 
                  index is out of bounds
        */
        DATA_TYPE& operator[](uint32_t index) { return this->get(index); }

        //! Get a constant reference to the value at the given index. 
        /*!
          \param[in] index The desired index to return.
          \return The reference to the value at the given index. Will return the last element if the 
                  index is out of bounds
        */
        const DATA_TYPE& operator[](uint32_t index) const { return this->get_const(index); }

        //! Assign the values in the right hand side FieldStringBytes object to this object.
        /*!
            This is only compatible with the same data type and length.
            \param[in] rhs The object from which to copy the data.
            \return Always return NO_ERRORS, this was added to be compadible with the other set function.
        */
        template<uint32_t RHS_LENGTH> 
        Error set(const FieldStringBytes<RHS_LENGTH, DATA_TYPE>& rhs)
        {
          return this->set(rhs.get_const(), rhs.get_length());
        }

        //! Assign data in the given array to this object.
        /*!
            \param[in] data A pointer to an array with data.
            \param[in] length The number of bytes/chars in the data array.
            \return Will return ARRAY_FULL when length exceeds the number of bytes/chars in this object.
        */        
        Error set(const DATA_TYPE* data, const uint32_t length)
        {
          Error return_value = Error::NO_ERRORS;
          if(MAX_LENGTH >= length)
          {
            current_length_ = length;
            memcpy(data_.data(), data, length);
          }
          else
          {
            return_value = Error::ARRAY_FULL;
          }
          return return_value;
        }

        //! Compare the data held by this object with that of another string or bytes field.
        /*!
            The maximum lengths of the two objects may differ, only the data actually held is
            compared. Used among others to look up a key in a map field.

            \param[in] rhs The object to compare this one with.
            \return True when both hold the same number of bytes and all of them are equal.
        */
        template<uint32_t RHS_LENGTH>
        bool operator==(const FieldStringBytes<RHS_LENGTH, DATA_TYPE>& rhs) const
        {
          return (current_length_ == rhs.get_length()) &&
                 (0 == memcmp(data_.data(), rhs.get_const(), current_length_));
        }

        //! Compare the data held by this object with that of another string or bytes field.
        /*!
            \param[in] rhs The object to compare this one with.
            \return True when the two differ in length or in any of the bytes held.
        */
        template<uint32_t RHS_LENGTH>
        bool operator!=(const FieldStringBytes<RHS_LENGTH, DATA_TYPE>& rhs) const
        {
          return !(*this == rhs);
        }

        Error serialize(WriteBufferInterface& buffer) const override 
        { 
          Error return_value = Error::NO_ERRORS;
          const auto* void_pointer = static_cast<const void*>(&(data_[0]));
          const auto* byte_pointer = static_cast<const uint8_t*>(void_pointer);
          if(!buffer.push(byte_pointer, current_length_))
          {
            return_value = Error::BUFFER_FULL;
          }
          return return_value;
        }

        Error deserialize(ReadBufferInterface& buffer) override 
        {
          uint32_t availiable = 0;
          Error return_value = WireFormatter::DeserializeVarint(buffer, availiable);
          if(Error::NO_ERRORS == return_value)
          {
            if(MAX_LENGTH >= availiable) 
            {
              clear();

              uint8_t byte = 0;
              while((current_length_ < availiable) && buffer.pop(byte)) 
              {
                (data_[current_length_]) = static_cast<DATA_TYPE>(byte);
                ++current_length_;
              }

              if(current_length_ != availiable)
              {
                // If at the end we did not read the same number of characters something went wrong.
                return_value = Error::END_OF_BUFFER;
              }
            }
            else 
            {
              return_value = Error::ARRAY_FULL;
            }
          }

          return return_value;
        }
        
        Error deserialize_check_type(::EmbeddedProto::ReadBufferInterface& buffer,
                                     const ::EmbeddedProto::WireFormatter::WireType& wire_type) final
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
        Error deserialize_partial_as_field(ReadBufferInterface& buffer,
                                           MessageState& state) override
        {
          Error return_value = Error::NO_ERRORS;
          bool size_phase_processed = false;

          if((::EmbeddedProto::FieldProcessingPhase::SIZE != state.phase) && (::EmbeddedProto::FieldProcessingPhase::DATA != state.phase))
          {
            return_value = Error::STATE_MISMATCH;
          }

          if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::SIZE == state.phase))
          {
            return_value = deserialize_partial_size_phase(buffer, state);
            if(Error::NO_ERRORS == return_value)
            {
              size_phase_processed = true;
            }
          }

          if((Error::NO_ERRORS == return_value) && size_phase_processed)
          {
            clear();
            if(MAX_LENGTH < state.bytes_remaining)
            {
              return_value = Error::ARRAY_FULL;
            }
          }

          if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
          {
            const uint32_t bytes_to_read = std::min(state.bytes_remaining, buffer.get_size());
            for(uint32_t i = 0U; i < bytes_to_read; ++i)
            {
              uint8_t byte = 0U;
              if(buffer.pop(byte))
              {
                data_[current_length_] = static_cast<DATA_TYPE>(byte);
                ++current_length_;
              }
              else
              {
                return_value = Error::END_OF_BUFFER;
              }
            }

            if(Error::NO_ERRORS == return_value)
            {
              state.bytes_remaining -= bytes_to_read;
              if(0U == state.bytes_remaining)
              {
                state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
              }
              else
              {
                return_value = Error::END_OF_BUFFER;
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
          Error return_value = Error::NO_ERRORS;

          // Handle TAG and SIZE phases using helper method
          if(::EmbeddedProto::FieldProcessingPhase::DATA != state.phase)
          {
            return_value = serialize_partial_tag_and_size(field_number, get_length(), buffer, state, optional);
          }

          // Handle DATA phase
          if((Error::NO_ERRORS == return_value) && (::EmbeddedProto::FieldProcessingPhase::DATA == state.phase))
          {
            // Calculate how many bytes we can write (limited by buffer space and remaining data)
            const uint32_t bytes_to_write = std::min(state.bytes_remaining, buffer.get_available_size());

            if(bytes_to_write > 0)
            {
              // Calculate starting position in data array
              const uint32_t start_offset = get_length() - state.bytes_remaining;
              const auto* void_pointer = static_cast<const void*>(&(data_[start_offset]));
              const auto* byte_pointer = static_cast<const uint8_t*>(void_pointer);

              // Try to write all bytes at once first
              if(buffer.push(byte_pointer, bytes_to_write))
              {
                state.bytes_remaining -= bytes_to_write;
                if(0 == state.bytes_remaining)
                {
                  state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
                  return_value = Error::NO_ERRORS;
                }
                else
                {
                  return_value = Error::BUFFER_FULL;
                }
              }
              else
              {
                // Buffer push failed - this can happen when the buffer's push method
                // uses > instead of >=, so we can't fill the buffer completely.
                // In this case, try to write bytes one at a time.
                uint32_t bytes_written = 0;
                bool push_more = true;
                for(uint32_t i = 0; (i < bytes_to_write) && push_more; ++i)
                {
                  push_more = buffer.push(byte_pointer[i]);
                  if(push_more)
                  {
                      bytes_written++;
                  }
                }

                if(bytes_written > 0)
                {
                  state.bytes_remaining -= bytes_written;
                  if(0 == state.bytes_remaining)
                  {
                    state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
                    return_value = Error::NO_ERRORS;
                  }
                  else
                  {
                    return_value = Error::BUFFER_FULL;
                  }
                }
                else
                {
                  // Couldn't write any bytes - this should not happen unless buffer is completely full
                  // To prevent infinite loops, we need to ensure progress is made
                  // If we can't write any bytes and there are still bytes remaining, we have a problem
                  if(state.bytes_remaining > 0)
                  {
                    // This is the infinite loop scenario - buffer is full but we can't write any bytes
                    // We need to return BUFFER_FULL to indicate we need a new buffer
                    return_value = Error::BUFFER_FULL;
                  }
                  else
                  {
                    state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
                    return_value = Error::NO_ERRORS;
                  }
                }
              }
            }
            else
            {
              // No space available in buffer - this can happen if buffer is completely full
              // In this case, we need to ensure we don't get stuck in an infinite loop
              // by checking if we've made any progress
              if(state.bytes_remaining > 0)
              {
                return_value = Error::BUFFER_FULL;
              }
              else
              {
                state.phase = ::EmbeddedProto::FieldProcessingPhase::COMPLETE;
                return_value = Error::NO_ERRORS;
              }
            }
          }

          return return_value;
        }
#endif

        //! Reset the field to it's initial value.
        void clear() override
        {
          data_.fill(0);
          current_length_ = 0;
        }

        //! When serialized with the all elements set, how much bytes are then required.
        /*!
          This function takes into account the field number and tag combination.
          \param[in] field_number We need to include the field number. This because large field numbers require more bytes.
          \return The number of bytes required at most.
        */
        static constexpr uint32_t max_serialized_size(const uint32_t field_number)
        {
          return MAX_LENGTH // The number of bytes of the data.
                  + WireFormatter::VarintSize(MAX_LENGTH) // The varint indicating the actual number of bytes.
                  + WireFormatter::VarintSize(WireFormatter::MakeTag(field_number, 
                                                                     WireFormatter::WireType::LENGTH_DELIMITED)); // The field and tag comby
        }

        //! When serialized with the all elements set, how much bytes are then required.
        /*!
          This function is used when the field bytes or string is serialized packed. Think in a repeated field.
          \return The number of bytes required at most.
        */
        static constexpr uint32_t max_serialized_size()
        {
          return MAX_LENGTH // The number of bytes of the data.
                  + WireFormatter::VarintSize(MAX_LENGTH); // The varint indicating the actual number of bytes.
        }
       
      protected:

        //! Set the current number of items in the array. Only for internal usage.
        /*!
            The value is limited to the maximum lenght of the array.
        */
        void set_length(uint32_t length) { current_length_ = std::min(length, MAX_LENGTH); }

        //! Get a non constant pointer to the first element in the array. Only for internal usage.
        DATA_TYPE* get() { return data_.data(); }

      private:

        //! The number of characters reserved behind MAX_LENGTH for a null terminator.
        /*!
            Define NULL_TERMINATED_STRINGS to reserve one character per string. It is zero from
            construction on and no write path ever reaches it, every one of them stops at
            MAX_LENGTH, so get_const() of a completely full string is a valid c style string
            without any bookkeeping. The cost is one byte of RAM per string field. A bytes field
            never reserves it, a byte array has no terminator. The default is to reserve nothing,
            which keeps the memory layout of earlier versions.
        */
#ifdef NULL_TERMINATED_STRINGS
        static constexpr uint32_t TERMINATOR_LENGTH = std::is_same<char, DATA_TYPE>::value ? 1U : 0U;
#else
        static constexpr uint32_t TERMINATOR_LENGTH = 0U;
#endif

        //! Number of item in the data array.
        uint32_t current_length_ = 0;

        //! Storage type, an empty stand-in when there is nothing to store, see EmptyArray.
        using Storage = typename std::conditional<(0U < (MAX_LENGTH + TERMINATOR_LENGTH)),
                                                  std::array<DATA_TYPE, MAX_LENGTH + TERMINATOR_LENGTH>,
                                                  internal::EmptyArray<DATA_TYPE>>::type;

        //! The text, plus the reserved terminator when enabled.
        Storage data_ = {};

        //! Clamp an index to the last element, zero when there is no element at all.
        static constexpr uint32_t clamp_index(const uint32_t index)
        {
          return (0U < MAX_LENGTH) ? std::min(index, MAX_LENGTH - 1U) : 0U;
        }

        //! The element at a clamped index, or the scratch element when there is no element at all.
        /*!
            With a maximum length of zero the storage may still hold the reserved terminator. That
            slot must never be handed out, so index access goes to the EmptyArray scratch element.
        */
        DATA_TYPE& element(const uint32_t index)
        {
          if constexpr(0U < MAX_LENGTH)
          {
            return data_[index];
          }
          else
          {
            return internal::EmptyArray<DATA_TYPE>()[index];
          }
        }

        const DATA_TYPE& element(const uint32_t index) const
        {
          if constexpr(0U < MAX_LENGTH)
          {
            return data_[index];
          }
          else
          {
            return internal::EmptyArray<DATA_TYPE>()[index];
          }
        }

    }; // End of class FieldStringBytes

  } // End of namespace internal

  //! The class definition used in messages for String fields.
  template<uint32_t MAX_LENGTH>
  class FieldString : public internal::FieldStringBytes<MAX_LENGTH, char>
  {
    public:

      using internal::FieldStringBytes<MAX_LENGTH, char>::set;

      FieldString() = default;
      ~FieldString() override = default;

      //! Assign the values in the right hand side FieldStringBytes object to this object.
      /*!
          This is only compatible with the same data type and length.
          \param[in] rhs The object from which to copy the data.
          \return A reference to this object.
      */
      template<uint32_t RHS_LENGTH> 
      FieldString<MAX_LENGTH>& operator=(const FieldString<RHS_LENGTH>& rhs)
      {
        this->set(rhs.get_const(), rhs.get_length());
        return *this;
      }

      //! Assign a c style string to this object.
      /*!
          A short example:
            char text[] = "Foo bar";
            msg.mutable_txt() = text;
          
          \param[in] rhs The c style string from which to take the characters and copy it to this object.
          \return A reference to this object used for function chaining.
      */
      FieldString<MAX_LENGTH>& operator=(const char* const rhs)
      {
        this->set(rhs);
        return *this;
      }

      //! Compare the characters in this object with a c style string.
      /*!
          A short example:
            if(msg.get_name() == "Foo bar") { }

          \param[in] rhs The c style string to compare the characters in this object with.
          \return True when both hold the same number of characters and all of them are equal.
      */
      bool operator==(const char* const rhs) const
      {
        bool result = false;
        if(nullptr != rhs)
        {
          const uint32_t rhs_length = strnlen(rhs, MAX_LENGTH + 1);
          result = (rhs_length == this->get_length()) &&
                   (0 == memcmp(this->get_const(), rhs, rhs_length));
        }
        return result;
      }

      //! Compare the characters in this object with a c style string.
      /*!
          \param[in] rhs The c style string to compare the characters in this object with.
          \return True when the two differ in length or in any of the characters held.
      */
      bool operator!=(const char* const rhs) const
      {
        return !(*this == rhs);
      }

      //! Does the given c style string fit in this object without being cut short?
      /*!
          set() stores at most MAX_LENGTH characters and drops the rest. Check first when a cut
          short string would be wrong rather than merely shorter, as with a map key.

          \param[in] str The c style string to check. A nullptr counts as an empty string.
          \return True when the string is at most MAX_LENGTH characters long.
      */
      static bool fits(const char* const str)
      {
        return (nullptr == str) || (MAX_LENGTH >= strnlen(str, MAX_LENGTH + 1));
      }

      //! Assign the data from the given c style string to this object.
      /*!
          \param[in] str The c style string from which to take the characters and copy it to this object.
      */
      void set(const char* const str)
      {
        if(nullptr != str) {
          const uint32_t str_MAX_LENGTH = strnlen(str, MAX_LENGTH + 1);
          this->set_length(str_MAX_LENGTH);
          uint32_t this_length = this->get_length();
          // If it fits in this object copy the null terminator.
          if(MAX_LENGTH > this_length) {
            ++this_length;
          }
          strncpy(this->get(), str, this_length);
        }
        else {
          this->clear();
        }      
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

        if(nullptr != name)
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s\"%s\": \"%s\"", indent_level, " ", name, this->get_const());
        }
        else
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s\"%s\"", indent_level, " ", this->get_const());
        }
        
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
      //! Use our own implementation of limited string length function.
      /*!
          \param s The character array.
          \param len The maximum length to search for a null terminator.
          
          \return The length of this character array will be returned or the value of len.
      */
      static uint32_t strnlen(const char* s, uint32_t len)
      {
        uint32_t i = 0;
        for(; (i < len) && (s[i] != '\0'); ++i)
        {
          // Do nothing but the loop checks.
        }
        return i;
      }

  };

  //! The class definition used in messages for Bytes fields.
  template<uint32_t MAX_LENGTH>
  class FieldBytes : public internal::FieldStringBytes<MAX_LENGTH, uint8_t>
  {
    public:
      FieldBytes() = default;
      ~FieldBytes() override = default;

      //! Assign the values in the right hand side FieldStringBytes object to this object.
      /*!
          This is only compatible with the same data type and length.
          \param[in] rhs The object from which to copy the data.
          \return A reference to this object.
      */
      template<uint32_t RHS_LENGTH> 
      FieldBytes<MAX_LENGTH>& operator=(const FieldBytes<RHS_LENGTH>& rhs)
      {
        this->set(rhs.get_const(), rhs.get_length());
        return *this;
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

        if(nullptr != name)
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s\"%s\": [\n", indent_level, " ", name );
        }
        else
        {
          n_chars_used = snprintf(left_chars.data, left_chars.size, "%*s[\n", indent_level, " ");
        }
        
        if(0 < n_chars_used) 
        {
          const int32_t actual_chars_used = EmbeddedProto::min(n_chars_used, left_chars.size);
          left_chars.data += actual_chars_used;
          left_chars.size -= actual_chars_used;
        }

        uint32 field;
        for(uint32_t i = 0; i < this->get_length(); ++i)
        {
          field = this->get_const(i);
          left_chars = field.to_string(left_chars, n_chars_used, nullptr, (0 == i));
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
  };


} // End of namespace EmbeddedProto

#endif // End of _FIELD_STRING_BYTES_H_
