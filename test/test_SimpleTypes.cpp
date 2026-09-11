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

#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>
#include <MessageState.h>

#include <cstdint>    
#include <limits>
#include <string.h>

// EAMS message definitions
#include <simple_types.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_SimpleTypes 
{

TEST(SimpleTypes, zero) 
{
  InSequence s;

  // See if an empty message results in no data been pushed.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}


TEST(SimpleTypes, mutable_enum)
{
  // Enum fields expose a mutable reference like the other scalar fields do.
  ::Test_Simple_Types msg;
  msg.mutable_a_enum() = Test_Enum::TWO;
  EXPECT_EQ(Test_Enum::TWO, msg.get_a_enum());
  msg.mutable_a_nested_enum() = ::Test_Simple_Types::Nested_Enum::NE_B;
  EXPECT_EQ(::Test_Simple_Types::Nested_Enum::NE_B, msg.get_a_nested_enum());
}

#if EMBEDDED_PROTO_LITTLE_ENDIAN
TEST(SimpleTypes, serialize_fixed_is_one_block_push)
{
  InSequence s;

  ::Test_Simple_Types msg;
  msg.set_a_fixed32(1);
  msg.set_a_float(1.0F);

  Mocks::WriteBufferMock buffer;
  // Tag byte by byte, value as one block, for both fixed32 and float.
  EXPECT_CALL(buffer, push(0x65)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(_, 4U)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x75)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(_, 4U)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}
#endif // EMBEDDED_PROTO_LITTLE_ENDIAN

TEST(SimpleTypes, serialize_one) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B);

  std::array<uint8_t, 60> expected = {0x08, 0x01, 
                                      0x10, 0x01, 
                                      0x18, 0x01, 
                                      0x20, 0x01, 
                                      0x28, 0x02, 
                                      0x30, 0x02, 
                                      0x38, 0x01, 
                                      0x40, 0x01, 
                                      0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                      0x65, 0x01, 0x00, 0x00, 0x00, 
                                      0x6d, 0x01, 0x00, 0x00, 0x00, 
                                      0x75, 0x00, 0x00, 0x80, 0x3f,
                                      0x78, 0x01};

  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(60, msg.serialized_size());
}

TEST(SimpleTypes, serialize_max) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_int32(std::numeric_limits<int32_t>::max());   
  msg.set_a_int64(std::numeric_limits<int64_t>::max());     
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


  std::array<uint8_t, 100> expected = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
                                        0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                        0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                        0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                        0x28, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 
                                        0x30, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                        0x38, 0x01, 
                                        0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07,
                                        0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                        0x51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                        0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, 
                                        0x65, 0xFF, 0xFF, 0xFF, 0xFF, 
                                        0x6D, 0xFF, 0xFF, 0xFF, 0x7F, 
                                        0x75, 0xFF, 0xFF, 0x7F, 0x7F };
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(100, msg.serialized_size());
}

TEST(SimpleTypes, serialize_min) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_int32(std::numeric_limits<int32_t>::min());   
  msg.set_a_int64(std::numeric_limits<int64_t>::min());     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::min());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::min());
  msg.set_a_sint32(std::numeric_limits<int32_t>::min());
  msg.set_a_sint64(std::numeric_limits<int64_t>::min());
  msg.set_a_bool(false);
  msg.set_a_enum(Test_Enum::ZERO);
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::min());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::min());
  msg.set_a_double(std::numeric_limits<double>::lowest());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::min());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::min()); 
  msg.set_a_float(std::numeric_limits<float>::lowest());

  std::array<uint8_t, 62> expected = {0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                                      0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                                      0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                      0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                                      0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
                                      0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 
                                      0x6D, 0x00, 0x00, 0x00, 0x80, 
                                      0x75, 0xFF, 0xFF, 0x7F, 0xFF};
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(62, msg.serialized_size());
}

TEST(SimpleTypes, serialize_smalest_real) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_double(std::numeric_limits<double>::min());
  msg.set_a_float(std::numeric_limits<float>::min());


  std::array<uint8_t, 14>  expected = { 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 
                                        0x75, 0x00, 0x00, 0x80, 0x00 };
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(14, msg.serialized_size());
}

TEST(SimpleTypes, serialize_fault_buffer_full_varint)
{
    InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  // Just set some large value.
  msg.set_a_uint32(std::numeric_limits<uint32_t>::max());

  // Allow for some bytes to be serialized
  EXPECT_CALL(buffer, push(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(_)).Times(1).WillOnce(Return(true));  
  // And then fail
  EXPECT_CALL(buffer, push(_)).Times(2).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, msg.serialize(buffer));

}

TEST(SimpleTypes, deserialize_zero) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  ::Test_Simple_Types msg;

  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  EXPECT_EQ(0, msg.get_a_int32());   
  EXPECT_EQ(0, msg.get_a_int64());     
  EXPECT_EQ(0U, msg.get_a_uint32());    
  EXPECT_EQ(0U, msg.get_a_uint64());
  EXPECT_EQ(0, msg.get_a_sint32());
  EXPECT_EQ(0, msg.get_a_sint64());
  EXPECT_EQ(false, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ZERO, msg.get_a_enum());
  EXPECT_EQ(0U, msg.get_a_fixed64());
  EXPECT_EQ(0, msg.get_a_sfixed64());
  EXPECT_EQ(0.0, msg.get_a_double());
  EXPECT_EQ(0U, msg.get_a_fixed32());
  EXPECT_EQ(0, msg.get_a_sfixed32()); 
  EXPECT_EQ(0.0F, msg.get_a_float());
}

TEST(SimpleTypes, deserialize_one) 
{  
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<58> buffer(
                                    { 0x08, 0x01, 
                                      0x10, 0x01, 
                                      0x18, 0x01, 
                                      0x20, 0x01, 
                                      0x28, 0x02, 
                                      0x30, 0x02, 
                                      0x38, 0x01, 
                                      0x40, 0x01, 
                                      0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                      0x65, 0x01, 0x00, 0x00, 0x00, 
                                      0x6d, 0x01, 0x00, 0x00, 0x00, 
                                      0x75, 0x00, 0x00, 0x80, 0x3f} );
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a_int32());   
  EXPECT_EQ(1, msg.get_a_int64());     
  EXPECT_EQ(1U, msg.get_a_uint32());    
  EXPECT_EQ(1U, msg.get_a_uint64());
  EXPECT_EQ(1, msg.get_a_sint32());
  EXPECT_EQ(1, msg.get_a_sint64());
  EXPECT_EQ(true, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ONE, msg.get_a_enum());
  EXPECT_EQ(1U, msg.get_a_fixed64());
  EXPECT_EQ(1, msg.get_a_sfixed64());
  EXPECT_EQ(1.0, msg.get_a_double());
  EXPECT_EQ(1U, msg.get_a_fixed32());
  EXPECT_EQ(1, msg.get_a_sfixed32()); 
  EXPECT_EQ(1.0F, msg.get_a_float());
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

TEST(SimpleTypes, deserialize_one_partial_clean)
{
  ::EmbeddedProto::ReadBufferFixedSize<75> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // Setup the first part of the test data
  constexpr uint32_t Na = 12;
  std::array<uint8_t, Na>  refereeA = {  0x08, 0x01, 
                                         0x10, 0x01, 
                                         0x18, 0x01, 
                                         0x20, 0x01, 
                                         0x28, 0x02, 
                                         0x30, 0x02};
  for(const auto& a: refereeA){ buffer.push(a); }

  // Deserialize the first part. We expect that we reached the end of the buffer an need more data.
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  // Setup the second part of the test data.
  constexpr uint32_t Nb = 46;
  std::array<uint8_t, Nb>  refereeB = {  0x38, 0x01, 
                                         0x40, 0x01, 
                                         0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                         0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                         0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                         0x65, 0x01, 0x00, 0x00, 0x00, 
                                         0x6d, 0x01, 0x00, 0x00, 0x00, 
                                         0x75, 0x00, 0x00, 0x80, 0x3f};
  for(const auto& b: refereeB){ buffer.push(b); }

  // Deserialize the second part.
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(1, msg.get_a_int32());
  EXPECT_EQ(1, msg.get_a_int64());
  EXPECT_EQ(1U, msg.get_a_uint32());
  EXPECT_EQ(1U, msg.get_a_uint64());
  EXPECT_EQ(1, msg.get_a_sint32());
  EXPECT_EQ(1, msg.get_a_sint64());
  EXPECT_EQ(true, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ONE, msg.get_a_enum());
  EXPECT_EQ(1U, msg.get_a_fixed64());
  EXPECT_EQ(1, msg.get_a_sfixed64());
  EXPECT_EQ(1.0, msg.get_a_double());
  EXPECT_EQ(1U, msg.get_a_fixed32());
  EXPECT_EQ(1, msg.get_a_sfixed32());
  EXPECT_EQ(1.0F, msg.get_a_float());
}

TEST(SimpleTypes, deserialize_one_partial_halfway_through_field)
{
  ::EmbeddedProto::ReadBufferFixedSize<75> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // Setup the first part of the test data
  constexpr uint32_t Na = 20;
  std::array<uint8_t, Na>  refereeA = {  0x08, 0x01, 
                                         0x10, 0x01, 
                                         0x18, 0x01, 
                                         0x20, 0x01, 
                                         0x28, 0x02, 
                                         0x30, 0x02,
                                         0x38, 0x01, 
                                         0x40, 0x01, 
                                         0x49, 0x01, 0x00, 0x00 }; // This field is not fished. 
                                         //  The three bytes data bytes should remain in the
                                         // buffer as the tag is remembered.
  for(const auto& a: refereeA){ buffer.push(a); }

  // Deserialize the first part. We expect that we reached the end of the buffer an need more data.
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  // Setup the second part of the test data.
  constexpr uint32_t Nb = 38;
  std::array<uint8_t, Nb>  refereeB = {                    0x00, 0x00, 0x00, 0x00, 0x00, // No tag and the three data bytes are remembered.
                                         0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                         0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                         0x65, 0x01, 0x00, 0x00, 0x00, 
                                         0x6d, 0x01, 0x00, 0x00, 0x00, 
                                         0x75, 0x00, 0x00, 0x80, 0x3f};
  for(const auto& b: refereeB){ buffer.push(b); }

  // Deserialize the second part.
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(1, msg.get_a_int32());
  EXPECT_EQ(1, msg.get_a_int64());
  EXPECT_EQ(1U, msg.get_a_uint32());
  EXPECT_EQ(1U, msg.get_a_uint64());
  EXPECT_EQ(1, msg.get_a_sint32());
  EXPECT_EQ(1, msg.get_a_sint64());
  EXPECT_EQ(true, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ONE, msg.get_a_enum());
  EXPECT_EQ(1U, msg.get_a_fixed64());
  EXPECT_EQ(1, msg.get_a_sfixed64());
  EXPECT_EQ(1.0, msg.get_a_double());
  EXPECT_EQ(1U, msg.get_a_fixed32());
  EXPECT_EQ(1, msg.get_a_sfixed32());
  EXPECT_EQ(1.0F, msg.get_a_float());
}

TEST(SimpleTypes, PartialDeserialize_VarintSplitInData)
{
  ::EmbeddedProto::ReadBufferFixedSize<10> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // a_int64 = 300 -> tag 0x10, value 0xAC 0x02
  buffer.push(0x10);
  buffer.push(0xAC);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  // Continue varint value without repeating the tag.
  buffer.push(0x02);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(300, msg.get_a_int64());
}

TEST(SimpleTypes, PartialDeserialize_Fixed32SplitInData)
{
  ::EmbeddedProto::ReadBufferFixedSize<10> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // a_fixed32 = 1 -> tag 0x65 followed by 4 bytes.
  buffer.push(0x65);
  buffer.push(0x01);
  buffer.push(0x00);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  // Continue fixed-width payload without repeating the tag.
  buffer.push(0x00);
  buffer.push(0x00);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(1U, msg.get_a_fixed32());
}

TEST(SimpleTypes, PartialDeserialize_EnumLargeValue_SplitInData)
{
  ::EmbeddedProto::ReadBufferFixedSize<10> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // a_enum = TWOBILLION -> tag 0x40 + value 0x80 0xA8 0xD6 0xB9 0x07
  buffer.push(0x40);
  buffer.push(0x80);
  buffer.push(0xA8);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  // Continue enum varint payload without repeating the tag.
  buffer.push(0xD6);
  buffer.push(0xB9);
  buffer.push(0x07);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(Test_Enum::TWOBILLION, msg.get_a_enum());
}

TEST(SimpleTypes, PartialDeserialize_TagOnlyThenValue)
{
  ::EmbeddedProto::ReadBufferFixedSize<10> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // a_bool tag only.
  buffer.push(0x38);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  // Continue with only the value.
  buffer.push(0x01);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_TRUE(msg.get_a_bool());
}

TEST(SimpleTypes, PartialDeserialize_InvalidFieldId)
{
  // A tag that decodes to field number 0 is invalid protobuf.
  ::EmbeddedProto::ReadBufferFixedSize<2> buffer({0x00});
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  EXPECT_EQ(::EmbeddedProto::Error::INVALID_FIELD_ID, msg.deserialize_partial(buffer, state.root()));
}

TEST(SimpleTypes, PartialDeserialize_InvalidWireType)
{
  // a_int32 (field 1) expects a VARINT, but here it is tagged as FIXED32
  // (tag = (1 << 3) | 5 = 0x0D).
  ::EmbeddedProto::ReadBufferFixedSize<5> buffer({0x0D, 0x00, 0x00, 0x00, 0x00});
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize_partial(buffer, state.root()));
}

#endif // PARTIAL_SERIALIZATION_ENABLED

TEST(SimpleTypes, deserialize_10_byte_int32)
{
  // Some implementations serialize 32bit integers in 10 bytes instead of the theoretical 5.
  // Data in the higher bytes should be ignored.
  
  constexpr uint32_t N_BYTES = 11;
 
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<N_BYTES> buffer( {0x08, 0x81, 0x80, 0x80, 0x80, 0x80, 0x8F,
                                                         0x8F, 0x8F, 0x8F, 0x0F } );
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a_int32());   
}

TEST(SimpleTypes, deserialize_max) 
{
  ::Test_Simple_Types msg;

  constexpr uint32_t N_BYTES = 100;
  ::EmbeddedProto::ReadBufferFixedSize<N_BYTES> buffer(
                                         { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
                                           0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                           0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                           0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                           0x28, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 
                                           0x30, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                           0x38, 0x01, 
                                           0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07,
                                           0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                           0x51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                           0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, 
                                           0x65, 0xFF, 0xFF, 0xFF, 0xFF, 
                                           0x6D, 0xFF, 0xFF, 0xFF, 0x7F, 
                                           0x75, 0xFF, 0xFF, 0x7F, 0x7F } );
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(),  msg.get_a_int32());   
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),  msg.get_a_int64());     
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());    
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), msg.get_a_uint64());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(),  msg.get_a_sint32());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),  msg.get_a_sint64());
  EXPECT_EQ(true, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::TWOBILLION, msg.get_a_enum());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), msg.get_a_fixed64());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),  msg.get_a_sfixed64());
  EXPECT_EQ(std::numeric_limits<double>::max(),   msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_fixed32());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(),  msg.get_a_sfixed32()); 
  EXPECT_EQ(std::numeric_limits<float>::max(),    msg.get_a_float());
}

TEST(SimpleTypes, deserialize_min) 
{
  ::Test_Simple_Types msg;

  constexpr uint32_t N_BYTES = 62;
  ::EmbeddedProto::ReadBufferFixedSize<N_BYTES> buffer(
                                          { 0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                                            0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                                            0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                            0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                                            0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
                                            0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 
                                            0x6D, 0x00, 0x00, 0x00, 0x80, 
                                            0x75, 0xFF, 0xFF, 0x7F, 0xFF } );

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::min(),  msg.get_a_int32());   
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),  msg.get_a_int64());     
  EXPECT_EQ(std::numeric_limits<uint32_t>::min(), msg.get_a_uint32());    
  EXPECT_EQ(std::numeric_limits<uint64_t>::min(), msg.get_a_uint64());
  EXPECT_EQ(std::numeric_limits<int32_t>::min(),  msg.get_a_sint32());
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),  msg.get_a_sint64());
  EXPECT_EQ(false, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ZERO, msg.get_a_enum());
  EXPECT_EQ(std::numeric_limits<uint64_t>::min(),  msg.get_a_fixed64());
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),   msg.get_a_sfixed64());
  EXPECT_EQ(std::numeric_limits<double>::lowest(), msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<uint32_t>::min(),  msg.get_a_fixed32());
  EXPECT_EQ(std::numeric_limits<int32_t>::min(),   msg.get_a_sfixed32()); 
  EXPECT_EQ(std::numeric_limits<float>::lowest(),  msg.get_a_float());
}

TEST(SimpleTypes, deserialize_smalest_real) 
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<14> buffer({ 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 
                                                    0x75, 0x00, 0x00, 0x80, 0x00 });
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<double>::min(), msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<float>::min(),  msg.get_a_float());
}

TEST(SimpleTypes, deserialize_fault_end_of_buffer_fixed)
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(58));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 3> referee = {0x49, 0xFF, 0xFF}; // End half way through a fixed size value.

  for(auto r: referee) {
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
}

TEST(SimpleTypes, deserialize_fault_end_of_buffer_bool)
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<58> buffer( {0x38} ); // Just the tag of a bool but no data

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
}

TEST(SimpleTypes, deserialize_enum_beond_range)
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(2));

  ::Test_Simple_Types msg;

  // This enum value is beond the range known to this code. Decodation should not fail. The value
  // should however not match to any of the known enum values.
  std::array<uint8_t, 2> referee = {0x78, 0x03}; 

  for(auto r: referee) {
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_NE(::Test_Simple_Types::Nested_Enum::NE_A, msg.get_a_nested_enum());
  EXPECT_NE(::Test_Simple_Types::Nested_Enum::NE_B, msg.get_a_nested_enum());
  EXPECT_NE(::Test_Simple_Types::Nested_Enum::NE_C, msg.get_a_nested_enum());

}

TEST(SimpleTypes, deserialize_fault_overlong_varint)
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<11> buffer( 
      { 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } // Invalid closing byte for a_int64
    );

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.deserialize(buffer));
}

#ifndef DISABLE_FIELD_NUMBER_TO_NAME 

TEST(SimpleTypes, field_number_to_name)
{
  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(::Test_Simple_Types::FieldNumber::A_INT32),
                          "a_int32"));

  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(::Test_Simple_Types::FieldNumber::A_NESTED_ENUM),
                          "a_nested_enum"));

  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(static_cast<::Test_Simple_Types::FieldNumber>(0)),
                          "Invalid FieldNumber"));

  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(static_cast<::Test_Simple_Types::FieldNumber>(99)),
                          "Invalid FieldNumber"));
}

#endif

#ifdef MSG_TO_STRING

TEST(SimpleTypes, to_string)
{
  constexpr uint32_t N = 1024;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };
 
  ::Test_Simple_Types msg;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B); 

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);

  // std::cout << std::endl << str << std::endl;
  
  constexpr uint32_t TXT_LEN = 278;
  const char expected_str[TXT_LEN + 1] = "{\n  \"a_int32\": 1,\n  \"a_int64\": 1,\n  \"a_uint32\": 1,\n  \"a_uint64\": 1,\n  \"a_sint32\": 1,\n  \"a_sint64\": 1,\n  \"a_bool\": true,\n  \"a_enum\": 1,\n  \"a_fixed64\": 1,\n  \"a_sfixed64\": 1,\n  \"a_double\": 1.000000,\n  \"a_fixed32\": 1,\n  \"a_sfixed32\": 1,\n  \"a_float\": 1.000000,\n  \"a_nested_enum\": 1\n}";
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);
  EXPECT_EQ(str + TXT_LEN, str_left.data);  

}

TEST(SimpleTypes, to_string_buffer_overrun)
{
  constexpr uint32_t N = 100;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };
 
  ::Test_Simple_Types msg;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B); 

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);

  EXPECT_EQ(0, str_left.size);
  EXPECT_EQ(str + N, str_left.data);  

}

#endif // MSG_TO_STRING

//==============================================================================
// Partial Serialization Tests
//==============================================================================


#ifdef PARTIAL_SERIALIZATION_ENABLED
TEST(SimpleTypes, PartialSerialize_SingleVarintField_SufficientBuffer)
{
  // Test 14.3.1: Single varint field with sufficient buffer
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());
  EXPECT_EQ(0x08, buffer.get_data()[0]);
  EXPECT_EQ(0x01, buffer.get_data()[1]);
}

TEST(SimpleTypes, PartialSerialize_SingleFixed64Field_SufficientBuffer)
{
  // Test 14.3.2: Single fixed64 field (9 bytes: 1 tag + 8 data)
  ::Test_Simple_Types msg;
  msg.set_a_fixed64(1);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, buffer.get_size());
  
  std::array<uint8_t, 9> expected = {0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(SimpleTypes, PartialSerialize_AllFieldsOne_LargeBuffer)
{
  // Test 14.3.3: Verify serialize_partial() produces identical output to serialize()
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B);

  ::EmbeddedProto::WriteBufferFixedSize<100> buffer;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(60U, buffer.get_size());

  std::array<uint8_t, 60> expected = {0x08, 0x01, 
                                      0x10, 0x01, 
                                      0x18, 0x01, 
                                      0x20, 0x01, 
                                      0x28, 0x02, 
                                      0x30, 0x02, 
                                      0x38, 0x01, 
                                      0x40, 0x01, 
                                      0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                      0x65, 0x01, 0x00, 0x00, 0x00, 
                                      0x6d, 0x01, 0x00, 0x00, 0x00, 
                                      0x75, 0x00, 0x00, 0x80, 0x3f,
                                      0x78, 0x01};

  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(SimpleTypes, PartialSerialize_AllFieldsOne_TwoBuffers_CleanSplit)
{
  // Test 14.3.4: Split across two buffers at clean field boundary
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B);

  ::Test_Simple_Types::StateStack state;
  
  // First buffer - fits first 8 varint fields (16 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<16> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(16U, bufferA.get_size());

  std::array<uint8_t, 16> expectedA = {0x08, 0x01, 0x10, 0x01, 0x18, 0x01, 0x20, 0x01, 
                                       0x28, 0x02, 0x30, 0x02, 0x38, 0x01, 0x40, 0x01};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Buffer A mismatch at byte " << i;
  }

  // Second buffer - fits remaining fields
  ::EmbeddedProto::WriteBufferFixedSize<50> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(44U, bufferB.get_size());
}

TEST(SimpleTypes, PartialSerialize_Fixed64_BufferTooSmall_Rollback)
{
  // Test 14.3.5: Fixed64 field requires 9 bytes - buffer too small triggers rollback
  ::Test_Simple_Types msg;
  msg.set_a_fixed64(1);

  ::Test_Simple_Types::StateStack state;

  // Buffer too small for 9 bytes (tag + data)
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  // Nothing should be written if tag+value cannot fit atomically
  EXPECT_EQ(0U, bufferA.get_size());

  // Second buffer should succeed with sufficient space
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, bufferB.get_size());
}

TEST(SimpleTypes, PartialSerialize_LargeVarint_BufferTooSmall_Rollback)
{
  // Test 14.3.6: Large varint (UINT64_MAX requires 11 bytes) with buffer too small
  ::Test_Simple_Types msg;
  msg.set_a_uint64(std::numeric_limits<uint64_t>::max());

  ::Test_Simple_Types::StateStack state;

  // Buffer too small for 11 bytes
  ::EmbeddedProto::WriteBufferFixedSize<6> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(0U, bufferA.get_size());

  // Second buffer with sufficient space
  ::EmbeddedProto::WriteBufferFixedSize<15> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  // Field 4 (a_uint64): tag 0x20 + 10 byte varint = 11 bytes
  EXPECT_EQ(11U, bufferB.get_size());
}

TEST(SimpleTypes, PartialSerialize_TwoVarintFields_SplitBetweenFields)
{
  // Test 14.3.7: Split happens cleanly between two varint fields
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);  // 2 bytes
  msg.set_a_int64(1);  // 2 bytes

  ::Test_Simple_Types::StateStack state;

  // Buffer exactly fits first field
  ::EmbeddedProto::WriteBufferFixedSize<2> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, bufferA.get_size());
  EXPECT_EQ(0x08, bufferA.get_data()[0]);
  EXPECT_EQ(0x01, bufferA.get_data()[1]);

  // Second buffer for second field
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, bufferB.get_size());
  EXPECT_EQ(0x10, bufferB.get_data()[0]);
  EXPECT_EQ(0x01, bufferB.get_data()[1]);
}

TEST(SimpleTypes, PartialSerialize_TwoVarintFields_SecondDoesNotFit)
{
  // Test 14.3.8: First field fits but second does not (rollback second)
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);  // 2 bytes
  msg.set_a_int64(1);  // 2 bytes

  ::Test_Simple_Types::StateStack state;

  // Buffer has 3 bytes: first field fits (2), but 1 byte left is not enough for second
  ::EmbeddedProto::WriteBufferFixedSize<3> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, bufferA.get_size());  // Only first field written
  EXPECT_EQ(0x08, bufferA.get_data()[0]);
  EXPECT_EQ(0x01, bufferA.get_data()[1]);

  // Second buffer for second field
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, bufferB.get_size());
  EXPECT_EQ(0x10, bufferB.get_data()[0]);
  EXPECT_EQ(0x01, bufferB.get_data()[1]);
}

TEST(SimpleTypes, PartialSerialize_LoopSmallBuffers)
{
  // Test 14.3.9: Serialize using many small buffers in a loop
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B);

  std::array<uint8_t, 100> collected_data = {0};
  uint32_t total_bytes = 0;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t iterations = 0;
  constexpr uint32_t MAX_ITERATIONS = 20;

  while((::EmbeddedProto::Error::BUFFER_FULL == result) && (iterations < MAX_ITERATIONS))
  {
    ::EmbeddedProto::WriteBufferFixedSize<12> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    // Copy to collected buffer
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    ++iterations;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(60U, total_bytes);
  EXPECT_LT(iterations, MAX_ITERATIONS);

  // Verify collected data matches expected
  std::array<uint8_t, 60> expected = {0x08, 0x01, 
                                      0x10, 0x01, 
                                      0x18, 0x01, 
                                      0x20, 0x01, 
                                      0x28, 0x02, 
                                      0x30, 0x02, 
                                      0x38, 0x01, 
                                      0x40, 0x01, 
                                      0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                      0x65, 0x01, 0x00, 0x00, 0x00, 
                                      0x6d, 0x01, 0x00, 0x00, 0x00, 
                                      0x75, 0x00, 0x00, 0x80, 0x3f,
                                      0x78, 0x01};

  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], collected_data[i]) << "Mismatch at byte " << i;
  }
}

TEST(SimpleTypes, PartialSerialize_AllFieldsMax_LoopSmallBuffers)
{
  // Test 14.3.10: Maximum field values with small buffers
  ::Test_Simple_Types msg;
  msg.set_a_int32(std::numeric_limits<int32_t>::max());   
  msg.set_a_int64(std::numeric_limits<int64_t>::max());     
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

  std::array<uint8_t, 150> collected_data = {0};
  uint32_t total_bytes = 0;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t iterations = 0;
  constexpr uint32_t MAX_ITERATIONS = 20;

  while((::EmbeddedProto::Error::BUFFER_FULL == result) && (iterations < MAX_ITERATIONS))
  {
    ::EmbeddedProto::WriteBufferFixedSize<20> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    ++iterations;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(100U, total_bytes);
  EXPECT_LT(iterations, MAX_ITERATIONS);
}

TEST(SimpleTypes, PartialSerialize_SingleBool_MinimumBuffer)
{
  // Test 14.3.12: Minimum buffer size (2 bytes) for bool field
  ::Test_Simple_Types msg;
  msg.set_a_bool(true);

  ::EmbeddedProto::WriteBufferFixedSize<2> buffer;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());
  EXPECT_EQ(0x38, buffer.get_data()[0]);
  EXPECT_EQ(0x01, buffer.get_data()[1]);
}

TEST(SimpleTypes, PartialSerialize_Fixed32_ExactFit)
{
  // Test 14.3.13: fixed32 field (5 bytes) with exactly fitting buffer
  ::Test_Simple_Types msg;
  msg.set_a_fixed32(1);

  ::EmbeddedProto::WriteBufferFixedSize<5> buffer;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(5U, buffer.get_size());

  std::array<uint8_t, 5> expected = {0x65, 0x01, 0x00, 0x00, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(SimpleTypes, PartialSerialize_Fixed32_BufferOneByteTooSmall)
{
  // Test 14.3.14: fixed32 field with buffer one byte too small
  ::Test_Simple_Types msg;
  msg.set_a_fixed32(1);

  ::Test_Simple_Types::StateStack state;

  // Buffer 4 bytes - 1 byte too small for 5-byte field
  ::EmbeddedProto::WriteBufferFixedSize<4> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(0U, bufferA.get_size());  // Rollback

  // Second buffer succeeds
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(5U, bufferB.get_size());
}

TEST(SimpleTypes, PartialSerialize_StateReset_SerializeTwice)
{
  // Test 14.3.15: State can be reset and reused
  ::Test_Simple_Types msg;
  msg.set_a_int32(42);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer1;
  ::Test_Simple_Types::StateStack state;

  // First serialization
  ::EmbeddedProto::Error result = msg.serialize_partial(buffer1, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  uint32_t size1 = buffer1.get_size();

  // Reset state and buffer
  state.reset();
  ::EmbeddedProto::WriteBufferFixedSize<10> buffer2;

  // Second serialization should produce identical output
  result = msg.serialize_partial(buffer2, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(size1, buffer2.get_size());

  for(uint32_t i = 0; i < size1; ++i)
  {
    EXPECT_EQ(buffer1.get_data()[i], buffer2.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(SimpleTypes, PartialSerialize_MixedFields_VarintThenFixed)
{
  // Test 14.3.17: Transition from varint to fixed field across buffer boundary
  ::Test_Simple_Types msg;
  msg.set_a_bool(true);    // Field 7, 2 bytes
  msg.set_a_fixed64(1);    // Field 9, 9 bytes

  ::Test_Simple_Types::StateStack state;

  // Buffer fits bool only
  ::EmbeddedProto::WriteBufferFixedSize<2> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, bufferA.get_size());
  EXPECT_EQ(0x38, bufferA.get_data()[0]);
  EXPECT_EQ(0x01, bufferA.get_data()[1]);

  // Second buffer fits fixed64
  ::EmbeddedProto::WriteBufferFixedSize<15> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, bufferB.get_size());
  EXPECT_EQ(0x49, bufferB.get_data()[0]);  // Tag for field 9 with FIXED64 wire type
}

TEST(SimpleTypes, PartialSerialize_EnumField_LargeValue)
{
  // Test 14.3.18: Enum with large value (multi-byte varint)
  ::Test_Simple_Types msg;
  msg.set_a_enum(Test_Enum::TWOBILLION);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::Test_Simple_Types::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(6U, buffer.get_size());

  std::array<uint8_t, 6> expected = {0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(SimpleTypes, PartialSerialize_ConsecutiveSmallBuffers_VerifyProgress)
{
  // Test 14.3.20: State correctly tracks progress through multiple fields
  ::Test_Simple_Types msg;
  msg.set_a_int32(1);   // 2 bytes
  msg.set_a_int64(1);   // 2 bytes
  msg.set_a_uint32(1);  // 2 bytes
  msg.set_a_uint64(1);  // 2 bytes

  ::Test_Simple_Types::StateStack state;

  // Call 1: First field
  ::EmbeddedProto::WriteBufferFixedSize<2> buffer1;
  ::EmbeddedProto::Error result = msg.serialize_partial(buffer1, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, buffer1.get_size());
  EXPECT_EQ(0x08, buffer1.get_data()[0]);
  EXPECT_EQ(0x01, buffer1.get_data()[1]);

  // Call 2: Second field
  ::EmbeddedProto::WriteBufferFixedSize<2> buffer2;
  result = msg.serialize_partial(buffer2, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, buffer2.get_size());
  EXPECT_EQ(0x10, buffer2.get_data()[0]);
  EXPECT_EQ(0x01, buffer2.get_data()[1]);

  // Call 3: Third field
  ::EmbeddedProto::WriteBufferFixedSize<2> buffer3;
  result = msg.serialize_partial(buffer3, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, buffer3.get_size());
  EXPECT_EQ(0x18, buffer3.get_data()[0]);
  EXPECT_EQ(0x01, buffer3.get_data()[1]);

  // Call 4: Fourth field (last set)
  ::EmbeddedProto::WriteBufferFixedSize<2> buffer4;
  result = msg.serialize_partial(buffer4, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer4.get_size());
  EXPECT_EQ(0x20, buffer4.get_data()[0]);
  EXPECT_EQ(0x01, buffer4.get_data()[1]);
}

TEST(SimpleTypes, PartialDeserialize_Fixed64SplitInData)
{
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  buffer.push(0x49);
  buffer.push(0x01);
  buffer.push(0x00);
  buffer.push(0x00);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x00);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(1U, msg.get_a_fixed64());
}

TEST(SimpleTypes, PartialDeserialize_DoubleSplitInData)
{
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  buffer.push(0x59);
  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x00);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0xF0);
  buffer.push(0x3F);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(1.0, msg.get_a_double());
}

TEST(SimpleTypes, PartialDeserialize_SFixed32AndFloat_MultiFieldProgress)
{
  ::EmbeddedProto::ReadBufferFixedSize<20> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // a_sfixed32 = 1 and a_float = 1.0F
  buffer.push(0x6D);
  buffer.push(0x01);
  buffer.push(0x00);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x75);
  buffer.push(0x00);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  buffer.push(0x00);
  buffer.push(0x80);
  buffer.push(0x3F);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(1, msg.get_a_sfixed32());
  EXPECT_EQ(1.0F, msg.get_a_float());
}

TEST(SimpleTypes, PartialDeserialize_FatalOverlongVarint)
{
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer;
  ::Test_Simple_Types msg;
  ::Test_Simple_Types::StateStack state;

  // a_int64 tag + overlong varint payload
  buffer.push(0x10);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.deserialize_partial(buffer, state.root()));
}

#endif // PARTIAL_SERIALIZATION_ENABLED
} // End of namespace test_EmbeddedAMS_SimpleTypes
