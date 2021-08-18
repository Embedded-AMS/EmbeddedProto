/*
 *  Copyright (C) 2020-2021 Embedded AMS B.V. - All Rights Reserved
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
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>    
#include <limits> 
#include <array>

// EAMS message definitions
#include <oneof_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;


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
  msg.set_x(1);
  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  msg.clear_x();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  msg.set_y(1);
  EXPECT_EQ(1, msg.get_y());
  EXPECT_EQ(message_oneof::FieldNumber::Y, msg.get_which_xyz());
  msg.clear_y();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  msg.set_z(1);
  EXPECT_EQ(1, msg.get_z());
  EXPECT_EQ(message_oneof::FieldNumber::Z, msg.get_which_xyz());
  msg.clear_z();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  msg.set_state(message_oneof::States::Run);
  EXPECT_EQ(message_oneof::States::Run, msg.get_state());
  EXPECT_EQ(message_oneof::FieldNumber::STATE, msg.get_which_xyz());
  msg.clear_state();

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());

  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);
  EXPECT_EQ(message_oneof::FieldNumber::MSG_ABC, msg.get_which_message());
  EXPECT_EQ(1, msg.msg_ABC().varA());
  EXPECT_EQ(22, msg.msg_ABC().varB());
  EXPECT_EQ(333, msg.msg_ABC().varC());
  msg.clear_msg_ABC();
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());

  msg.set_x(1);
  msg.mutable_msg_ABC().set_varA(1);
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(message_oneof::FieldNumber::MSG_ABC, msg.get_which_message());
  msg.clear();
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_message());
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
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

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
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(1, msg.get_b());
  EXPECT_EQ(message_oneof::FieldNumber::X, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_x());
}

TEST(OneofField, deserialize_failure)
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x30), Return(true)));  // y
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(false))); // This simulates the fialure.

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
  EXPECT_EQ(message_oneof::FieldNumber::NOT_SET, msg.get_which_xyz());

}

TEST(OneofField, deserialize_second_oneof) 
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 12> referee = { 0x08, 0x01,  // a
                                      0x50, 0x01,  // b
                                      0x28, 0x01,  // x
                                      0x85, 0x01, 0x00, 0x00, 0x80, 0x3f }; // v

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

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
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 10;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  std::array<uint8_t, SIZE> referee = {0xaa, 0x01, 0x07, 0x08, 0x01, 0x10, 0x16, 0x18, 0xcd, 0x02};
  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

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