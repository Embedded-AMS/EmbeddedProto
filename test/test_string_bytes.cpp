/*
 *  Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
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
#include "gmock/gmock.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <string_bytes.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::ElementsAre;

namespace test_EmbeddedAMS_string_bytes
{

TEST(FieldString, clear)
{
  text<10> msg;  
  
  // Clear the field specific.
  msg.mutable_txt() = "Foo Bar";
  EXPECT_EQ(7, msg.txt().get_length());
  msg.clear_txt();
  EXPECT_EQ(0, msg.txt().get_length());

  // Clear the whole message.
  msg.mutable_txt() = "Foo Bar";
  msg.clear();
  EXPECT_EQ(0, msg.txt().get_length());
}

TEST(FieldString, serialize) 
{
  InSequence s;

  text<10> msg;
  Mocks::WriteBufferMock buffer;

  msg.mutable_txt() = "Foo bar";

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  uint8_t expected[] = {0x0a, 0x07};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  EXPECT_CALL(buffer, push(_, 7)).Times(1).WillOnce(Return(true));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.txt().get_max_length());
}

TEST(FieldString, deserialize) 
{
  InSequence s;

  text<10> msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.txt().get_length());
  EXPECT_STREQ(msg.get_txt(), "Foo bar");
}

TEST(FieldString, deserialize_error_invalid_wiretype) 
{
  InSequence s;

  text<10> msg;
  Mocks::ReadBufferMock buffer;

  // The first byte is an invalid wiretype
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x09), Return(true)));
  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.txt().get_length());
}

TEST(FieldString, oneof_serialize)
{
  InSequence s;

  string_or_bytes<10, 10> msg;
  Mocks::WriteBufferMock buffer;

  msg.mutable_txt() = "Foo bar";

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(99));

  uint8_t expected[] = {0x0a, 0x07};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  EXPECT_CALL(buffer, push(_, 7)).Times(1).WillOnce(Return(true));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.txt().get_max_length());
}

TEST(FieldString, oneof_deserialize) 
{
  InSequence s;

  string_or_bytes<10, 10> msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.txt().get_length());
  EXPECT_STREQ(msg.get_txt(), "Foo bar");
}

TEST(FieldBytes, set_get)
{
  raw_bytes<10> msg;
  msg.mutable_b()[0] = 1;
  EXPECT_EQ(1, msg.b().get_length());
  EXPECT_EQ(1, msg.b().get_const(0));

  msg.clear();
  EXPECT_EQ(0, msg.b().get_length());
  EXPECT_EQ(0, msg.b().get_const(0));

  msg.mutable_b()[1] = 2;
  EXPECT_EQ(2, msg.b().get_length());
  EXPECT_EQ(0, msg.b().get_const(0));
  EXPECT_EQ(2, msg.b().get_const(1));

  // Check index out of bound will return the last element.
  msg.mutable_b()[10] = 11; // max index should be 9.
  // The last element should be changed
  EXPECT_EQ(10, msg.b().get_length());
  EXPECT_EQ(11, msg.b().get_const(9));
  // Check this function out of bound aswell.
  EXPECT_EQ(11, msg.b().get_const(10));

  // Try to set more bytes compared to what will fit.
  uint8_t big_array[11] = {0};
  big_array[10] = 11;
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.mutable_b().set(big_array, 11));
}

TEST(FieldBytes, clear)
{
  raw_bytes<10> msg;  
  
  const uint8_t array[2] = {1 ,2};

  // Clear the field specific.
  msg.mutable_b().set(array, 2);
  EXPECT_EQ(2, msg.b().get_length());
  msg.clear_b();
  EXPECT_EQ(0, msg.b().get_length());

  // Clear the whole message.
  msg.mutable_b().set(array, 2);
  msg.clear();
  EXPECT_EQ(0, msg.b().get_length());
}

TEST(FieldBytes, serialize)
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::WriteBufferMock buffer;

  uint8_t bytes[] = {1u, 2u, 3u, 0u};
  msg.mutable_b().set(bytes, 4);

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  uint8_t expected[] = {0x0a, 0x04};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  EXPECT_CALL(buffer, push(_, 4)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.b().get_max_length());
}

TEST(FieldBytes, deserialize) 
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0x0a, 0x04, 0x01, 0x02, 0x03, 0x00};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(4, msg.b().get_length());
  EXPECT_EQ(1, msg.b()[0]);
  EXPECT_EQ(2, msg.b()[1]);
  EXPECT_EQ(3, msg.b()[2]);
  EXPECT_EQ(0, msg.b()[3]);
}

TEST(FieldBytes, deserialize_error_invalid_wiretype) 
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::ReadBufferMock buffer;

  // The first byte is an invalid wiretype
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x09), Return(true)));
  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.b().get_length());
}

TEST(FieldBytes, oneof_set_get)
{
  string_or_bytes<10, 10> msg;  
  msg.mutable_txt() = "Foo Bar";
  
  auto id = string_or_bytes<10, 10>::id::TXT;
  EXPECT_EQ(id, msg.get_which_s_or_b());
  EXPECT_STREQ(msg.get_txt(), "Foo Bar");

  // Switch to the array
  uint8_t array[] = {1, 2, 3, 4, 5};
  msg.mutable_b().set(array, 5);

  id = string_or_bytes<10, 10>::id::B;
  EXPECT_EQ(id, msg.get_which_s_or_b());
  for(uint8_t i = 0; i < 5; ++i)
  {
    EXPECT_EQ(i+1, msg.get_b()[i]);
  }
}

TEST(FieldBytes, oneof_clear)
{
  raw_bytes<10> msg;  
  
  const uint8_t array[2] = {1 ,2};

  // Clear the field specific.
  msg.mutable_b().set(array, 2);
  EXPECT_EQ(2, msg.b().get_length());
  msg.clear_b();
  EXPECT_EQ(0, msg.b().get_length());

  // Clear the whole message.
  msg.mutable_b().set(array, 2);
  msg.clear();
  EXPECT_EQ(0, msg.b().get_length());
}

TEST(FieldBytes, oneof_serialize)
{
  InSequence s;

  string_or_bytes<10, 10> msg;
  Mocks::WriteBufferMock buffer;

  uint8_t bytes[] = {1u, 2u, 3u, 0u};
  msg.mutable_b().set(bytes, 4);

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  uint8_t expected[] = {0x12, 0x04};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  EXPECT_CALL(buffer, push(_, 4)).Times(1).WillOnce(Return(true));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.txt().get_max_length());
}

TEST(FieldBytes, oneof_deserialize) 
{
  InSequence s;

  string_or_bytes<10, 10> msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0x12, 0x04, 0x01, 0x02, 0x03, 0x00};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(4, msg.b().get_length());
  EXPECT_EQ(1, msg.b()[0]);
  EXPECT_EQ(2, msg.b()[1]);
  EXPECT_EQ(3, msg.b()[2]);
  EXPECT_EQ(0, msg.b()[3]);
}

} // End of namespace test_EmbeddedAMS_string_bytes