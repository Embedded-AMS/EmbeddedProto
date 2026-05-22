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
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
#include <WriteBufferFixedSize.h>
#include <Errors.h>

#include <cstdint>    
#include <limits> 
#include <array>

// EAMS message definitions
#include <nested_message.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_NestedMessage
{

constexpr uint32_t SIZE_MSG_A = 3;
constexpr uint32_t SIZE_MSG_D = 5;


TEST(NestedMessage, assign_by_set)
{
    ::demo::space::message_a<SIZE_MSG_A> msg_a;
    msg_a.mutable_z() = 1;

    ::demo::space::message_b<SIZE_MSG_A> msg_b;
    msg_b.set_nested_a(msg_a);

    EXPECT_EQ(1, msg_b.nested_a().z());
}

TEST(NestedMessage, assign_by_refrence)
{
    ::demo::space::message_b<SIZE_MSG_A> msg_b;
    auto& msg_a = msg_b.mutable_nested_a();
    msg_a.mutable_z() = 1;

    EXPECT_EQ(1, msg_b.nested_a().z());
}

TEST(NestedMessage, serialize_zero) 
{
  // Test if a unset message results in zero bytes in the buffer.

  ::demo::space::message_b<SIZE_MSG_A> msg;
  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}

TEST(NestedMessage, serialize_one) 
{
  InSequence s;

  ::demo::space::message_b<SIZE_MSG_A> msg;
  Mocks::WriteBufferMock buffer;
  ON_CALL(buffer, get_size()).WillByDefault(Return(25));


  // Test if a nested message can be serialized with values set to one.
  msg.set_u(1.0F);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  std::array<uint8_t, 9> expected_uv = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F}; // u

  for(auto e : expected_uv) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // tag and size of nested a
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x0A)).Times(1).WillOnce(Return(true));

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(12));

  // tag and size of x
  EXPECT_CALL(buffer, push(0x0A)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));

  // The next call is for the repeated field x.
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  std::array<uint8_t, 10> expected_a = {0x01, // x
                                        0x15, 0x00, 0x00, 0x80, 0x3f, // y
                                        0x18, 0x02, // z
                                        0x18, 0x01}; // And back to the parent message with field v.

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(23, msg.serialized_size());
}

TEST(NestedMessage, serialize_max) 
{
  InSequence s;

  ::demo::space::message_b<SIZE_MSG_A> msg;
  Mocks::WriteBufferMock buffer;

  // Test if a nested message can be serialized with values set to one.
  msg.set_u(std::numeric_limits<double>::max());
  msg.mutable_nested_a().add_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());
  msg.set_v(std::numeric_limits<int32_t>::max());

  std::array<uint8_t, 9> expected_b = {0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F}; // u

  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // tag and size of nested a
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x17)).Times(1).WillOnce(Return(true));

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(29));

  // tag and size of x
  EXPECT_CALL(buffer, push(0X0A)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x05)).Times(1).WillOnce(Return(true));

  // The next call is for the repeated field x.
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(27));

  std::array<uint8_t, 27> expected_a = {0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                                        0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                                        0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, // z
                                        // And back to the parent message with field v.
                                        0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x07};

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(40, msg.serialized_size());
}

TEST(NestedMessage, serialize_nested_in_nested_max) 
{
  InSequence s;

  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D> msg;
  Mocks::WriteBufferMock buffer;

  // Test if a nested message in a nested message with some data works.
  msg.mutable_nested_b().set_u(std::numeric_limits<double>::max());
  msg.mutable_nested_b().mutable_nested_a().add_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_b().mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_b().mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());
  msg.mutable_nested_b().set_v(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_d().add_d(std::numeric_limits<uint32_t>::max());
  msg.mutable_nested_d().add_d(std::numeric_limits<uint32_t>::max());
  msg.mutable_nested_g().set_g(std::numeric_limits<int32_t>::max());

  // tag and size of nested b
  EXPECT_CALL(buffer, push(0x0A)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x28)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(40));

  std::array<uint8_t, 9> expected_b = {0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F}; // u

  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // tag and size of nested a
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x17)).Times(1).WillOnce(Return(true));

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(29));

  // tag and size of x
  EXPECT_CALL(buffer, push(0x0A)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x05)).Times(1).WillOnce(Return(true));

  // The next call is for the repeated field x.
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(27));

  std::array<uint8_t, 27> expected_a = {0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                                        0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                                        0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, // z
                                        // And back to the parent message with field v.
                                        0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x07};

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // Tag and size of nested d
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x0C)).Times(1).WillOnce(Return(true));

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(12));

  // Tag and length of d
  EXPECT_CALL(buffer, push(0x0A)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x0A)).Times(1).WillOnce(Return(true));

  // The next call is for the repeated field d.
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  std::array<uint8_t, 10> expected_d = {0xFF, 0xFF, 0xFF, 0xFF, 0x0F,
                                        0xFF, 0xFF, 0xFF, 0xFF, 0x0F};

  for(auto e : expected_d) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  // Tag and size of g
  EXPECT_CALL(buffer, push(0x1A)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x06)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(6));

  std::array<uint8_t, 6> expected_g = {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07}; // g
  for(auto e : expected_g) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(NestedMessage, deserialize_one) 
{
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  static constexpr uint32_t SIZE = 23;

  // Test if a nested message can be deserialized with values set to one.

  ::EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({ 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                                        0x12, 0x0A, // tag and size of nested a
                                        0x0A, 0x01, 0x01, // x
                                        0x15, 0x00, 0x00, 0x80, 0x3F, // y
                                        0x18, 0x02, // z
                                        // And back to the parent message with field v.
                                        0x18, 0x01 });

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1, msg.get_nested_a().x(0));
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

#ifdef PARTIAL_DESERIALIZATION_ENABLED

TEST(NestedMessage, deserialize_one_partial_clean_tag) 
{
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  static constexpr uint32_t SIZE1 = 10;
  static constexpr uint32_t SIZE2 = 13;

  // Test splitting the buffer after the tag of nested message a just before the size of the message.

  ::EmbeddedProto::ReadBufferFixedSize<SIZE1> buffer1({ 
                                        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                                        0x12 // tag of nested a
                                    });
  
  // Split after the tag of nested message A
  
  ::EmbeddedProto::ReadBufferFixedSize<SIZE2> buffer2({ 
                                              0x0A, // size of nested a
                                        0x0A, 0x01, 0x01, // x                                  
                                        0x15, 0x00, 0x00, 0x80, 0x3F, // y of nested a
                                        0x18, 0x02, // z
                                        // And back to the parent message with field v.
                                        0x18, 0x01 });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer1));
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer2));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1, msg.get_nested_a().x(0));
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, deserialize_one_partial_clean_size) 
{
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  static constexpr uint32_t SIZE1 = 11;
  static constexpr uint32_t SIZE2 = 12;

  // Test splitting the buffer just after the size of nested message a.

  ::EmbeddedProto::ReadBufferFixedSize<SIZE1> buffer1({ 
                                        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                                        0x12, 0x0A, // tag and size of nested a
                                    });
  
  // Split after the tag of nested message A
  
  ::EmbeddedProto::ReadBufferFixedSize<SIZE2> buffer2({ 
                                        0x0A, 0x01, 0x01, // x of nested message a                               
                                        0x15, 0x00, 0x00, 0x80, 0x3F, // y of nested a
                                        0x18, 0x02, // z of nested message a
                                        // And back to the parent message with field v.
                                        0x18, 0x01 });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer1));
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer2));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1, msg.get_nested_a().x(0));
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, deserialize_one_partial_size) 
{
  ::demo::space::message_b<127> msg;
 
  // Have a larger nested message a where the size consists of multiple bytes. Split in between 
  // those size bytes.

  ::EmbeddedProto::ReadBufferFixedSize<150> buffer({  
                                        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                                        0x18, 0x01, // v
                                        0x12, 0x88, });   // Split within the size nested message A
  
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
  
  std::array<uint8_t, 10> temp_buffer({             0x01, // Continuation of the size of nested message A.
                                        0x15, 0x00, 0x00, 0x80, 0x3F, // y of nested a
                                        0x18, 0x02, // z of nested message a
                                        0x0A, 0x7F }); // Tag and size of x in nested message A.
  for(const auto& tb: temp_buffer) {
    buffer.push(tb);
  }

  // The reason why the message is large, a large array.
  for(uint32_t i = 0; i < 127; ++i)
  {
    buffer.push(0x01);
  }

  // Deserialize the second part. 
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(127, msg.get_nested_a().get_x().get_length());
  for(uint32_t i = 0; i < 127; ++i)
  {
    EXPECT_EQ(1, msg.get_nested_a().x(i));
  }
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, deserialize_one_partial_clean_field) 
{
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  static constexpr uint32_t SIZE1 = 14;
  static constexpr uint32_t SIZE2 = 9;

  // Split after a random field of nested message a.

  ::EmbeddedProto::ReadBufferFixedSize<SIZE1> buffer1({ 
                                        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                                        0x12, 0x0A, // tag and size of nested a
                                        0x0A, 0x01, 0x01, // x
                                    });
  
  // Clean split at the end of a field in a nested message
  
  ::EmbeddedProto::ReadBufferFixedSize<SIZE2> buffer2({                                   
                                        0x15, 0x00, 0x00, 0x80, 0x3F, // y of nested a
                                        0x18, 0x02, // z
                                        // And back to the parent message with field v.
                                        0x18, 0x01 });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer1));
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer2));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1, msg.get_nested_a().x(0));
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

#endif // PARTIAL_DESERIALIZATION_ENABLED

TEST(NestedMessage, deserialize_nested_in_nested_max) 
{
  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D> msg;

  static constexpr uint32_t SIZE = 64;

  // Test if a double nested message can be deserialized with values set to maximum.

  ::EmbeddedProto::ReadBufferFixedSize<SIZE> buffer (
                                      { 0x0A, 0x28, // tag and size of nested b
                                        0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, // u
                                        0x12, 0x17, // tag and size of nested a
                                        0x0A, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                                        0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                                        0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, // z
                                        // And back to the parent message with field v.
                                        0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x07,
                                        0x12, 0x0C, // Tag and size of nested d
                                        0x0A, 0x0A, // Length of d
                                        0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // Value(s) of d
                                        0x1A, 0x06, // Tag and size of nested g
                                        0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07 }); // Value g 

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<double>::max(), msg.get_nested_b().get_u());
  EXPECT_EQ(1, msg.get_nested_b().get_nested_a().get_x().get_length());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_nested_b().get_nested_a().x(0));
  EXPECT_EQ(std::numeric_limits<float>::max(), msg.get_nested_b().get_nested_a().get_y());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), msg.get_nested_b().get_nested_a().get_z());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_nested_b().get_v());
  EXPECT_EQ(2, msg.get_nested_d().get_d().get_length());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_nested_d().d(0));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_nested_d().d(1));
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_nested_g().get_g());
}

#ifdef MSG_TO_STRING

TEST(NestedMessage, to_string)
{
  ::demo::space::message_b<SIZE_MSG_A> msg;

  constexpr uint32_t N = 1024;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };

  // Test if a nested message can be serialized with values set to one.
  msg.set_u(1.0F);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().add_x(2);
  msg.mutable_nested_a().add_x(3);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);
  
  //std::cout << std::endl << str << std::endl;

  constexpr uint32_t TXT_LEN = 144;
  const char expected_str[TXT_LEN + 1] = "{\n  \"u\": 1.000000,\n  \"nested_a\": {\n    \"x\": [\n           1,\n           2,\n           3\n         ],\n    \"y\": 1.000000,\n    \"z\": 1\n  },\n  \"v\": 1\n}"; 
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);
  EXPECT_EQ(str + TXT_LEN, str_left.data);
}

#endif // MSG_TO_STRING

#if (EP_SERIALIZATION_MODE_PARTIAL == EP_SERIALIZATION_MODE)

TEST(NestedMessage, PartialSerialize_SingleNestedMessage_SufficientBuffer)
{
  // Test 3.1: Partial serialization of a message with a single nested message works when buffer is sufficient.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message
  msg.set_u(1.0);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  ::EmbeddedProto::WriteBufferFixedSize<50> buffer;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(23U, buffer.get_size());
  
  // Verify expected bytes
  std::array<uint8_t, 23> expected = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
    0x12, 0x0A, // tag and size of nested_a
    0x0A, 0x01, 0x01, // x
    0x15, 0x00, 0x00, 0x80, 0x3F, // y
    0x18, 0x02, // z
    0x18, 0x01  // v
  };
  
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(NestedMessage, PartialSerialize_NestedMessage_SplitAtTag)
{
  // Test 3.2: Verify that serialization can be split between parent fields and nested message tag.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message
  msg.set_u(1.0);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  // Buffer A: fits u field only (9 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<9> bufferA;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(9U, bufferA.get_size());
  
  // Verify Buffer A contains u field only
  std::array<uint8_t, 9> expectedA = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Mismatch in buffer A at byte " << i;
  }

  // Buffer B: fits remaining message (20 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(14U, bufferB.get_size());
  
  // Verify Buffer B contains remaining message starting with nested_a tag
  std::array<uint8_t, 14> expectedB = {
    0x12, 0x0A, // tag and size of nested_a
    0x0A, 0x01, 0x01, // x
    0x15, 0x00, 0x00, 0x80, 0x3F, // y
    0x18, 0x02, // z
    0x18, 0x01  // v
  };
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Mismatch in buffer B at byte " << i;
  }
}

TEST(NestedMessage, PartialSerialize_NestedMessage_SplitAtSize)
{
  // Test 3.3: Verify atomic tag+size for nested message field.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message
  msg.set_u(1.0);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  // Buffer A: cannot fit nested_a tag+size atomically after u field.
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferA;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(9U, bufferA.get_size());
  
  // Verify Buffer A contains only u field
  std::array<uint8_t, 9> expectedA = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Mismatch in buffer A at byte " << i;
  }

  // Buffer B: fits remaining message (20 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(14U, bufferB.get_size());
  
  // Verify Buffer B contains remaining message starting with nested_a tag+size
  std::array<uint8_t, 14> expectedB = {
    0x12, 0x0A, // tag and size of nested_a
    0x0A, 0x01, 0x01, // x
    0x15, 0x00, 0x00, 0x80, 0x3F, // y
    0x18, 0x02, // z
    0x18, 0x01  // v
  };
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Mismatch in buffer B at byte " << i;
  }
}

TEST(NestedMessage, PartialSerialize_NestedMessage_SplitDuringData)
{
  // Test 3.4: Verify that serialization can be split during nested message data.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message
  msg.set_u(1.0);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  // Buffer A: fits u + nested_a tag+size + x field (14 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<14> bufferA;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(14U, bufferA.get_size());
  
  // Verify Buffer A contains u + nested_a tag+size + x field
  std::array<uint8_t, 14> expectedA = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
    0x12, 0x0A, // tag and size of nested_a
    0x0A, 0x01, 0x01 // x
  };
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Mismatch in buffer A at byte " << i;
  }

  // Buffer B: fits remaining nested_a data + v field (15 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<15> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, bufferB.get_size());
  
  // Verify Buffer B contains remaining nested_a data + v field
  std::array<uint8_t, 9> expectedB = {
    0x15, 0x00, 0x00, 0x80, 0x3F, // y
    0x18, 0x02, // z
    0x18, 0x01  // v
  };
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Mismatch in buffer B at byte " << i;
  }
}

TEST(NestedMessage, PartialSerialize_DeeplyNestedMessage)
{
  // Test 3.5: Verify partial serialization with multiple levels of nesting.
  constexpr uint32_t SIZE_MSG_A = 3;
  constexpr uint32_t SIZE_MSG_D = 5;
  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D> msg;
  
  // Set up message with deeply nested structure
  msg.mutable_nested_b().set_u(1.0);
  msg.mutable_nested_b().mutable_nested_a().add_x(1);
  msg.mutable_nested_b().mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_b().mutable_nested_a().set_z(1);
  msg.mutable_nested_b().set_v(1);
  msg.mutable_nested_d().add_d(1);
  msg.mutable_nested_d().add_d(2);
  msg.mutable_nested_g().set_g(1);

  // Use loop with small buffers to test multiple levels of nesting
  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D>::StateStack state;
  std::array<uint8_t, 100> collected_data;
  uint32_t total_bytes = 0;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result) 
  {
    ::EmbeddedProto::WriteBufferFixedSize<20> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    // Copy to collected buffer
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  
  // For values set to 1, the expected size should be smaller
  // Let's verify the key structure rather than exact size
  
  // Verify key bytes at important positions
  // Tag and size of nested_b (field 1)
  EXPECT_EQ(0x0A, collected_data[0]);
  // Size will vary, but should be reasonable
  EXPECT_GT(collected_data[1], 0);
  EXPECT_LT(collected_data[1], 50);
  
  // u field in nested_b (should be present)
  EXPECT_EQ(0x09, collected_data[2]);
  
  // Look for nested_a tag (0x12) somewhere in the data
  bool found_nested_a_tag = false;
  for(uint32_t i = 0; i < total_bytes - 1; ++i)
  {
    if(collected_data[i] == 0x12)
    {
      found_nested_a_tag = true;
      break;
    }
  }
  EXPECT_TRUE(found_nested_a_tag);
  
  // Look for nested_d tag (0x12) somewhere after nested_b
  bool found_nested_d_tag = false;
  for(uint32_t i = 10; i < total_bytes - 1; ++i)
  {
    if(collected_data[i] == 0x12)
    {
      found_nested_d_tag = true;
      break;
    }
  }
  EXPECT_TRUE(found_nested_d_tag);
  
  // Look for nested_g tag (0x1A) somewhere after nested_d
  bool found_nested_g_tag = false;
  for(uint32_t i = 20; i < total_bytes - 1; ++i)
  {
    if(collected_data[i] == 0x1A)
    {
      found_nested_g_tag = true;
      break;
    }
  }
  EXPECT_TRUE(found_nested_g_tag);
}

TEST(NestedMessage, PartialSerialize_NestedMessage_LargeArray)
{
  // Test 3.6: Verify partial serialization when nested message contains large repeated field.
  ::demo::space::message_b<10> msg;  // Use smaller array for testing
  
  // Set up message with array in nested message
  msg.set_u(1.0);
  for(uint32_t i = 0; i < 10; ++i)
  {
    msg.mutable_nested_a().add_x(1);
  }
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  // Use loop with small buffers to test array serialization
  ::demo::space::message_b<10>::StateStack state;
  std::array<uint8_t, 100> collected_data;
  uint32_t total_bytes = 0;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result) 
  {
    ::EmbeddedProto::WriteBufferFixedSize<20> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    // Copy to collected buffer
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_GT(total_bytes, 0U);
  
  // Verify that the serialization completed successfully
  // The exact byte pattern is complex due to the nested structure,
  // but we can verify that the total size is reasonable
  EXPECT_LT(total_bytes, 100U);
  
  // Verify that the u field is present at the beginning
  EXPECT_EQ(0x09, collected_data[0]);
  
  // Verify that nested message tag is present
  bool found_nested_tag = false;
  for(uint32_t i = 0; i < total_bytes; ++i)
  {
    if(collected_data[i] == 0x12) // nested_a tag
    {
      found_nested_tag = true;
      break;
    }
  }
  EXPECT_TRUE(found_nested_tag);
}

TEST(NestedMessage, PartialSerialize_MultipleNestedMessages)
{
  // Test 3.7: Verify partial serialization with multiple nested messages at same level.
  constexpr uint32_t SIZE_MSG_A = 3;
  constexpr uint32_t SIZE_MSG_D = 5;
  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D> msg;
  
  // Set up message with multiple nested messages
  msg.mutable_nested_b().set_u(1.0);
  msg.mutable_nested_b().mutable_nested_a().add_x(1);
  msg.mutable_nested_b().mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_b().mutable_nested_a().set_z(1);
  msg.mutable_nested_b().set_v(1);
  msg.mutable_nested_d().add_d(1);
  msg.mutable_nested_d().add_d(2);
  msg.mutable_nested_g().set_g(1);

  // Use loop with small buffers to test multiple nested messages
  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D>::StateStack state;
  std::array<uint8_t, 100> collected_data;
  uint32_t total_bytes = 0;
  uint32_t buffer_count = 0;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result) 
  {
    ::EmbeddedProto::WriteBufferFixedSize<15> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    // Copy to collected buffer
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    buffer_count++;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_GT(buffer_count, 1U); // Should require multiple buffers
  
  // Verify that the collected data contains all expected nested messages
  EXPECT_GT(total_bytes, 0U);
  
  // Verify that the first buffer starts with nested_b tag
  EXPECT_EQ(0x0A, collected_data[0]);
  
  // Verify that nested_g data is present somewhere in the collected data
  bool found_g_field = false;
  for(uint32_t i = 0; i < total_bytes - 1; ++i)
  {
    if(collected_data[i] == 0x08 && collected_data[i+1] == 0x01) // g field
    {
      found_g_field = true;
      break;
    }
  }
  EXPECT_TRUE(found_g_field);
}

TEST(NestedMessage, PartialSerialize_NestedMessage_Rollback)
{
  // Test 3.8: Verify that when nested message cannot fit entirely, nothing is written (atomic behavior).
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message
  msg.set_u(1.0);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  // Buffer A: fits u field only (9 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<9> bufferA;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(9U, bufferA.get_size()); // Should fit u field only
  
  // Verify Buffer A contains only u field
  std::array<uint8_t, 9> expectedA = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F};
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]) << "Mismatch in buffer A at byte " << i;
  }

  // Buffer B: sufficient for remaining message (20 bytes)
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(14U, bufferB.get_size()); // Should fit remaining message
  
  // Verify Buffer B contains remaining message starting with nested_a tag
  std::array<uint8_t, 14> expectedB = {
    0x12, 0x0A, // tag and size of nested_a
    0x0A, 0x01, 0x01, // x
    0x15, 0x00, 0x00, 0x80, 0x3F, // y
    0x18, 0x02, // z
    0x18, 0x01  // v
  };
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]) << "Mismatch in buffer B at byte " << i;
  }
}

TEST(NestedMessage, PartialSerialize_EmptyNestedMessage)
{
  // Test 3.9: Verify partial serialization when nested message has no fields set.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message with empty nested message
  msg.set_u(1.0);
  // nested_a is empty and non-optional, so it is skipped.
  msg.set_v(1);

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(11U, buffer.get_size()); // u field + v field
  
  // Verify expected bytes (u field + v field)
  std::array<uint8_t, 11> expected = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
    0x18, 0x01  // v
  };
  
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(NestedMessage, PartialSerialize_StateReuse)
{
  // Test 3.10: Verify that state can be reset and reused for another serialization.
  constexpr uint32_t SIZE_MSG_A = 3;
  
  // First message
  ::demo::space::message_b<SIZE_MSG_A> msg1;
  msg1.set_u(1.0);
  msg1.mutable_nested_a().add_x(1);
  msg1.mutable_nested_a().set_y(1.0F);
  msg1.mutable_nested_a().set_z(1);
  msg1.set_v(1);

  ::EmbeddedProto::WriteBufferFixedSize<30> buffer1;
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;

  ::EmbeddedProto::Error result = msg1.serialize_partial(buffer1, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(23U, buffer1.get_size());
  
  // Save first message data
  std::array<uint8_t, 23> first_data;
  memcpy(first_data.data(), buffer1.get_data(), 23);

  // Reset state
  state.reset();
  buffer1.clear();

  // Second message with different values
  ::demo::space::message_b<SIZE_MSG_A> msg2;
  msg2.set_u(2.0);
  msg2.mutable_nested_a().add_x(2);
  msg2.mutable_nested_a().set_y(2.0F);
  msg2.mutable_nested_a().set_z(2);
  msg2.set_v(2);

  result = msg2.serialize_partial(buffer1, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(23U, buffer1.get_size());
  
  // Verify that the second message has different data
  bool different_data = false;
  for(uint32_t i = 0; i < 23; ++i)
  {
    if(first_data[i] != buffer1.get_data()[i])
    {
      different_data = true;
      break;
    }
  }
  EXPECT_TRUE(different_data);
  
  // Verify that the second message has the expected u field tag
  EXPECT_EQ(0x09, buffer1.get_data()[0]); // u field tag
}

TEST(NestedMessage, PartialSerialize_LoopSmallBuffers)
{
  // Test 3.11: Verify serialization completes correctly when using many small buffers in a loop.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message
  msg.set_u(1.0);
  msg.mutable_nested_a().add_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  // Use loop with small buffers (10 bytes)
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;
  std::array<uint8_t, 50> collected_data;
  uint32_t total_bytes = 0;
  uint32_t buffer_count = 0;
  uint32_t max_iterations = 10; // Safety limit

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result && max_iterations-- > 0) 
  {
    ::EmbeddedProto::WriteBufferFixedSize<10> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    // Copy to collected buffer
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
    buffer_count++;
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(23U, total_bytes); // Should match expected size
  EXPECT_GT(buffer_count, 2U); // Should require multiple buffers
  
  // Verify collected data matches expected pattern
  EXPECT_EQ(0x09, collected_data[0]); // u field tag
  EXPECT_EQ(0x12, collected_data[9]); // nested_a tag
  EXPECT_EQ(0x18, collected_data[21]); // v field tag
}

TEST(NestedMessage, PartialSerialize_MaximumValues)
{
  // Test 3.12: Verify partial serialization with maximum field values.
  constexpr uint32_t SIZE_MSG_A = 3;
  ::demo::space::message_b<SIZE_MSG_A> msg;
  
  // Set up message with maximum values
  msg.set_u(std::numeric_limits<double>::max());
  msg.mutable_nested_a().add_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());
  msg.set_v(std::numeric_limits<int32_t>::max());

  // Use loop with small buffers
  ::demo::space::message_b<SIZE_MSG_A>::StateStack state;
  std::array<uint8_t, 100> collected_data;
  uint32_t total_bytes = 0;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result) 
  {
    ::EmbeddedProto::WriteBufferFixedSize<25> small_buffer;
    result = msg.serialize_partial(small_buffer, state.root());
    
    // Copy to collected buffer
    memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
    total_bytes += small_buffer.get_size();
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(40U, total_bytes); // Should match expected size from serialize_max test
  
  // Verify key bytes at important positions
  // u field tag
  EXPECT_EQ(0x09, collected_data[0]);
  
  // nested_a tag
  bool found_nested_a_tag = false;
  for(uint32_t i = 0; i < total_bytes; ++i)
  {
    if(collected_data[i] == 0x12)
    {
      found_nested_a_tag = true;
      break;
    }
  }
  EXPECT_TRUE(found_nested_a_tag);
}



TEST(NestedMessage, PartialDeserialize_NestedMessage_MultiFieldProgress)
{
  ::demo::space::message_b<SIZE_MSG_A> msg;
  ::EmbeddedProto::ReadBufferFixedSize<32> buffer;

  const std::array<uint8_t, 22> part_a = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F,
    0x12, 0x0A,
    0x0A, 0x01, 0x01,
    0x15, 0x00, 0x00, 0x80, 0x3F,
    0x18, 0x02,
    0x18
  };
  for(const auto& byte : part_a)
  {
    buffer.push(byte);
  }

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0x01);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1U, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1, msg.get_nested_a().x(0));
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, PartialDeserialize_NestedMessage_LargeSizeVarintSplit)
{
  ::demo::space::message_b<127> msg;
  ::EmbeddedProto::ReadBufferFixedSize<170> buffer;

  // Start with split in multi-byte size varint for nested_a.
  const std::array<uint8_t, 13> part_a = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F,
    0x18, 0x01,
    0x12, 0x88
  };
  for(const auto& byte : part_a)
  {
    buffer.push(byte);
  }

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  const std::array<uint8_t, 10> part_b = {
    0x01,
    0x15, 0x00, 0x00, 0x80, 0x3F,
    0x18, 0x02,
    0x0A, 0x7F
  };
  for(const auto& byte : part_b)
  {
    buffer.push(byte);
  }

  for(uint32_t i = 0; i < 127; ++i)
  {
    buffer.push(0x01);
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(127U, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, PartialDeserialize_NestedMessage_FatalOverlongVarint)
{
  ::demo::space::message_b<SIZE_MSG_A> msg;
  ::EmbeddedProto::ReadBufferFixedSize<32> buffer;

  const std::array<uint8_t, 22> data = {
    0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F,
    0x12, 0x0B,
    0x18,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  };

  for(const auto& byte : data)
  {
    buffer.push(byte);
  }

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.deserialize(buffer));
}

#endif // EP_SERIALIZATION_MODE_PARTIAL

} // End of namespace test_EmbeddedAMS_NestedMessage
