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
#include <WriteBufferFixedSize.h>

#include <simple_types.h>
#include <oneof_fields.h>

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

template<class FIELD_TYPE, class BASE_TYPE>
void helper_test(const BASE_TYPE max)
{
  EmbeddedProto::WriteBufferFixedSize<10> buffer;
  //constexpr BASE_TYPE max = std::numeric_limits<BASE_TYPE>::digits == 64 ? static_cast<BASE_TYPE>(18446744073709551615) : static_cast<BASE_TYPE>(4294967295);
  FIELD_TYPE field = max; // std::numeric_limits<BASE_TYPE>::max();
  field.serialize(buffer);
  EXPECT_EQ(buffer.get_size(), FIELD_TYPE::max_serialized_size());
}

void helper_set_most_consuming_value(Test_Simple_Types& msg)
{
  msg.set_a_int32(std::numeric_limits<int32_t>::max());  
  msg.set_a_int64(-9223372036854775808);     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::max());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sint32(std::numeric_limits<int32_t>::max());
  msg.set_a_sint64(std::numeric_limits<int64_t>::max());
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::TWOBILLION);
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::max());
  msg.set_a_double(std::numeric_limits<double>::max());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::max());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::max()); 
  msg.set_a_float(std::numeric_limits<float>::max());

  msg.set_a_nested_enum(Test_Simple_Types::Nested_Enum::NE_C);
}

TEST(MaxFieldSize, SimpleTypesMsg_max_serialized_size)
{
  helper_test<EmbeddedProto::int32, int32_t>(2147483647);
  helper_test<EmbeddedProto::int64, int64_t>(-9223372036854775808);
  helper_test<EmbeddedProto::uint32, uint32_t>(4294967295);
  helper_test<EmbeddedProto::uint64, uint64_t>(18446744073709551615ull);
  helper_test<EmbeddedProto::sint32, int32_t>(2147483647);
  helper_test<EmbeddedProto::sint64, int64_t>(9223372036854775807);
  helper_test<EmbeddedProto::fixed32, uint32_t>(4294967295);
  helper_test<EmbeddedProto::fixed64, uint64_t>(18446744073709551615ull);
  helper_test<EmbeddedProto::sfixed32, int32_t>(2147483647);
  helper_test<EmbeddedProto::sfixed64, int64_t>(9223372036854775807);
  helper_test<EmbeddedProto::floatfixed, float>(std::numeric_limits<float>::max());
  helper_test<EmbeddedProto::doublefixed, double>(std::numeric_limits<double>::max());
  helper_test<EmbeddedProto::boolean, bool>(true);

  EmbeddedProto::WriteBufferFixedSize<150> buffer;
  Test_Simple_Types msg;
  helper_set_most_consuming_value(msg);

  msg.serialize(buffer);
  EXPECT_EQ((Test_Simple_Types::max_serialized_size()), buffer.get_size());

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
  EmbeddedProto::RepeatedFieldFixedSize<Test_Simple_Types, 3> rffs;
  helper_set_most_consuming_value(rffs[0]);
  helper_set_most_consuming_value(rffs[1]);
  helper_set_most_consuming_value(rffs[2]);

  EmbeddedProto::WriteBufferFixedSize<500> buffer;

  constexpr uint32_t max_ser_size_A = EmbeddedProto::RepeatedFieldFixedSize<Test_Simple_Types, 3>::max_serialized_size(1);
  rffs.serialize_with_id(1, buffer, false);
  EXPECT_EQ(buffer.get_size(), max_ser_size_A);

  buffer.clear();
  constexpr uint32_t max_ser_size_B = EmbeddedProto::RepeatedFieldFixedSize<Test_Simple_Types, 3>::max_serialized_size(16);
  rffs.serialize_with_id(16, buffer, false);
  EXPECT_EQ(buffer.get_size(), max_ser_size_B);
}

TEST(MaxFieldSize, OneofMaxFieldSize)
{
  message_oneof msg;
  msg.set_a(std::numeric_limits<int32_t>::max());
  msg.set_x(std::numeric_limits<int32_t>::max());
  msg.set_b(std::numeric_limits<int32_t>::max());
  msg.set_w(std::numeric_limits<float>::max());
  msg.mutable_msg_ABC().set_varA(std::numeric_limits<int32_t>::max());
  msg.mutable_msg_ABC().set_varB(std::numeric_limits<int32_t>::max());
  msg.mutable_msg_ABC().set_varC(std::numeric_limits<int32_t>::max());

  EmbeddedProto::WriteBufferFixedSize<200> buffer;

  msg.serialize(buffer);
  EXPECT_EQ(buffer.get_size(), message_oneof::max_serialized_size());

  buffer.clear();
  constexpr uint32_t N_CHARS = 25+1;
  string_bytes_oneof<N_CHARS, 1> msg_chars;
  msg_chars.mutable_name().set("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  msg_chars.serialize(buffer);
  EXPECT_EQ(buffer.get_size(), (string_bytes_oneof<N_CHARS,1>::max_serialized_size()));
  
  buffer.clear();
  constexpr uint32_t N_BYTES = 75;
  string_bytes_oneof<1, N_BYTES> msg_bytes;
  for(uint32_t i = 0; i < N_BYTES; ++i) { msg_bytes.mutable_data()[i] = 0xFF; }
  msg_bytes.serialize(buffer);  
  EXPECT_EQ(buffer.get_size(), (string_bytes_oneof<1,N_BYTES>::max_serialized_size()));

  buffer.clear();
  combined_oneof<1,1> msg_combiA;
  msg_combiA.mutable_msg_oneof() = msg;
  msg_combiA.serialize(buffer);
  EXPECT_EQ(buffer.get_size(), (combined_oneof<1,1>::max_serialized_size()));

  buffer.clear();
  combined_oneof<1, N_BYTES> msg_combiB;
  msg_combiB.mutable_msg_sb_oneof() = msg_bytes; 
  msg_combiB.serialize(buffer);
  EXPECT_EQ(buffer.get_size(), (combined_oneof<1,N_BYTES>::max_serialized_size()));
}

} // End of namespace test_max_field_size