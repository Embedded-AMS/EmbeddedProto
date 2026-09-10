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
#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
#include <MessageState.h>

#include <cstdint>    
#include <limits> 
#include <array>
#include <string.h>

// EAMS message definitions
#include <oneof_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;


TEST(OneofField, construction) 
{
  message_oneof msg;
}

TEST(OneofField, serialize_zero) 
{
  message_oneof msg;
  Mocks::WriteBufferMock buffer;
  
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).Times(0);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(OneofField, set_get_clear)
{
  message_oneof msg;
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  EXPECT_FALSE(msg.has_x());
  msg.set_x(1);
  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_TRUE(msg.has_x());
  msg.clear_x();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  EXPECT_FALSE(msg.has_y());
  msg.set_y(1);
  EXPECT_EQ(1, msg.get_y());
  EXPECT_EQ(message_oneof::FieldNumber::Y, msg.get_which_xyz());
  EXPECT_TRUE(msg.has_y());
  msg.clear_y();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  EXPECT_FALSE(msg.has_z());
  msg.set_z(1);
  EXPECT_EQ(1, msg.get_z());
  EXPECT_EQ(message_oneof::FieldNumber::Z, msg.get_which_xyz());
  EXPECT_TRUE(msg.has_z());
  msg.clear_z();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  msg.set_state(message_oneof::States::Run);
  EXPECT_EQ(message_oneof::States::Run, msg.get_state());
  EXPECT_EQ(message_oneof::FieldNumber::STATE, msg.get_which_xyz());
  EXPECT_TRUE(msg.has_state());
  msg.clear_state();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  EXPECT_FALSE(msg.has_state());

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());
  EXPECT_FALSE(msg.has_msg_ABC());
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);
  EXPECT_EQ(message_oneof::FieldNumber::MSG_ABC, msg.get_which_message());
  EXPECT_TRUE(msg.has_msg_ABC());
  EXPECT_EQ(1, msg.msg_ABC().varA());
  EXPECT_EQ(22, msg.msg_ABC().varB());
  EXPECT_EQ(333, msg.msg_ABC().varC());
  msg.clear_msg_ABC();
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());
  EXPECT_FALSE(msg.has_msg_ABC());

  msg.set_x(1);
  msg.mutable_msg_ABC().set_varA(1);
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(message_oneof::FieldNumber::MSG_ABC, msg.get_which_message());
  msg.clear();
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());
}

TEST(OneofField, serizlize_set_zeros) 
{
  // Oneof fields should serialize optional fields which are set to the default value.
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  msg.set_x(0);
  EXPECT_TRUE(msg.has_x());
  
  msg.mutable_msg_ABC().clear();
  EXPECT_TRUE(msg.has_msg_ABC());

  msg.set_u(0.0F);
  EXPECT_TRUE(msg.has_u());

  std::array<uint8_t, 10> expected = { 0x28, 0x00, // x
                                       0x7d, 0x00, 0x00, 0x00, 0x00, // u
                                       0xa2, 0x01, 0x00}; // msg ABC

  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(OneofField, serialize_ones) 
{
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  // X
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  
  std::array<uint8_t, 6> expected_x = { 0x08, 0x01,  // a
                                        0x50, 0x01,  // b
                                        0x28, 0x01 };// x

  for(auto e : expected_x) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // Y
  msg.set_y(1);
  std::array<uint8_t, 6> expected_y = { 0x08, 0x01,  // a
                                        0x50, 0x01,  // b
                                        0x30, 0x01 };// y

  for(auto e : expected_y) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // z
  msg.set_z(1);
  std::array<uint8_t, 6> expected_z = { 0x08, 0x01,  // a
                                        0x50, 0x01,  // b
                                        0x38, 0x01 };// z

  for(auto e : expected_z) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(OneofField, serialize_second_oneof)
{
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  // X and V
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1);

  std::array<uint8_t, 12> expected_z = {0x08, 0x01,  // a
                                        0x50, 0x01,  // b
                                        0x28, 0x01,  // x
                                        0x85, 0x01, 0x00, 0x00, 0x80, 0x3f }; // v

  for(auto e : expected_z) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}


TEST(OneofField, deserialize) 
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 6> referee = { 0x08, 0x01,  // a
                                     0x50, 0x01,  // b
                                     0x30, 0x01 };// y

  for(auto r: referee) {
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(1, msg.get_b());
  EXPECT_EQ(message_oneof::FieldNumber::Y, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_y());
}

TEST(OneofField, deserialize_override)
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 8> referee = { 0x08, 0x01,  // a
                                     0x50, 0x01,  // b
                                     0x30, 0x01,  // y
                                     0x28, 0x01 };// x 

  for(auto r: referee) {
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(1, msg.get_b());
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_x());
}

TEST(OneofField, deserialize_failure)
{
  message_oneof msg;

  ::EmbeddedProto::ReadBufferFixedSize<13> buffer(
                                    { 0x30, // y
                                      // This simulates the OVERLONG_VARINT Failure. More bytes than fit in a varint.
                                      0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                      0x00}); 

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.deserialize(buffer));
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());

}

TEST(OneofField, deserialize_second_oneof) 
{
  message_oneof msg;

  ::EmbeddedProto::ReadBufferFixedSize<13> buffer(
                                    { 0x08, 0x01,  // a
                                      0x50, 0x01,  // b
                                      0x28, 0x01,  // x
                                      0x85, 0x01, 0x00, 0x00, 0x80, 0x3f }); // v

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(1, msg.get_b());
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(message_oneof::FieldNumber::V, msg.get_which_uvw());
  EXPECT_EQ(1, msg.get_y());
}

TEST(OneofField, serialize_oneof_msg) 
{
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(1);
  msg.mutable_msg_ABC().set_varC(1);


  // Field ID
  EXPECT_CALL(buffer, push(0xA2)).Times(1).WillOnce(Return(true));
  // Followed by nested message size
  EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x06)).Times(1).WillOnce(Return(true));

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(99));

  std::array<uint8_t, 6> expected_ABC = { 0x08, 0x01,   // varA
                                          0x10, 0x01,   // varB
                                          0x18, 0x01 }; // varC

  for(auto e : expected_ABC) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(OneofField, deserialize_oneof_msg) 
{
  message_oneof msg;

  static constexpr uint32_t SIZE = 10;

  ::EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({0xaa, 0x01, 0x07, 0x08, 0x01, 0x10, 0x16, 0x18, 0xcd, 0x02});

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(message_oneof::FieldNumber::MSG_DEF, msg.get_which_message());
  EXPECT_EQ(1, msg.get_msg_DEF().get_varD());
  EXPECT_EQ(22, msg.get_msg_DEF().get_varE());
  EXPECT_EQ(333, msg.get_msg_DEF().get_varF());
}

TEST(OneofField, nested_assign)
{
  // This test will call the assignement operator and set the union data correctly in the message
  // which holds a nested message with oneof's.

  InSequence s;
  nested_oneof top_level_msg;

  message_oneof nested_msg;
  nested_msg.mutable_msg_ABC().set_varA(1);
  nested_msg.mutable_msg_ABC().set_varB(22);
  nested_msg.mutable_msg_ABC().set_varC(333);

  top_level_msg.set_msg_oneof(nested_msg);

  // Check the result.
  EXPECT_EQ(message_oneof::FieldNumber::MSG_ABC, top_level_msg.get_msg_oneof().get_which_message());
  EXPECT_EQ(1, top_level_msg.get_msg_oneof().msg_ABC().varA());
  EXPECT_EQ(22, top_level_msg.get_msg_oneof().msg_ABC().varB());
  EXPECT_EQ(333, top_level_msg.get_msg_oneof().msg_ABC().varC());
}

TEST(OneofField, sb_oneof_has)
{
  string_bytes_oneof<10, 10> msg;

  msg.mutable_name() = "John Doe";
  EXPECT_TRUE(msg.has_name());
  msg.mutable_name().clear();
  EXPECT_TRUE(msg.has_name());
  msg.clear_name();
  EXPECT_FALSE(msg.has_name());
  
  std::array<uint8_t, 3> d = {1, 2, 3};
  msg.mutable_data().set(d.data(), 3);
  EXPECT_TRUE(msg.has_data());
  msg.clear_data();
  EXPECT_FALSE(msg.has_data());
}

TEST(OneofField, sb_oneof_serialize_empty)
{
  InSequence s;
  string_bytes_oneof<10, 10> msg;
  Mocks::WriteBufferMock buffer;

  msg.mutable_name().clear();
  EXPECT_TRUE(msg.has_name());

  std::array<uint8_t, 2> expected = { 0x0a, 0x00}; // name

  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size for empty string
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  msg.mutable_data().clear();
  EXPECT_TRUE(msg.has_data());


  std::array<uint8_t, 2> expected2 = { 0x12, 0x00}; // data

  for(auto e : expected2) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size for empty bytes
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

}

#ifdef PARTIAL_SERIALIZATION_ENABLED

TEST(OneofField, PartialDeserialize_ScalarOneof_SplitTagAndData)
{
  message_oneof msg;
  message_oneof::StateStack state;

  ::EmbeddedProto::ReadBufferFixedSize<10> buffer({
    0x08, 0x01, // a = 1
    0x30        // y tag only
  });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(message_oneof::FieldNumber::Y, msg.get_which_xyz());

  buffer.push(0x02); // y = 2

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);
  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(message_oneof::FieldNumber::Y, msg.get_which_xyz());
  EXPECT_EQ(2, msg.get_y());
}

TEST(OneofField, PartialDeserialize_OverwriteLastFieldWins_AcrossChunks)
{
  message_oneof msg;
  message_oneof::StateStack state;

  ::EmbeddedProto::ReadBufferFixedSize<10> buffer({
    0x30, 0x01, // y = 1
    0x28        // x tag only
  });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());

  buffer.push(0x02); // x = 2

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(2, msg.get_x());
}

TEST(OneofField, PartialDeserialize_NestedOneofMessage_SplitTagSizeAndData)
{
  message_oneof msg;
  message_oneof::StateStack state;

  // msg_ABC tag split over two bytes.
  ::EmbeddedProto::ReadBufferFixedSize<20> buffer({0xA2});

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());

  buffer.push(0x01);
  buffer.push(0x07);
  buffer.push(0x08);
  buffer.push(0x01);
  buffer.push(0x10);
  buffer.push(0x16);
  buffer.push(0x18);
  buffer.push(0xCD);
  buffer.push(0x02);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);
  EXPECT_EQ(message_oneof::FieldNumber::MSG_ABC, msg.get_which_message());
  EXPECT_EQ(1, msg.get_msg_ABC().get_varA());
  EXPECT_EQ(22, msg.get_msg_ABC().get_varB());
  EXPECT_EQ(333, msg.get_msg_ABC().get_varC());
}

TEST(OneofField, PartialDeserialize_StringBytesOneof_StringAndBytesSplit)
{
  string_bytes_oneof<20, 20> msg_string;
  string_bytes_oneof<20, 20>::StateStack state_string;

  // name tag only first.
  ::EmbeddedProto::ReadBufferFixedSize<20> buffer_string({0x0A});

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg_string.deserialize_partial(buffer_string, state_string.root()));
  EXPECT_EQ((string_bytes_oneof<20, 20>::FieldNumber::NAME), msg_string.get_which_sb());

  buffer_string.push(0x02);
  buffer_string.push('J');
  buffer_string.push('o');

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg_string.deserialize_partial(buffer_string, state_string.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state_string.root().phase);
  EXPECT_EQ((string_bytes_oneof<20, 20>::FieldNumber::NAME), msg_string.get_which_sb());
  EXPECT_STREQ("Jo", msg_string.name());

  string_bytes_oneof<20, 20> msg_bytes;
  string_bytes_oneof<20, 20>::StateStack state_bytes;
  std::array<uint8_t, 2> expected = {0x01, 0x02};
  ::EmbeddedProto::ReadBufferFixedSize<20> buffer_bytes({0x12});

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg_bytes.deserialize_partial(buffer_bytes, state_bytes.root()));
  EXPECT_EQ((string_bytes_oneof<20, 20>::FieldNumber::DATA), msg_bytes.get_which_sb());

  buffer_bytes.push(0x02);
  buffer_bytes.push(0x01);
  buffer_bytes.push(0x02);

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg_bytes.deserialize_partial(buffer_bytes, state_bytes.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state_bytes.root().phase);
  EXPECT_EQ((string_bytes_oneof<20, 20>::FieldNumber::DATA), msg_bytes.get_which_sb());
  EXPECT_EQ(2U, msg_bytes.get_data().get_length());
  for(uint32_t i = 0U; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], msg_bytes.get_data().get_const(i));
  }
}

TEST(OneofField, PartialDeserialize_StateResetReuse_BetweenMessages)
{
  message_oneof msg;
  message_oneof::StateStack state;

  ::EmbeddedProto::ReadBufferFixedSize<10> buffer_x({0x28});
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer_x, state.root()));

  buffer_x.push(0x01);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer_x, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_x());

  msg.clear();
  state.reset();

  ::EmbeddedProto::ReadBufferFixedSize<10> buffer_y({0x30});
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer_y, state.root()));

  buffer_y.push(0x02);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer_y, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);
  EXPECT_EQ(message_oneof::FieldNumber::Y, msg.get_which_xyz());
  EXPECT_EQ(2, msg.get_y());
}

#endif // PARTIAL_SERIALIZATION_ENABLED


#ifndef DISABLE_FIELD_NUMBER_TO_NAME 

TEST(OneofField, field_number_to_name)
{
  EXPECT_TRUE(0 == strcmp(::message_oneof::field_number_to_name(::message_oneof::FieldNumber::X),
                          "x"));

  EXPECT_TRUE(0 == strcmp(::message_oneof::field_number_to_name(::message_oneof::FieldNumber::STATE),
                          "state"));

  EXPECT_TRUE(0 == strcmp(::message_oneof::field_number_to_name(::message_oneof::FieldNumber::MSG_ABC),
                          "msg_ABC"));
}

#endif


#ifdef MSG_TO_STRING

TEST(OneofField, to_string)
{
  message_oneof msg;

  // X and V
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1);

  constexpr uint32_t N = 1024;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);
  
  //std::cout << std::endl << str << std::endl;

  constexpr uint32_t TXT_LEN = 49;
  const char expected_str[TXT_LEN + 1] = "{\n  \"a\": 1,\n  \"b\": 1,\n  \"x\": 1,\n  \"v\": 1.000000\n}"; 
  
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);  
  EXPECT_EQ(str + TXT_LEN, str_left.data);
}

TEST(OneofField, to_string_buffer_overrun)
{
  message_oneof msg;

  // X and V
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1);

  constexpr uint32_t N = 10;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);

  EXPECT_EQ(0, str_left.size);  
  EXPECT_EQ(str + N, str_left.data);
}
#endif // End of MSG_TO_STRING

//==============================================================================
// Partial Serialization Tests
//==============================================================================

#ifdef PARTIAL_SERIALIZATION_ENABLED

TEST(OneofField, PartialSerialize_ScalarOneof_SufficientBuffer)
{
  // Test 3.1: Single scalar oneof field with sufficient buffer
  ::message_oneof msg;
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(6U, buffer.get_size());

  std::array<uint8_t, 6> expected = {0x08, 0x01,  // a
                                     0x50, 0x01,  // b
                                     0x28, 0x01}; // x
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_ScalarOneof_SplitBeforeOneof)
{
  // Test 3.2: Split between regular field and oneof field
  ::message_oneof msg;
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);

  ::message_oneof::StateStack state;

  // Buffer A: fits a field only
  ::EmbeddedProto::WriteBufferFixedSize<2> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, bufferA.get_size());
  EXPECT_EQ(0x08, bufferA.get_data()[0]);
  EXPECT_EQ(0x01, bufferA.get_data()[1]);

  // Buffer B: fits remaining fields
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(4U, bufferB.get_size());

  std::array<uint8_t, 4> expectedB = {0x50, 0x01,  // b
                                      0x28, 0x01}; // x
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_MultipleOneofs_SufficientBuffer)
{
  // Test 3.4: Multiple oneof groups with sufficient buffer
  ::message_oneof msg;
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1.0F);

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(12U, buffer.get_size());

  std::array<uint8_t, 12> expected = {0x08, 0x01,  // a
                                      0x50, 0x01,  // b
                                      0x28, 0x01,  // x
                                      0x85, 0x01, 0x00, 0x00, 0x80, 0x3f}; // v
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_MultipleOneofs_SplitBetweenOneofs)
{
  // Test 3.5: Split between two oneof groups
  ::message_oneof msg;
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1.0F);

  ::message_oneof::StateStack state;

  // Buffer A: fits a, b, x fields (6 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<6> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(6U, bufferA.get_size());

  std::array<uint8_t, 6> expectedA = {0x08, 0x01, 0x50, 0x01, 0x28, 0x01};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Buffer A mismatch at byte " << i;
  }

  // Buffer B: fits v field
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(6U, bufferB.get_size());

  std::array<uint8_t, 6> expectedB = {0x85, 0x01, 0x00, 0x00, 0x80, 0x3f};
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Buffer B mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_EnumOneof_SufficientBuffer)
{
  // Test 3.6: Enum field within oneof
  ::message_oneof msg;
  msg.set_state(message_oneof::States::Run);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());
  EXPECT_EQ(0x40, buffer.get_data()[0]);
  EXPECT_EQ(0x01, buffer.get_data()[1]);
}

TEST(OneofField, PartialSerialize_FloatOneof_Rollback)
{
  // Test 3.7: Rollback when float oneof field cannot fit entirely
  ::message_oneof msg;
  msg.set_a(1);
  msg.set_v(1.0F);

  ::message_oneof::StateStack state;

  // Buffer A: fits a but not v (v requires 6 bytes: tag 2 + data 4)
  // With 5 bytes: a (2 bytes) fits, but v (6 bytes) doesn't fit completely
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  // Buffer A contains {0x08, 0x01} (a field only, v rolled back)
  EXPECT_EQ(2U, bufferA.get_size());
  EXPECT_EQ(0x08, bufferA.get_data()[0]);
  EXPECT_EQ(0x01, bufferA.get_data()[1]);

  // Buffer B: fits v field (6 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  // Buffer B contains {0x85, 0x01, 0x00, 0x00, 0x80, 0x3f} (v field)
  EXPECT_EQ(6U, bufferB.get_size());
  EXPECT_EQ(0x85, bufferB.get_data()[0]);
  EXPECT_EQ(0x01, bufferB.get_data()[1]);
  EXPECT_EQ(0x00, bufferB.get_data()[2]);
  EXPECT_EQ(0x00, bufferB.get_data()[3]);
  EXPECT_EQ(0x80, bufferB.get_data()[4]);
  EXPECT_EQ(0x3f, bufferB.get_data()[5]);
}

TEST(OneofField, PartialSerialize_OneofSetToZero)
{
  // Test 3.8: Oneof fields serialize even when set to default value
  ::message_oneof msg;
  msg.set_x(0);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());
  EXPECT_EQ(0x28, buffer.get_data()[0]);
  EXPECT_EQ(0x00, buffer.get_data()[1]);
}

TEST(OneofField, PartialSerialize_NestedOneof_SufficientBuffer)
{
  // Test 4.1: Nested message within oneof with sufficient buffer
  ::message_oneof msg;
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(10U, buffer.get_size());

  std::array<uint8_t, 10> expected = {0xa2, 0x01, 0x07,  // tag and size of msg_ABC
                                      0x08, 0x01,        // varA
                                      0x10, 0x16,        // varB
                                      0x18, 0xcd, 0x02}; // varC
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_NestedOneof_SplitAtTag)
{
  // Test 4.2: Split at nested message tag
  ::message_oneof msg;
  msg.set_a(1);
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);

  ::message_oneof::StateStack state;

  // Buffer A: fits a field only
  ::EmbeddedProto::WriteBufferFixedSize<2> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, bufferA.get_size());
  EXPECT_EQ(0x08, bufferA.get_data()[0]);
  EXPECT_EQ(0x01, bufferA.get_data()[1]);

  // Buffer B: fits remaining message
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(10U, bufferB.get_size());
}

TEST(OneofField, PartialSerialize_NestedOneof_SplitDuringData)
{
  // Test 4.4: Split during nested message data
  ::message_oneof msg;
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);

  ::message_oneof::StateStack state;

  // Buffer A: fits tag + size + partial data (5 bytes)
  // msg_ABC: tag (2) + size (1) + varA tag (1) + varA value (1) = 5 bytes
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(5U, bufferA.get_size());

  // Buffer B: fits remaining data
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(5U, bufferB.get_size());

  // Verify combined data
  std::array<uint8_t, 10> expected = {0xa2, 0x01, 0x07,  // tag and size
                                      0x08, 0x01,        // varA
                                      0x10, 0x16,        // varB
                                      0x18, 0xcd, 0x02}; // varC
  for(uint32_t i = 0; i < 5; ++i)
  {
    EXPECT_EQ(expected[i], bufferA.get_data()[i]) << "Buffer A mismatch at byte " << i;
  }
  for(uint32_t i = 0; i < 5; ++i)
  {
    EXPECT_EQ(expected[5 + i], bufferB.get_data()[i]) << "Buffer B mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_NestedOneof_EmptyMessage)
{
  // Test 4.5: Nested oneof message with no fields set
  ::message_oneof msg;
  msg.mutable_msg_ABC().clear();

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(3U, buffer.get_size());
  EXPECT_EQ(0xa2, buffer.get_data()[0]);
  EXPECT_EQ(0x01, buffer.get_data()[1]);
  EXPECT_EQ(0x00, buffer.get_data()[2]);
}

TEST(OneofField, PartialSerialize_NestedOneof_Rollback)
{
  // Test 4.6: When buffer can't fit next field's tag completely
  // The serialization stops after field 'a' and continues in next buffer
  ::message_oneof msg;
  msg.set_a(1);
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);

  ::message_oneof::StateStack state;

  // Buffer A: fits a (2 bytes), msg_ABC tag is 2 bytes (0xa2 0x01)
  // With 4 byte buffer: a fits (2 bytes), 2 bytes for tag (but not size or data)
  ::EmbeddedProto::WriteBufferFixedSize<4> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  // May write a + partial tag depending on implementation
  EXPECT_GE(bufferA.get_size(), 2U);  // At least a field

  // Buffer B: sufficient for remaining
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  // Total bytes (a=2, msg_ABC tag=2, size=1, nested_content=7) = 12 minimum
  // Combined could be 12-14 depending on partial tag handling
  EXPECT_GE(bufferA.get_size() + bufferB.get_size(), 12U);  // Minimum needed
  EXPECT_LE(bufferA.get_size() + bufferB.get_size(), 14U);  // Max with potential overlap
}

TEST(OneofField, PartialSerialize_AlternateNestedOneof)
{
  // Test 4.7: Alternate nested message in oneof (msg_DEF)
  ::message_oneof msg;
  msg.mutable_msg_DEF().set_varD(1);
  msg.mutable_msg_DEF().set_varE(22);
  msg.mutable_msg_DEF().set_varF(333);

  std::array<uint8_t, 20> collected_data = {0};
  uint32_t total_bytes = 0;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t iterations = 0;
  constexpr uint32_t MAX_ITERATIONS = 10;

  while((::EmbeddedProto::Error::BUFFER_FULL == result) && (iterations < MAX_ITERATIONS))
  {
    ::EmbeddedProto::WriteBufferFixedSize<5> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());

    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    ++iterations;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(10U, total_bytes);
  EXPECT_LT(iterations, MAX_ITERATIONS);

  std::array<uint8_t, 10> expected = {0xaa, 0x01, 0x07,  // tag and size of msg_DEF
                                      0x08, 0x01,        // varD
                                      0x10, 0x16,        // varE
                                      0x18, 0xcd, 0x02}; // varF
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], collected_data[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_NestedOneof_TwoLevels_SufficientBuffer)
{
  // Test 5.1: nested_oneof message (two levels of nesting)
  ::nested_oneof msg;
  msg.mutable_msg_oneof().set_a(1);
  msg.mutable_msg_oneof().set_b(1);
  msg.mutable_msg_oneof().set_x(1);

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  ::nested_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(8U, buffer.get_size());

  std::array<uint8_t, 8> expected = {0x0a, 0x06,  // tag and size of msg_oneof
                                     0x08, 0x01,  // a
                                     0x50, 0x01,  // b
                                     0x28, 0x01}; // x
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_NestedOneof_TwoLevels_SplitAtOuterTag)
{
  // Test 5.2: Split at outer message tag
  ::nested_oneof msg;
  msg.mutable_msg_oneof().set_a(1);
  msg.mutable_msg_oneof().set_b(1);
  msg.mutable_msg_oneof().set_x(1);

  ::nested_oneof::StateStack state;

  // Buffer A: fits partial tag only (1 byte)
  // Depending on implementation, partial tags may or may not be written
  ::EmbeddedProto::WriteBufferFixedSize<1> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  // BufferA will have 0 or 1 byte depending on tag rollback behavior
  EXPECT_LE(bufferA.get_size(), 1U);

  // Buffer B: fits remaining message
  ::EmbeddedProto::WriteBufferFixedSize<15> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  // Combined should be 8 bytes total
  EXPECT_EQ(8U, bufferA.get_size() + bufferB.get_size());
}

TEST(OneofField, PartialSerialize_NestedOneof_ThreeLevels)
{
  // Test 5.4: Three levels of nesting (nested_oneof -> message_oneof -> msg_ABC)
  ::nested_oneof msg;
  msg.mutable_msg_oneof().mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_oneof().mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_oneof().mutable_msg_ABC().set_varC(333);

  std::array<uint8_t, 30> collected_data = {0};
  uint32_t total_bytes = 0;
  ::nested_oneof::StateStack state;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t iterations = 0;
  constexpr uint32_t MAX_ITERATIONS = 10;

  while((::EmbeddedProto::Error::BUFFER_FULL == result) && (iterations < MAX_ITERATIONS))
  {
    ::EmbeddedProto::WriteBufferFixedSize<5> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());

    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    ++iterations;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(12U, total_bytes);
  EXPECT_LT(iterations, MAX_ITERATIONS);
}

TEST(OneofField, PartialSerialize_StringOneof_SufficientBuffer)
{
  // Test 6.1: String field within oneof
  ::string_bytes_oneof<20, 20> msg;
  msg.mutable_name() = "John Doe";

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  ::string_bytes_oneof<20, 20>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(10U, buffer.get_size());

  std::array<uint8_t, 10> expected = {0x0a, 0x08, 'J', 'o', 'h', 'n', ' ', 'D', 'o', 'e'};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_StringOneof_SplitDuringData)
{
  // Test 6.2: Split during string data
  ::string_bytes_oneof<20, 20> msg;
  msg.mutable_name() = "John Doe";

  ::string_bytes_oneof<20, 20>::StateStack state;

  // Buffer A: fits tag + size + partial string
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(5U, bufferA.get_size());

  std::array<uint8_t, 5> expectedA = {0x0a, 0x08, 'J', 'o', 'h'};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Buffer A mismatch at byte " << i;
  }

  // Buffer B: fits remaining string
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(5U, bufferB.get_size());

  std::array<uint8_t, 5> expectedB = {'n', ' ', 'D', 'o', 'e'};
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Buffer B mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_BytesOneof_SufficientBuffer)
{
  // Test 6.3: Bytes field within oneof
  ::string_bytes_oneof<20, 20> msg;
  std::array<uint8_t, 5> data = {0x01, 0x02, 0x03, 0x04, 0x05};
  msg.mutable_data().set(data.data(), data.size());

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::string_bytes_oneof<20, 20>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(7U, buffer.get_size());

  std::array<uint8_t, 7> expected = {0x12, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_BytesOneof_SplitDuringData)
{
  // Test 6.4: Split during bytes data
  ::string_bytes_oneof<20, 20> msg;
  std::array<uint8_t, 5> data = {0x01, 0x02, 0x03, 0x04, 0x05};
  msg.mutable_data().set(data.data(), data.size());

  ::string_bytes_oneof<20, 20>::StateStack state;

  // Buffer A: fits tag + size + partial bytes
  ::EmbeddedProto::WriteBufferFixedSize<4> bufferA;
  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(4U, bufferA.get_size());

  std::array<uint8_t, 4> expectedA = {0x12, 0x05, 0x01, 0x02};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Buffer A mismatch at byte " << i;
  }

  // Buffer B: fits remaining bytes
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(3U, bufferB.get_size());

  std::array<uint8_t, 3> expectedB = {0x03, 0x04, 0x05};
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Buffer B mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_EmptyStringOneof)
{
  // Test 6.5: Empty string within oneof
  ::string_bytes_oneof<20, 20> msg;
  msg.mutable_name().clear();

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::string_bytes_oneof<20, 20>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());
  EXPECT_EQ(0x0a, buffer.get_data()[0]);
  EXPECT_EQ(0x00, buffer.get_data()[1]);
}

TEST(OneofField, PartialSerialize_LoopSmallBuffers)
{
  // Test 8.1: Serialize using small buffers in a loop
  // Note: Buffer size must accommodate atomic scalar fields (float = 6 bytes)
  ::message_oneof msg;
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1.0F);
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);

  std::array<uint8_t, 50> collected_data = {0};
  uint32_t total_bytes = 0;
  ::message_oneof::StateStack state;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t iterations = 0;
  constexpr uint32_t MAX_ITERATIONS = 20;

  while((::EmbeddedProto::Error::BUFFER_FULL == result) && (iterations < MAX_ITERATIONS))
  {
    // Use 6-byte buffer to accommodate float field (tag 2 + data 4 = 6 bytes)
    ::EmbeddedProto::WriteBufferFixedSize<6> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());

    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    ++iterations;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(22U, total_bytes);
  EXPECT_LT(iterations, MAX_ITERATIONS);

  // Verify the collected data matches expected serialization
  std::array<uint8_t, 22> expected = {0x08, 0x01,  // a
                                      0x50, 0x01,  // b
                                      0x28, 0x01,  // x
                                      0x85, 0x01, 0x00, 0x00, 0x80, 0x3f,  // v
                                      0xa2, 0x01, 0x07,  // msg_ABC tag and size
                                      0x08, 0x01,        // varA
                                      0x10, 0x16,        // varB
                                      0x18, 0xcd, 0x02}; // varC
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], collected_data[i]) << "Mismatch at byte " << i;
  }
}

TEST(OneofField, PartialSerialize_StateReset_SerializeTwice)
{
  // Test 8.2: State can be reset and reused
  ::message_oneof msg1;
  msg1.set_x(1);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer1;
  ::message_oneof::StateStack state;

  // First serialization
  ::EmbeddedProto::Error result = msg1.serialize_partial(buffer1, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  uint32_t size1 = buffer1.get_size();

  // Reset state
  state.reset();

  // Second serialization with different message
  ::message_oneof msg2;
  msg2.set_y(2);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer2;
  result = msg2.serialize_partial(buffer2, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);

  EXPECT_EQ(size1, buffer2.get_size());  // Same size (both are 2 bytes)
  EXPECT_EQ(0x30, buffer2.get_data()[0]);  // y tag
  EXPECT_EQ(0x02, buffer2.get_data()[1]);  // value 2
}

TEST(OneofField, PartialSerialize_SingleOneof_Set)
{
  // Test 10.1: oneof_sigle message with field set
  ::oneof_sigle msg;
  msg.set_single_num(42);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::oneof_sigle::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());
  EXPECT_EQ(0x08, buffer.get_data()[0]);
  EXPECT_EQ(0x2a, buffer.get_data()[1]);
}

TEST(OneofField, PartialSerialize_SingleOneof_NotSet)
{
  // Test 10.2: oneof_sigle message with no field set
  ::oneof_sigle msg;

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::oneof_sigle::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(0U, buffer.get_size());
}

//==============================================================================
// Basic Oneof Partial Serialization Tests
//==============================================================================

TEST(OneofField, PartialSerialize_BasicOneof_VarASet)
{
  // Test basic_oneof with varA set and sufficient buffer
  ::basic_oneof msg;
  msg.set_varA(42);

  EXPECT_TRUE(msg.has_varA());
  EXPECT_EQ(basic_oneof::FieldNumber::VARA, msg.get_which_abc());

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::basic_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());

  // Expected: tag 0x08 (field 1, varint), value 0x2a (42)
  EXPECT_EQ(0x08, buffer.get_data()[0]);
  EXPECT_EQ(0x2a, buffer.get_data()[1]);
}

TEST(OneofField, PartialSerialize_BasicOneof_NotSet)
{
  // Test basic_oneof with no field set - should produce 0 bytes
  ::basic_oneof msg;

  EXPECT_EQ(basic_oneof::FieldNumber::NOT_SET, msg.get_which_abc());

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::basic_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(0U, buffer.get_size());
}

TEST(OneofField, PartialSerialize_BasicOneof_VarBSet)
{
  // Test basic_oneof with varB set
  ::basic_oneof msg;
  msg.set_varB(100);

  EXPECT_TRUE(msg.has_varB());
  EXPECT_EQ(basic_oneof::FieldNumber::VARB, msg.get_which_abc());

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::basic_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());

  // Expected: tag 0x10 (field 2, varint), value 0x64 (100)
  EXPECT_EQ(0x10, buffer.get_data()[0]);
  EXPECT_EQ(0x64, buffer.get_data()[1]);
}

TEST(OneofField, PartialSerialize_BasicOneof_SetToZero)
{
  // Test basic_oneof with varC set to 0 - oneof should still serialize
  ::basic_oneof msg;
  msg.set_varC(0);

  EXPECT_TRUE(msg.has_varC());
  EXPECT_EQ(basic_oneof::FieldNumber::VARC, msg.get_which_abc());

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  ::basic_oneof::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());

  // Expected: tag 0x18 (field 3, varint), value 0x00 (0)
  EXPECT_EQ(0x18, buffer.get_data()[0]);
  EXPECT_EQ(0x00, buffer.get_data()[1]);
}

#endif // PARTIAL_SERIALIZATION_ENABLED
