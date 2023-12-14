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
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>
#include <limits>
#include <array>
#include <string.h>

// EAMS message definitions
#include <string_bytes.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::ElementsAre;
using ::testing::DoAll;

namespace test_EmbeddedAMS_string_bytes
{

TEST(FieldString, get_set)
{
  text<10> msg;
  
  // Test directly assigning a static string.
  msg.mutable_txt() = "Foo Bar";
  EXPECT_EQ(7, msg.get_txt().get_length());
  ASSERT_STREQ("Foo Bar", msg.get_txt().get_const());

  // Test using the string inding.
  msg.mutable_txt()[0] = 'f';
  msg.mutable_txt()[4] = 'b';
  EXPECT_EQ(7, msg.get_txt().get_length());
  ASSERT_STREQ("foo bar", msg.get_txt().get_const());

  // Test extending the string length by means of the non const get function.
  msg.mutable_txt().get(7) = ' ';
  msg.mutable_txt().get(8) = '2';
  EXPECT_EQ(9, msg.get_txt().get_length());
  ASSERT_STREQ("foo bar 2", msg.get_txt().get_const());

  // Test assigning a string by array pointer with max length.
  char text_3[] = "Foo bar 3!";
  msg.mutable_txt() = text_3;
  EXPECT_EQ(10, msg.get_txt().get_length());
  ASSERT_STREQ("Foo bar 3!", msg.get_txt().get_const());

  const char* text2 = msg.get_txt().get_const();
  ASSERT_STREQ("Foo bar 3!", text2);

  // Test the set function.
  char text_4[] = "Foo bar 4!";
  msg.mutable_txt().set(text_4);
  EXPECT_EQ(10, msg.get_txt().get_length());
  ASSERT_STREQ("Foo bar 4!", msg.get_txt().get_const());

  // Test setting strings using a pointer. 
  msg.clear();
  char* text_5_p;
  char text_5[] = "Foo bar";
  text_5_p = &(text_5[0]);
  msg.mutable_txt() = text_5_p;
  EXPECT_EQ(7, msg.get_txt().get_length());
  ASSERT_STREQ("Foo bar", msg.get_txt().get_const());

  msg.clear();
  msg.mutable_txt().set(text_5_p);
  EXPECT_EQ(7, msg.get_txt().get_length());
  ASSERT_STREQ("Foo bar", msg.get_txt().get_const());

  // Use a static string with the set function
  msg.clear();
  msg.mutable_txt().set("Foo bar 6");
  EXPECT_EQ(9, msg.get_txt().get_length());
  ASSERT_STREQ("Foo bar 6", msg.get_txt().get_const());

  // Set an array which is longer
  // Asignment operator
  msg.clear();
  msg.mutable_txt() = "12345678901234567890";
  EXPECT_EQ(10, msg.get_txt().get_length());
  ASSERT_STREQ("1234567890", msg.get_txt().get_const());

  // Set function
  msg.clear();
  msg.mutable_txt().set("12345678901234567890");
  EXPECT_EQ(10, msg.get_txt().get_length());
  ASSERT_STREQ("1234567890", msg.get_txt().get_const());
}

TEST(FieldString, set_smaller)
{
  text<10> msgA;
  text<5> msgB;
  
  msgA.mutable_txt().set("Foo bar");

  // Asignment operator, too big.
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msgB.mutable_txt().set(msgA.get_txt()));
  EXPECT_EQ(0, msgB.get_txt().get_length());

  // Use the assignment operator with too much characters.
  // The error will not be visible but the effect of not setting msgB will!
  msgB.mutable_txt() = msgA.get_txt();
  EXPECT_EQ(0, msgB.get_txt().get_length());

  // Set a suitable amount of characters in the lager string.
  msgA.clear();
  msgA.mutable_txt().set("1234"); // Leave one space for the null terminator.
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msgB.mutable_txt().set(msgA.get_txt()));
  EXPECT_EQ(4, msgB.get_txt().get_length());
  ASSERT_STREQ("1234", msgB.get_txt().get_const());
  
  // Use the asignment operator with something that fits.
  msgB.clear();
  msgB.mutable_txt() = msgA.get_txt();
  EXPECT_EQ(4, msgB.get_txt().get_length());
  ASSERT_STREQ("1234", msgB.get_txt().get_const());
}

TEST(FieldString, set_larger)
{
  text<10> msgA;
  text<15> msgB;

  msgA.mutable_txt().set("Foo bar");

  // Use the set function
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msgB.mutable_txt().set(msgA.get_txt()));
  EXPECT_EQ(7, msgB.get_txt().get_length());
  ASSERT_STREQ("Foo bar", msgB.get_txt().get_const());

  // Use the assignment operator.
  msgB.clear();
  msgB.mutable_txt() = msgA.get_txt();
  EXPECT_EQ(7, msgB.get_txt().get_length());
  ASSERT_STREQ("Foo bar", msgB.get_txt().get_const());
}

TEST(FieldString, clear)
{
  text<10> msg;  
  
  // Clear the field specific.
  msg.mutable_txt() = "Foo Bar";
  EXPECT_EQ(7, msg.get_txt().get_length());
  msg.clear_txt();
  EXPECT_EQ(0, msg.get_txt().get_length());
  ASSERT_STREQ("", msg.get_txt().get_const());

  // Clear the whole message.
  msg.mutable_txt() = "Foo Bar";
  msg.clear();
  EXPECT_EQ(0, msg.get_txt().get_length());
  ASSERT_STREQ("", msg.get_txt().get_const());

  // Assign a nullptr to clear.
  msg.mutable_txt() = "Foo Bar";
  msg.mutable_txt() = nullptr;
  EXPECT_EQ(0, msg.get_txt().get_length());
  ASSERT_STREQ("", msg.get_txt().get_const());
}

TEST(FieldString, serialize) 
{
  InSequence s;

  text<10> msg;
  Mocks::WriteBufferMock buffer;

  char text[] = "Foo bar";
  msg.mutable_txt() = text;

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  std::array<uint8_t, 2> expected = {0x0a, 0x07};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  EXPECT_CALL(buffer, push(_, 7)).Times(1).WillOnce(Return(true));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_txt().get_max_length());
}

TEST(FieldString, serialize_buffer_full) 
{
  InSequence s;

  text<10> msg;
  Mocks::WriteBufferMock buffer;

  char text[] = "Foo bar";
  msg.mutable_txt() = text;

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(5));

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, msg.serialize(buffer));
}

TEST(FieldString, deserialize) 
{
  InSequence s;

  text<10> msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 9> referee = {0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "Foo bar");
}

TEST(FieldString, deserialize_error_invalid_wiretype) 
{
  InSequence s;

  text<10> msg;
  Mocks::ReadBufferMock buffer;

  // The first byte is an invalid wiretype
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x09), Return(true)));
  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.get_txt().get_length());
}

TEST(FieldString, deserialize_array_full) 
{
  InSequence s;

  text<3> msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 2> referee = {0x0a, 0x07};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }


  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "");
}

TEST(FieldString, deserialize_end_of_buffer) 
{
  InSequence s;

  text<10> msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 7> referee = {0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
  EXPECT_EQ(5, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "Foo b");
}


TEST(FieldString, oneof_serialize)
{
  InSequence s;

  string_or_bytes<3, 3, 10, 10> msg;
  Mocks::WriteBufferMock buffer;

  msg.mutable_txt() = "Foo bar";

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillRepeatedly(Return(99));

  // The tag and number of characters.
  std::array<uint8_t, 2> expected = {0x0a, 0x07};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // The actual data but it does not matter what as long as there are seven characters.
  EXPECT_CALL(buffer, push(_, 7)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_txt().get_max_length());
}

TEST(FieldString, oneof_deserialize) 
{
  InSequence s;

  string_or_bytes<3, 3, 10, 10> msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 9> referee = {0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "Foo bar");
}

TEST(FieldBytes, set_get)
{
  raw_bytes<10> msg;
  msg.mutable_b()[0] = 1;
  EXPECT_EQ(1, msg.get_b().get_length());
  EXPECT_EQ(1, msg.get_b().get_const(0));

  uint8_t value = 0;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.get_b().get_const(0, value));
  EXPECT_EQ(1, value);

  value = 99;
  EXPECT_EQ(::EmbeddedProto::Error::INDEX_OUT_OF_BOUND, msg.get_b().get_const(1, value));
  // Value should not have changed.
  EXPECT_EQ(99, value);


  msg.clear();
  EXPECT_EQ(0, msg.get_b().get_length());
  EXPECT_EQ(0, msg.get_b().get_const(0));

  msg.mutable_b()[1] = 2;
  EXPECT_EQ(2, msg.get_b().get_length());
  EXPECT_EQ(0, msg.get_b().get_const(0));
  EXPECT_EQ(2, msg.get_b().get_const(1));

  // Check index out of bound will return the last element.
  msg.mutable_b()[10] = 11; // max index should be 9.
  // The last element should be changed
  EXPECT_EQ(10, msg.get_b().get_length());
  EXPECT_EQ(11, msg.get_b().get_const(9));
  // Check this function out of bound aswell.
  EXPECT_EQ(11, msg.get_b().get_const(10));

  // Try to set more bytes compared to what will fit.
  uint8_t big_array[11] = {0};
  big_array[10] = 11;
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.mutable_b().set(big_array, 11));

  // Expect an error when setting more bytes in a smaller message.
  raw_bytes<5> msgB;
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msgB.mutable_b().set(msg.get_b()));

  // Set the full array when it fits.
  msg.clear();
  for(uint8_t i = 0; i < msgB.get_b().get_max_length(); ++i)
  {
    msg.mutable_b()[i] = i;
  }
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msgB.mutable_b().set(msg.get_b()));
  EXPECT_EQ(msgB.get_b().get_length(), msg.get_b().get_length());

  for(uint8_t i = 0; i < msgB.mutable_b().get_max_length(); ++i) {
    EXPECT_EQ(i, msgB.get_b()[i]);
  }

  // Use the assignment operator .
  msgB.clear();
  msgB.mutable_b() = msg.get_b();
  EXPECT_EQ(msgB.get_b().get_length(), msg.get_b().get_length());

  for(uint8_t i = 0; i < msgB.mutable_b().get_max_length(); ++i) {
    EXPECT_EQ(i, msgB.get_b()[i]);
  }

  // Now with a message destination which has more element compared to the source.
  raw_bytes<15> msgC;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msgC.mutable_b().set(msg.get_b()));
  EXPECT_EQ(msgC.get_b().get_length(), msg.get_b().get_length());

  for(uint8_t i = 0; i < msgC.mutable_b().get_length(); ++i) {
    EXPECT_EQ(i, msgC.get_b()[i]);
  }

  // Use the assignment operator.
  msgC.clear();
  msgC.mutable_b() = msg.get_b();
  EXPECT_EQ(msgC.get_b().get_length(), msg.get_b().get_length());

  for(uint8_t i = 0; i < msgC.mutable_b().get_length(); ++i) {
    EXPECT_EQ(i, msgC.get_b()[i]);
  }
}

TEST(FieldBytes, assign_msg) 
{
  raw_bytes<10> msgA;
  raw_bytes<10> msgB;
  const std::array<uint8_t, 10> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  msgA.mutable_b().set(data.data(), 10);
  msgB = msgA;

  for(uint8_t i = 0; i < msgB.mutable_b().get_max_length(); ++i) {
    EXPECT_EQ(data[i], msgB.get_b()[i]);
  }
}

TEST(FieldBytes, clear)
{
  raw_bytes<10> msg;  
  
  const std::array<uint8_t, 2> array = {1 ,2};

  // Clear the field specific.
  msg.mutable_b().set(array.data(), 2);
  EXPECT_EQ(2, msg.get_b().get_length());
  msg.clear_b();
  EXPECT_EQ(0, msg.get_b().get_length());

  // Clear the whole message.
  msg.mutable_b().set(array.data(), 2);
  msg.clear();
  EXPECT_EQ(0, msg.get_b().get_length());
}

TEST(FieldBytes, serialize)
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::WriteBufferMock buffer;

  std::array<uint8_t, 4> bytes = {1u, 2u, 3u, 0u};
  msg.mutable_b().set(bytes.data(), 4);

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  std::array<uint8_t, 2> expected = {0x0a, 0x04};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  EXPECT_CALL(buffer, push(_, 4)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_b().get_max_length());
}

TEST(FieldBytes, deserialize) 
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 6> referee = {0x0a, 0x04, 0x01, 0x02, 0x03, 0x00};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(4, msg.get_b().get_length());
  EXPECT_EQ(1, msg.get_b()[0]);
  EXPECT_EQ(2, msg.get_b()[1]);
  EXPECT_EQ(3, msg.get_b()[2]);
  EXPECT_EQ(0, msg.get_b()[3]);
}

TEST(FieldBytes, deserialize_error_invalid_wiretype) 
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::ReadBufferMock buffer;

  // The first byte is an invalid wiretype
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x09), Return(true)));
  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.get_b().get_length());
}

TEST(FieldBytes, oneof_set_get)
{
  string_or_bytes<3, 3, 10, 10> msg;  
  msg.mutable_txt() = "Foo Bar";
  
  auto id = string_or_bytes<3, 3, 10, 10>::FieldNumber::TXT;
  EXPECT_EQ(id, msg.get_which_s_or_b());
  EXPECT_STREQ(msg.txt(), "Foo Bar");

  // Switch to the array
  std::array<uint8_t, 5> array = {1, 2, 3, 4, 5};
  msg.mutable_b().set(array.data(), 5);

  id = string_or_bytes<3, 3, 10, 10>::FieldNumber::B;
  EXPECT_EQ(id, msg.get_which_s_or_b());
  for(uint8_t i = 0; i < 5; ++i)
  {
    EXPECT_EQ(i+1, msg.get_b()[i]);
  }
}

TEST(FieldBytes, oneof_clear)
{
  raw_bytes<10> msg;  
  
  const std::array<uint8_t, 2> array = {1 ,2};

  // Clear the field specific.
  msg.mutable_b().set(array.data(), 2);
  EXPECT_EQ(2, msg.get_b().get_length());
  msg.clear_b();
  EXPECT_EQ(0, msg.get_b().get_length());

  // Clear the whole message.
  msg.mutable_b().set(array.data(), 2);
  msg.clear();
  EXPECT_EQ(0, msg.get_b().get_length());
}

TEST(FieldString, oneof_assign)
{ 
  string_or_bytes<3, 3, 10, 10> msgA;
  string_or_bytes<3, 3, 10, 10> msgB;

  msgA.mutable_txt() = "Foo Bar";
  msgB = msgA;

  auto id = string_or_bytes<3, 3, 10, 10>::FieldNumber::TXT;
  EXPECT_EQ(id, msgB.get_which_s_or_b());
  EXPECT_STREQ(msgB.txt(), "Foo Bar");
}

TEST(FieldBytes, oneof_serialize)
{
  InSequence s;

  string_or_bytes<3, 3, 10, 10> msg;
  Mocks::WriteBufferMock buffer;

  std::array<uint8_t, 4> bytes = {1u, 2u, 3u, 0u};
  msg.mutable_b().set(bytes.data(), 4);

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillRepeatedly(Return(17));

  // The tag and size
  std::array<uint8_t, 2> expected = {0x12, 0x04};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // The actual data but it does not matter what as long as there are four bytes.
  EXPECT_CALL(buffer, push(_, 4)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_txt().get_max_length());
}

TEST(FieldBytes, oneof_deserialize) 
{
  InSequence s;

  string_or_bytes<3, 3, 10, 10> msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 6> referee = {0x12, 0x04, 0x01, 0x02, 0x03, 0x00};

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));


  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(4, msg.get_b().get_length());
  EXPECT_EQ(1, msg.get_b()[0]);
  EXPECT_EQ(2, msg.get_b()[1]);
  EXPECT_EQ(3, msg.get_b()[2]);
  EXPECT_EQ(0, msg.get_b()[3]);
}

TEST(RepeatedStringBytes, empty) 
{ 
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  Mocks::WriteBufferMock buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedStringBytes, get_set) 
{ 
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;

  ::EmbeddedProto::FieldString<15> str;
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(0) = "Foo bar 1";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(1) = "Foo bar 2";

  str = "Foo bar 3";
  msg.add_array_of_txt(str);
  
  EXPECT_EQ(3, msg.array_of_txt().get_length());
  EXPECT_EQ(0, msg.array_of_bytes().get_length());
  EXPECT_STREQ(msg.array_of_txt(0).get_const(), "Foo bar 1");
  EXPECT_STREQ(msg.array_of_txt(1).get_const(), "Foo bar 2");
  EXPECT_STREQ(msg.array_of_txt(2).get_const(), "Foo bar 3");
}

TEST(RepeatedStringBytes, assign_msg) 
{ 
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msgA;
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msgB;

  ::EmbeddedProto::FieldString<15> str;
  msgA.add_array_of_txt(str);
  msgA.mutable_array_of_txt(0) = "Foo bar 1";
  msgA.add_array_of_txt(str);
  msgA.mutable_array_of_txt(1) = "Foo bar 2";

  str = "Foo bar 3";
  msgA.add_array_of_txt(str);


  ::EmbeddedProto::FieldBytes<15> bytes;
  bytes[0] = 1;
  msgA.add_array_of_bytes(bytes);
  bytes[1] = 2;
  msgA.add_array_of_bytes(bytes);
   
  msgB = msgA;

  EXPECT_EQ(3, msgB.array_of_txt().get_length());
  EXPECT_EQ(2, msgB.array_of_bytes().get_length());
  EXPECT_STREQ(msgB.array_of_txt(0).get_const(), "Foo bar 1");
  EXPECT_STREQ(msgB.array_of_txt(1).get_const(), "Foo bar 2");
  EXPECT_STREQ(msgB.array_of_txt(2).get_const(), "Foo bar 3");

  EXPECT_EQ(1, msgB.array_of_bytes()[0].get_length());
  EXPECT_EQ(1, msgB.array_of_bytes()[0][0]);

  EXPECT_EQ(2, msgB.array_of_bytes()[1].get_length());
  EXPECT_EQ(1, msgB.array_of_bytes()[1][0]);
  EXPECT_EQ(2, msgB.array_of_bytes()[1][1]); 


}

TEST(RepeatedStringBytes, serialize) 
{ 
  InSequence s;

  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  Mocks::WriteBufferMock buffer;

  ::EmbeddedProto::FieldString<15> str;
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(0) = "Foo bar 1";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(1) = "";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(2) = "Foo bar 3";

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(24));

  // The first string.
  // Id and size of array of txt.
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x09)).Times(1).WillOnce(Return(true));

  // The string is pushed as an array, we do not know the pointer value so use _, but we do know 
  // the size.
  EXPECT_CALL(buffer, push(_, 9)).Times(1).WillOnce(Return(true));
  

  // The empty string
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));
  
  // The last string 
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x09)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, push(_, 9)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(0));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedStringBytes, deserialize) 
{ 
  InSequence s;

  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  Mocks::ReadBufferMock buffer;

  // Pop the tag and size of the first string
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x0a), Return(true)));
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x09), Return(true)));

  std::array<uint8_t, 9> referee_str1 = {0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72, 0x20, 0x31};

  for(auto r: referee_str1) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }

  // Pop the tag and size of the second string
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x0a), Return(true)));
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x00), Return(true)));

  // Pop the tag and size of the third string
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x0a), Return(true)));
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x09), Return(true)));

  std::array<uint8_t, 9> referee_str3 = {0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72, 0x20, 0x33};

  for(auto r: referee_str3) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }

  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(3, msg.array_of_txt().get_length());
  EXPECT_EQ(0, msg.array_of_bytes().get_length());
  EXPECT_STREQ(msg.array_of_txt(0).get_const(), "Foo bar 1");
  EXPECT_STREQ(msg.array_of_txt(1).get_const(), "");
  EXPECT_STREQ(msg.array_of_txt(2).get_const(), "Foo bar 3"); 
}

TEST(RepeatedStringBytes, field_number_to_name)
{
  using RSB = repeated_string_bytes<3, 15, 3, 15, 3, 3>;
  RSB msg;

  EXPECT_TRUE(0 == strcmp(RSB::field_number_to_name(RSB::FieldNumber::ARRAY_OF_TXT),
                          "array_of_txt"));

  EXPECT_TRUE(0 == strcmp(RSB::field_number_to_name(RSB::FieldNumber::NESTED_BYTES),
                          "nested_bytes"));
}

#ifdef MSG_TO_STRING

TEST(RepeatedStringBytes, to_string)
{
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;

  ::EmbeddedProto::FieldString<15> field_str;
  msg.add_array_of_txt(field_str);
  msg.mutable_array_of_txt(0) = "Foo bar 1";
  msg.add_array_of_txt(field_str);
  msg.mutable_array_of_txt(1) = "";
  msg.add_array_of_txt(field_str);
  msg.mutable_array_of_txt(2) = "Foo bar 3";

  ::EmbeddedProto::FieldBytes<15> data_field;
  for(uint8_t i = 0; i < 10; ++i) {
    data_field[i] = i;
  }
  msg.mutable_array_of_bytes().add(data_field);

  for(uint8_t i = 0; i < 10; ++i) {
    data_field[i] = i + 5;
  }
  msg.mutable_array_of_bytes().add(data_field);

  for(uint8_t i = 0; i < 10; ++i) {
    data_field[i] = i + 10;
  }
  msg.mutable_array_of_bytes().add(data_field);

  msg.mutable_nested_text().mutable_txt() = "A.B";

  const std::array<uint8_t, 3> b = {1, 2, 3};
  msg.mutable_nested_bytes().mutable_b().set(b.data(), 3); 

  constexpr uint32_t N = 2048;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);
  
  //std::cout << std::endl << str << std::endl;

  constexpr uint32_t TXT_LEN = 1274;
  const char expected_str[TXT_LEN + 1] = "{\n  \"array_of_txt\": [\n                    \"Foo bar 1\",\n                    \"\",\n                    \"Foo bar 3\"\n                  ],\n  \"array_of_bytes\": [\n                      [\n                        0,\n                        1,\n                        2,\n                        3,\n                        4,\n                        5,\n                        6,\n                        7,\n                        8,\n                        9\n                      ],\n                      [\n                        5,\n                        6,\n                        7,\n                        8,\n                        9,\n                        10,\n                        11,\n                        12,\n                        13,\n                        14\n                      ],\n                      [\n                        10,\n                        11,\n                        12,\n                        13,\n                        14,\n                        15,\n                        16,\n                        17,\n                        18,\n                        19\n                      ]\n                    ],\n  \"nested_text\": {\n    \"txt\": \"A.B\"\n  },\n  \"nested_bytes\": {\n    \"b\": [\n           1,\n           2,\n           3\n         ]\n  }\n}"; 
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);
  EXPECT_EQ(str + TXT_LEN, str_left.data);
}

#endif // MSG_TO_STRING

} // End of namespace test_EmbeddedAMS_string_bytes