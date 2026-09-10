/*
 *  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, version 3 of the license.
 */

#ifndef _MOCK_REPEATED_FIELD_STORAGE_H_
#define _MOCK_REPEATED_FIELD_STORAGE_H_

#include <RepeatedField.h>
#include <Errors.h>

#include <algorithm>
#include <array>

namespace Mocks
{

template<class DATA_TYPE, uint32_t MAX_LENGTH>
class MockRepeatedFieldStorage final : public ::EmbeddedProto::RepeatedField<DATA_TYPE>
{
  public:
    MockRepeatedFieldStorage() = default;
    ~MockRepeatedFieldStorage() override = default;

    uint32_t get_length() const override { return current_length_; }
    uint32_t get_max_length() const override { return MAX_LENGTH; }
    uint32_t get_size() const override { return static_cast<uint32_t>(sizeof(DATA_TYPE)) * current_length_; }
    uint32_t get_max_size() const override { return static_cast<uint32_t>(sizeof(DATA_TYPE)) * MAX_LENGTH; }

    DATA_TYPE& get(uint32_t index) override
    {
      const uint32_t limited_index = std::min(index, MAX_LENGTH - 1U);
      if(limited_index >= current_length_)
      {
        current_length_ = limited_index + 1U;
      }
      return data_[limited_index];
    }

    const DATA_TYPE& get_const(uint32_t index) const override
    {
      const uint32_t limited_index = std::min(index, MAX_LENGTH - 1U);
      return data_[limited_index];
    }

    ::EmbeddedProto::Error get_const(const uint32_t index, DATA_TYPE& value) const override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;
      if(index < current_length_)
      {
        value = data_[index];
      }
      else
      {
        return_value = ::EmbeddedProto::Error::INDEX_OUT_OF_BOUND;
      }
      return return_value;
    }

    void set(uint32_t index, const DATA_TYPE& value) override
    {
      const uint32_t limited_index = std::min(index, MAX_LENGTH - 1U);
      if(limited_index >= current_length_)
      {
        current_length_ = limited_index + 1U;
      }
      data_[limited_index] = value;
    }

    ::EmbeddedProto::Error set_data(const DATA_TYPE* data, const uint32_t length) override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;
      if(length <= MAX_LENGTH)
      {
        for(uint32_t i = 0U; i < length; ++i)
        {
          data_[i] = data[i];
        }
        current_length_ = length;
      }
      else
      {
        return_value = ::EmbeddedProto::Error::ARRAY_FULL;
      }
      return return_value;
    }

    ::EmbeddedProto::Error add(const DATA_TYPE& value) override
    {
      ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;
      if(current_length_ < MAX_LENGTH)
      {
        data_[current_length_] = value;
        ++current_length_;
      }
      else
      {
        return_value = ::EmbeddedProto::Error::ARRAY_FULL;
      }
      return return_value;
    }

    void clear() override
    {
      for(auto& element : data_)
      {
        element.clear();
      }
      current_length_ = 0U;
    }

    static constexpr uint32_t max_serialized_size(const uint32_t field_number)
    {
      return ::EmbeddedProto::RepeatedField<DATA_TYPE>::REPEATED_FIELD_IS_PACKED
               ? max_packed_serialized_size(field_number)
               : max_unpacked_serialized_size(field_number);
    }

    static constexpr uint32_t max_packed_serialized_size(const uint32_t field_number)
    {
      return ::EmbeddedProto::WireFormatter::VarintSize(
               ::EmbeddedProto::WireFormatter::MakeTag(field_number,
                                                       ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED))
        + ::EmbeddedProto::WireFormatter::VarintSize(MAX_LENGTH)
        + (MAX_LENGTH * DATA_TYPE::max_serialized_size());
    }

    static constexpr uint32_t max_unpacked_serialized_size(const uint32_t field_number)
    {
      return MAX_LENGTH * DATA_TYPE::max_serialized_size(field_number);
    }

  private:
    uint32_t current_length_ = 0U;
    std::array<DATA_TYPE, MAX_LENGTH> data_ = {};
};

} // namespace Mocks

#endif // _MOCK_REPEATED_FIELD_STORAGE_H_
