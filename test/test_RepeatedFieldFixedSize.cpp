/*
 *  Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
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
 *  <https://EmbeddedProto.com/license/>.
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

#include <gtest/gtest.h>

#include <Fields.h>
#include <RepeatedFieldFixedSize.h>

namespace test_EmbeddedAMS_RepeatedFieldFixedSize
{

static constexpr int32_t UINT32_SIZE = sizeof(::EmbeddedProto::uint32);

TEST(RepeatedFieldFixedSize, construction) 
{
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  // Copy to the same size
  x.add(1);
  x.add(2);
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> y(x);
  EXPECT_EQ(2, y.get_length());
  EXPECT_EQ(1U, y.get_const(0));
  EXPECT_EQ(2U, y.get_const(1));

  // Copy to a larger array
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 2*LENGTH> z(x);
  

}

TEST(RepeatedFieldFixedSize, size_uint32_t) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  auto size = x.get_size();
  EXPECT_EQ(0, size);

  auto max_size = x.get_max_size();
  EXPECT_EQ(LENGTH*UINT32_SIZE, max_size);

  auto length = x.get_length();
  EXPECT_EQ(0, length);

  auto max_length = x.get_max_length();
  EXPECT_EQ(LENGTH, max_length);
}

TEST(RepeatedFieldFixedSize, add_data) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  x.add(1);
  x.add(2);
  EXPECT_EQ(2*UINT32_SIZE, x.get_size());
  EXPECT_EQ(2, x.get_length());

  auto result = x.add(3);
  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, result);

  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_size());
  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_max_size());
  EXPECT_EQ(LENGTH, x.get_length());
  EXPECT_EQ(LENGTH, x.get_max_length());

  // Check if we can add more than the limit.
  result = x.add(4);
  EXPECT_EQ(EmbeddedProto::Error::ARRAY_FULL, result);
}

TEST(RepeatedFieldFixedSize, get) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  // Get (non-const) should modify the number of items in the array.

  // Set the second element
  x.get(1) = 1;
  EXPECT_EQ(2, x.get_length());
  EXPECT_EQ(0U, x.get_const(0));
  EXPECT_EQ(1U, x.get_const(1));

  // When going out of bound the last element should be returned.
  EXPECT_EQ(0U, x.get_const(2));
  x.get(3) = 3U;
  EXPECT_EQ(3U, x.get_const(2));
  EXPECT_EQ(3U, x.get_const(3));

  // Test the get_const which will return out of bound errors.
  ::EmbeddedProto::uint32 value = 0;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, x.get_const(1, value));
  EXPECT_EQ(1, value);

  value = 99;
  EXPECT_EQ(::EmbeddedProto::Error::INDEX_OUT_OF_BOUND, x.get_const(3, value));
  // Value should not have changed.
  EXPECT_EQ(99, value);
}

TEST(RepeatedFieldFixedSize, set_data_array) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  EmbeddedProto::uint32 data3[] = {1, 2, 3};

  auto result = x.set_data(&(data3[0]), 3U);
  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, result);  

  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_size());
  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_max_size());
  EXPECT_EQ(LENGTH, x.get_length());
  EXPECT_EQ(LENGTH, x.get_max_length());

  // Check if we can add more than the limit.
  EmbeddedProto::uint32 data4[] = {1, 2, 3, 4};
  result = x.set_data(&(data4[0]), 4U);
  EXPECT_EQ(EmbeddedProto::Error::ARRAY_FULL, result);
}

TEST(RepeatedFieldFixedSize, set_element) 
{
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  // First add a value in the middle and see if we have a size of two.
  x.set(1, 2);
  EXPECT_EQ(2U, x.get_const(1));
  EXPECT_EQ(2U, x.get_length());

  x.set(0, 1);
  EXPECT_EQ(1U, x.get_const(0));

  x.set(2, 3);
  EXPECT_EQ(3U, x.get_const(2));
}

TEST(RepeatedFieldFixedSize, clear) 
{
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;
  x.add(1);
  x.add(2);
  x.clear();
  EXPECT_EQ(0U, x.get_const(0));
  EXPECT_EQ(0U, x.get_const(1));
  EXPECT_EQ(0U, x.get_length());
}


} // End namespace test_EmbeddedAMS_RepeatedField
