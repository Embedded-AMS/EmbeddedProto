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

#ifndef _REPEATED_FIELD_SIZE_H_
#define _REPEATED_FIELD_SIZE_H_

#include "RepeatedField.h"
#include "Errors.h"

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>
#include <type_traits>


namespace EmbeddedProto
{

  //! A template class that actually holds some data.
  /*!
    This is a separate class to make it possible to not have the size defined in every function or 
    class using this type of object.
  */
  template<class DATA_TYPE, uint32_t MAX_LENGTH>
  class RepeatedFieldFixedSize : public RepeatedField<DATA_TYPE>
  { 
      static constexpr uint32_t BYTES_PER_ELEMENT = sizeof(DATA_TYPE);

    public:

      RepeatedFieldFixedSize() = default;
      ~RepeatedFieldFixedSize() override = default;

      RepeatedFieldFixedSize(const RepeatedFieldFixedSize<DATA_TYPE, MAX_LENGTH>& rhs) :
        current_length_(rhs.get_length()),
        data_(rhs.get_data_const())
      {
        // Use the initializer list.
      }

      template<uint32_t MAX_LENGTH_RHS, typename std::enable_if<(MAX_LENGTH_RHS < MAX_LENGTH), int>::type = 0>
      explicit RepeatedFieldFixedSize(const RepeatedFieldFixedSize<DATA_TYPE, MAX_LENGTH_RHS>& rhs) :
        current_length_(rhs.get_length())
      {
        const auto& rhs_data = rhs.get_data_const();
        std::copy(rhs_data.begin(), rhs_data.end(), data_.begin());
      }

      template<uint32_t MAX_LENGTH_RHS, typename std::enable_if<(MAX_LENGTH_RHS < MAX_LENGTH), int>::type = 0>
      explicit RepeatedFieldFixedSize(const RepeatedFieldFixedSize<DATA_TYPE, MAX_LENGTH_RHS>&& rhs) :
        current_length_(rhs.get_length())
      {
        const auto& rhs_data = rhs.get_data_const();
        std::copy(rhs_data.begin(), rhs_data.end(), data_.begin());
      }

      //! Assign one repieted field to the other, but only when the length and type matches.
      RepeatedFieldFixedSize<DATA_TYPE, MAX_LENGTH>& operator=(const 
                                                RepeatedFieldFixedSize<DATA_TYPE, MAX_LENGTH>& rhs)
      {
        for(uint32_t i = 0; i < rhs.get_length(); ++i) 
        {
          data_[i] = rhs.get_const(i);
        }
        current_length_ = rhs.get_length();
        
        return *this;
      }

      //! Obtain the total number of DATA_TYPE items in the array.
      uint32_t get_length() const override { return current_length_; }

      //! Obtain the maximum number of DATA_TYPE items which can at most be stored in the array.
      uint32_t get_max_length() const override { return MAX_LENGTH; }

      //! Obtain the total number of bytes currently stored in the array.
      uint32_t get_size() const override { return BYTES_PER_ELEMENT * current_length_; }

      //! Obtain the maximum number of bytes which can at most be stored in the array.
      uint32_t get_max_size() const override { return BYTES_PER_ELEMENT * MAX_LENGTH; }

      DATA_TYPE& get(uint32_t index) override 
      { 
        uint32_t limited_index = std::min(index, MAX_LENGTH-1);
        // Check if we need to update the number of elements in the array.
        if(limited_index >= current_length_) {
          current_length_ = limited_index + 1;
        }
        return data_[limited_index]; 
      }

      const DATA_TYPE& get_const(uint32_t index) const override 
      { 
        uint32_t limited_index = std::min(index, MAX_LENGTH-1);
        return data_[limited_index]; 
      }

      Error get_const(const uint32_t index, DATA_TYPE& value) const override
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

      void set(uint32_t index, const DATA_TYPE& value) override 
      { 
        uint32_t limited_index = std::min(index, MAX_LENGTH-1);
        // Check if we need to update the number of elements in the array.
        if(limited_index >= current_length_) {
          current_length_ = limited_index + 1;
        }
        data_[limited_index] = value;  
      }

      Error set_data(const DATA_TYPE* data, const uint32_t length) override 
      {
        Error return_value = Error::NO_ERRORS;
        if(MAX_LENGTH >= length) 
        {
          const DATA_TYPE* d = data;
          for(uint32_t i = 0; i < length; ++i) 
          {
            (data_[i]) = (*d);
            ++d;
          }
          current_length_ = length;        
        }
        else 
        {
          return_value = Error::ARRAY_FULL;
        }
        return return_value;
      }

      Error add(const DATA_TYPE& value) override 
      {
        Error return_value = Error::NO_ERRORS;
        if(MAX_LENGTH > current_length_) 
        {
          data_[current_length_] = value;
          ++current_length_;
        }
        else 
        {
          return_value = Error::ARRAY_FULL;
        }
        return return_value;
      }

      void clear() override
      {
        for(auto& d : data_)
        {
          d.clear();
        }
        current_length_ = 0;
      }

      //! Serialize all elements (packed), batching fixed-width payloads.
      /*!
          For packed fixed-width scalar element types (fixed32/sfixed32/float and
          fixed64/sfixed64/double) the contiguous backing array is written to the
          buffer as a single whole-block push(bytes, length) on a little-endian
          target instead of one virtual call per byte. All other element types
          fall back to the element-by-element base implementation.
      */
      Error serialize(WriteBufferInterface& buffer) const override
      {
        return serialize_packed_(buffer,
            std::integral_constant<bool,
                RepeatedField<DATA_TYPE>::REPEATED_FIELD_IS_PACKED
                && ::EmbeddedProto::internal::PackedFixedTraits<DATA_TYPE>::is_fixed_width>{});
      }

      //! Deserialize all elements, batching packed fixed-width payloads into one read.
      /*!
          Reuses the existing Field::deserialize vtable slot (no new virtual). For
          packed fixed-width scalar element types the whole little-endian block is
          read straight into the contiguous backing array with a single batched
          pop, instead of one virtual call per byte. Misaligned / oversized /
          truncated blocks, and non-fixed-width / non-packed element types, fall
          back to the base implementation so their existing behaviour is preserved.
      */
      Error deserialize(::EmbeddedProto::ReadBufferInterface& buffer) override
      {
        return deserialize_(buffer,
            std::integral_constant<bool,
                RepeatedField<DATA_TYPE>::REPEATED_FIELD_IS_PACKED
                && ::EmbeddedProto::internal::PackedFixedTraits<DATA_TYPE>::is_fixed_width>{});
      }

      //! Return a reference to the internal data storage array.
      const std::array<DATA_TYPE, MAX_LENGTH>& get_data_const() const { return data_; }

      //! When serialized with the most unfavrouble value how much bytes does this field need.
      /*!
        This function takes into account the field number and tag combination.
        \param[in] field_number We need to include the field number. This because large field numbers require more bytes.
        \return The number of bytes required at most.
      */
      static constexpr uint32_t max_serialized_size(const uint32_t field_number)
      {
        return RepeatedField<DATA_TYPE>::REPEATED_FIELD_IS_PACKED 
                  ? max_packed_serialized_size(field_number)
                  : max_unpacked_serialized_size(field_number);
      }

      static constexpr uint32_t max_packed_serialized_size(const uint32_t field_number)
      {
        return WireFormatter::VarintSize(WireFormatter::MakeTag(field_number, 
                                                                WireFormatter::WireType::LENGTH_DELIMITED))
          + WireFormatter::VarintSize(MAX_LENGTH)
          + (MAX_LENGTH * DATA_TYPE::max_serialized_size());
      }

      static constexpr uint32_t max_unpacked_serialized_size(const uint32_t field_number)
      {
        return MAX_LENGTH * DATA_TYPE::max_serialized_size(field_number);
      }


    private:

      //! Whole-block batched deserialize for packed fixed-width scalar elements.
      Error deserialize_(::EmbeddedProto::ReadBufferInterface& buffer, std::true_type)
      {
#if EMBEDDED_PROTO_LITTLE_ENDIAN
        using VAR = typename ::EmbeddedProto::internal::PackedFixedTraits<DATA_TYPE>::scalar_type;
        static_assert(sizeof(DATA_TYPE) == sizeof(VAR),
                      "Fixed-width field must be layout-compatible with its scalar type.");
        static_assert(std::is_standard_layout<DATA_TYPE>::value,
                      "Fixed-width field must be standard-layout for block deserialization.");

        uint32_t size = 0;
        Error return_value = WireFormatter::DeserializeVarint(buffer, size);
        const uint32_t count = size / BYTES_PER_ELEMENT;

        // Fast path: a whole, correctly sized block that fits in the array and is
        // entirely present in the buffer is read in a single batched pop. Values
        // are appended after any already-decoded elements (repeated packed fields
        // may appear more than once and concatenate).
        if((Error::NO_ERRORS == return_value)
           && (0U == (size % BYTES_PER_ELEMENT))
           && ((current_length_ + count) <= MAX_LENGTH)
           && (buffer.get_size() >= size))
        {
          VAR* const raw = reinterpret_cast<VAR*>(data_.data()) + current_length_;
          return_value = WireFormatter::DeserializeFixedArrayNoTag(raw, count, buffer);
          if(Error::NO_ERRORS == return_value)
          {
            current_length_ += count;
          }
        }
        else
        {
          // Misaligned / oversized / truncated block: defer to the base loop which
          // preserves the previous lenient handling of such input.
          ReadBufferSection section(buffer, size);
          return_value = this->deserialize_packed_section(section);
        }
        return return_value;
#else
        // Qualified base call (non-virtual) reads the size varint fresh and runs
        // the element-by-element loop; no recursion back into this override.
        return RepeatedField<DATA_TYPE>::deserialize(buffer);
#endif
      }

      //! Non-fixed-width / non-packed element types keep the base implementation.
      Error deserialize_(::EmbeddedProto::ReadBufferInterface& buffer, std::false_type)
      {
        // Qualified base call (non-virtual): no recursion into this override.
        return RepeatedField<DATA_TYPE>::deserialize(buffer);
      }

      //! Whole-block batched serialize for packed fixed-width scalar elements.
      Error serialize_packed_(WriteBufferInterface& buffer, std::true_type) const
      {
        using VAR = typename ::EmbeddedProto::internal::PackedFixedTraits<DATA_TYPE>::scalar_type;
        static_assert(sizeof(DATA_TYPE) == sizeof(VAR),
                      "Fixed-width field must be layout-compatible with its scalar type.");
        static_assert(std::is_standard_layout<DATA_TYPE>::value,
                      "Fixed-width field must be standard-layout for block serialization.");
        const VAR* const raw = reinterpret_cast<const VAR*>(data_.data());
        return WireFormatter::SerializeFixedArrayNoTag(raw, current_length_, buffer);
      }

      //! Fallback to the element-by-element base implementation.
      Error serialize_packed_(WriteBufferInterface& buffer, std::false_type) const
      {
        return RepeatedField<DATA_TYPE>::serialize(buffer);
      }

      //! Number of item in the data array.
      uint32_t current_length_ = 0;

      //! The actual data 
      std::array<DATA_TYPE, MAX_LENGTH> data_ = {};
  };

} // End of namespace EmbeddedProto

#endif // End of _REPEATED_FIELD_SIZE_H_