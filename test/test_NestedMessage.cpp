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

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
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
  InSequence s;

  ::demo::space::message_b<SIZE_MSG_A> msg;
  Mocks::ReadBufferMock buffer;


  static constexpr uint32_t SIZE = 23;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  // Test if a nested message can be deserialized with values set to one.

  std::array<uint8_t, SIZE> referee = { 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                                        0x12, 0x0A, // tag and size of nested a
                                        0x0A, 0x01, 0x01, // x
                                        0x15, 0x00, 0x00, 0x80, 0x3F, // y
                                        0x18, 0x02, // z
                                        // And back to the parent message with field v.
                                        0x18, 0x01 };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1, msg.get_nested_a().get_x().get_length());
  EXPECT_EQ(1, msg.get_nested_a().x(0));
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_z());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, deserialize_nested_in_nested_max) 
{
  InSequence s;

  ::demo::space::message_c<SIZE_MSG_A, SIZE_MSG_D> msg;
  Mocks::ReadBufferMock buffer;


  static constexpr uint32_t SIZE = 64;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  // Test if a double nested message can be deserialized with values set to maximum.

  std::array<uint8_t, SIZE> referee = { 0x0A, 0x28, // tag and size of nested b
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
                                        0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07 }; // Value g 

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

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

} // End of namespace test_EmbeddedAMS_NestedMessage
