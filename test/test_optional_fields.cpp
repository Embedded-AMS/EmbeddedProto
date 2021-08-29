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

// EAMS message definitions
#include <optional_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace test_EmbeddedAMS_OptionalFields
{

TEST(OptionalFields, zero) 
{
  ::optional_fields<5,10> msg;

  // Test an int set to zero.
  EXPECT_FALSE(msg.has_b());
  msg.set_b(0);
  EXPECT_TRUE(msg.has_b());
  msg.clear_b();
  EXPECT_FALSE(msg.has_b());

  // Test a floating point to zero.
  EXPECT_FALSE(msg.has_y());
  msg.set_y(0.0F);
  EXPECT_TRUE(msg.has_y());
  msg.clear_y();
  EXPECT_FALSE(msg.has_y());

  // Address a nested message but do not set anything in it.
  EXPECT_FALSE(msg.has_pos());
  msg.mutable_pos().clear();
  EXPECT_TRUE(msg.has_pos());
  msg.clear_pos();
  EXPECT_FALSE(msg.has_pos());

  // Test an enum set to zero.
  EXPECT_FALSE(msg.has_state());
  msg.set_state(states::A);
  EXPECT_TRUE(msg.has_state());
  msg.clear_state();
  EXPECT_FALSE(msg.has_state());

  // Address a bytes array but do not set anything in it.
  EXPECT_FALSE(msg.has_bytes_array());
  msg.mutable_bytes_array().clear();
  EXPECT_TRUE(msg.has_bytes_array());
  msg.clear_bytes_array();
  EXPECT_FALSE(msg.has_bytes_array());

  // Address a string but do not set anything in it.
  EXPECT_FALSE(msg.has_str());
  msg.mutable_str().clear();
  EXPECT_TRUE(msg.has_str());
  msg.clear_str();
  EXPECT_FALSE(msg.has_str());

}

TEST(OptionalFields, construction_assignment) 
{
  ::optional_fields<5,10> msgA;
  msgA.set_b(1);
  msgA.set_y(1.0F);
  msgA.mutable_pos().set_xpos(1.0F);
  msgA.set_state(states::B);
  msgA.mutable_bytes_array()[0] = 1U;
  msgA.mutable_str().set("ABC", 3);

  optional_fields<5,10> msgB(msgA);
  EXPECT_TRUE(msgB.has_b());
  EXPECT_TRUE(msgB.has_y());
  EXPECT_TRUE(msgB.has_pos());
  EXPECT_TRUE(msgB.has_state());
  EXPECT_TRUE(msgB.has_bytes_array());
  EXPECT_TRUE(msgB.has_str());

  optional_fields<5,10> msgC = msgA;
  EXPECT_TRUE(msgC.has_b());
  EXPECT_TRUE(msgC.has_y());
  EXPECT_TRUE(msgC.has_pos());
  EXPECT_TRUE(msgC.has_state());
  EXPECT_TRUE(msgC.has_bytes_array());
  EXPECT_TRUE(msgC.has_str());

  optional_fields<5,10> msgD;
  msgD = msgA;
  EXPECT_TRUE(msgD.has_b());
  EXPECT_TRUE(msgD.has_y());
  EXPECT_TRUE(msgD.has_pos());
  EXPECT_TRUE(msgD.has_state());
  EXPECT_TRUE(msgD.has_bytes_array());
  EXPECT_TRUE(msgD.has_str());
}

TEST(OptionalFields, empty_serialization) 
{
  ::optional_fields<5,10> msg;
  msg.set_b(0);
  msg.set_y(0.0F);
  msg.mutable_pos().clear();
  msg.set_state(states::A);
  msg.mutable_bytes_array().clear();
  msg.mutable_str().clear();

  InSequence s;
  Mocks::WriteBufferMock buffer;

  std::array<uint8_t, 15> expected = { 0x10, 0x00, 
                                       0x25, 0x00, 0x00, 0x00, 0x00, 
                                       0x2a, 0x00, 
                                       0x30, 0x00,
                                       0x3a, 0x00, 
                                       0x42, 0x00}; // str

  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(OptionalFields, empty_deserialization) 
{

}

} // End of namespace test_EmbeddedAMS_OptionalFields
