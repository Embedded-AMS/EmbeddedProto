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

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include <WireFormatter.h>
#include <Fields.h>
#include <FieldStringBytes.h>
#include <RepeatedFieldFixedSize.h>

#include <simple_types.h>

namespace test_max_field_size
{

TEST(MaxFieldSize, Wireformatter_VarintSize)
{
  EXPECT_EQ(1, EmbeddedProto::WireFormatter::VarintSize(0));
  EXPECT_EQ(1, EmbeddedProto::WireFormatter::VarintSize(1));
  EXPECT_EQ(1, EmbeddedProto::WireFormatter::VarintSize(127));
  EXPECT_EQ(2, EmbeddedProto::WireFormatter::VarintSize(128));
  EXPECT_EQ(2, EmbeddedProto::WireFormatter::VarintSize(16383));
  EXPECT_EQ(3, EmbeddedProto::WireFormatter::VarintSize(16384));
  EXPECT_EQ(3, EmbeddedProto::WireFormatter::VarintSize(2097151));
  EXPECT_EQ(4, EmbeddedProto::WireFormatter::VarintSize(2097152));
  EXPECT_EQ(4, EmbeddedProto::WireFormatter::VarintSize(268435455));
  EXPECT_EQ(5, EmbeddedProto::WireFormatter::VarintSize(268435456));
  EXPECT_EQ(5, EmbeddedProto::WireFormatter::VarintSize(34359738367));
  EXPECT_EQ(6, EmbeddedProto::WireFormatter::VarintSize(34359738368));
  EXPECT_EQ(6, EmbeddedProto::WireFormatter::VarintSize(4398046511103));
  EXPECT_EQ(7, EmbeddedProto::WireFormatter::VarintSize(4398046511104));
  EXPECT_EQ(7, EmbeddedProto::WireFormatter::VarintSize(562949953421311));
  EXPECT_EQ(8, EmbeddedProto::WireFormatter::VarintSize(562949953421312));
  EXPECT_EQ(8, EmbeddedProto::WireFormatter::VarintSize(72057594037927935));
  EXPECT_EQ(9, EmbeddedProto::WireFormatter::VarintSize(72057594037927936));
  EXPECT_EQ(9, EmbeddedProto::WireFormatter::VarintSize(9223372036854775807));

  // Max values
  EXPECT_EQ(10, EmbeddedProto::WireFormatter::VarintSize(9223372036854775808ULL));
  EXPECT_EQ(10, EmbeddedProto::WireFormatter::VarintSize(18446744073709551615ULL));
}

TEST(MaxFieldSize, Field_max_serialized_size)
{
  EXPECT_EQ(6, EmbeddedProto::int32::max_serialized_size(1));
  EXPECT_EQ(11, EmbeddedProto::int64::max_serialized_size(1));
  EXPECT_EQ(6, EmbeddedProto::uint32::max_serialized_size(1));
  EXPECT_EQ(11, EmbeddedProto::uint64::max_serialized_size(1));
  EXPECT_EQ(6, EmbeddedProto::sint32::max_serialized_size(1));
  EXPECT_EQ(11, EmbeddedProto::sint64::max_serialized_size(1));
  EXPECT_EQ(5, EmbeddedProto::fixed32::max_serialized_size(1));
  EXPECT_EQ(9, EmbeddedProto::fixed64::max_serialized_size(1));
  EXPECT_EQ(5, EmbeddedProto::sfixed32::max_serialized_size(1));
  EXPECT_EQ(9, EmbeddedProto::sfixed64::max_serialized_size(1));
  EXPECT_EQ(5, EmbeddedProto::floatfixed::max_serialized_size(1));
  EXPECT_EQ(9, EmbeddedProto::doublefixed::max_serialized_size(1));
  EXPECT_EQ(2, EmbeddedProto::boolean::max_serialized_size(1));

  // Next with a large ID such that the tag becomes multiple bytes
  EXPECT_EQ(7, EmbeddedProto::int32::max_serialized_size(16));
  EXPECT_EQ(12, EmbeddedProto::int64::max_serialized_size(16));
  EXPECT_EQ(7, EmbeddedProto::uint32::max_serialized_size(16));
  EXPECT_EQ(12, EmbeddedProto::uint64::max_serialized_size(16));
  EXPECT_EQ(7, EmbeddedProto::sint32::max_serialized_size(16));
  EXPECT_EQ(12, EmbeddedProto::sint64::max_serialized_size(16));
  EXPECT_EQ(6, EmbeddedProto::fixed32::max_serialized_size(16));
  EXPECT_EQ(10, EmbeddedProto::fixed64::max_serialized_size(16));
  EXPECT_EQ(6, EmbeddedProto::sfixed32::max_serialized_size(16));
  EXPECT_EQ(10, EmbeddedProto::sfixed64::max_serialized_size(16));
  EXPECT_EQ(6, EmbeddedProto::floatfixed::max_serialized_size(16));
  EXPECT_EQ(10, EmbeddedProto::doublefixed::max_serialized_size(16));
  EXPECT_EQ(3, EmbeddedProto::boolean::max_serialized_size(16));
} 

TEST(MaxFieldSize, SimpleTypesMsg_max_serialized_size)
{
  // First all the basic fields with tag, two enums with tag, a boolean and the length varint.
  EXPECT_EQ(3*6 + 3*11 + 3*5 + 3*9 + 2*6 + 2 + 1, ::Test_Simple_Types::max_serialized_size());

  // Include the field tag.
  EXPECT_EQ(3*6 + 3*11 + 3*5 + 3*9 + 2*6 + 2 + 1 + 1, ::Test_Simple_Types::max_serialized_size(1));
}

TEST(MaxFieldSize, FieldStringBytes_max_serialized_size)
{
  // N byte + varint for the number of bytes + tag size.
  EXPECT_EQ(1+1+1, EmbeddedProto::FieldBytes<1>::max_serialized_size(1));
  EXPECT_EQ(15+1+1, EmbeddedProto::FieldBytes<15>::max_serialized_size(1));
  EXPECT_EQ(127+1+1, EmbeddedProto::FieldBytes<127>::max_serialized_size(1));
  EXPECT_EQ(128+2+1, EmbeddedProto::FieldBytes<128>::max_serialized_size(1));
}

TEST(MaxFieldSize, RepeatedFieldFixedSize_Packed)
{
  constexpr uint32_t max_ser_size_A = EmbeddedProto::RepeatedFieldFixedSize<EmbeddedProto::uint32, 3>::max_serialized_size(1);
  constexpr uint32_t max_ser_size_B = EmbeddedProto::RepeatedFieldFixedSize<EmbeddedProto::uint32, 3>::max_serialized_size(16);
  constexpr uint32_t max_ser_size_C = EmbeddedProto::RepeatedFieldFixedSize<EmbeddedProto::uint32, 128>::max_serialized_size(1);
  // id + size + data
  EXPECT_EQ(1 + 1 + (3*5), max_ser_size_A);
  EXPECT_EQ(2 + 1 + (3*5), max_ser_size_B);
  EXPECT_EQ(1 + 2 + (128*5), max_ser_size_C);
}

TEST(MaxFieldSize, RepeatedFieldFixedSize_Unpacked)
{
  constexpr uint32_t max_ser_size_A = EmbeddedProto::RepeatedFieldFixedSize<Test_Simple_Types, 3>::max_serialized_size(1);
  constexpr uint32_t max_ser_size_B = EmbeddedProto::RepeatedFieldFixedSize<Test_Simple_Types, 3>::max_serialized_size(16);
  
  // N times id + data
  EXPECT_EQ((3*(1 + (3*6 + 3*11 + 3*5 + 3*9 + 2*6 + 2 + 1))), max_ser_size_A);
  EXPECT_EQ((3*(2 + (3*6 + 3*11 + 3*5 + 3*9 + 2*6 + 2 + 1))), max_ser_size_B);
}

} // End of namespace test_max_field_size
