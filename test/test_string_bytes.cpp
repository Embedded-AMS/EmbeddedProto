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
#include "gmock/gmock.h"

#include <WireFormatter.h>
#include <ReadBufferFixedSize.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
#include <WriteBufferFixedSize.h>
#include <MessageState.h>

#include <cstdint>
#include <limits>
#include <array>
#include <string.h>
#include <vector>

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

  std::array<uint8_t, 2> expected = {0x0a, 0x07};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

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

  std::array<uint8_t, 2> expected = {0x0a, 0x07};
  for(auto e : expected)
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(5));

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, msg.serialize(buffer));
}

TEST(FieldString, deserialize) 
{
  text<10> msg;
 
  ::EmbeddedProto::ReadBufferFixedSize<9> buffer({0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72});

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "Foo bar");
}

#ifdef PARTIAL_DESERIALIZATION_ENABLED

TEST(FieldString, deserialize_partial_in_data) 
{
  text<10> msg;
 
  ::EmbeddedProto::ReadBufferFixedSize<9> buffer({0x0a, 0x07, 0x46, 0x6f, 0x6f}); // Split in the data

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0x20);
  buffer.push(0x62);
  buffer.push(0x61);
  buffer.push(0x72);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "Foo bar");
}

TEST(FieldString, deserialize_partial_before_and_in_size) 
{
  text<140> msg;
 
  ::EmbeddedProto::ReadBufferFixedSize<150> buffer({0x0a});   // Field tag 

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0x8c); // First byte of length  

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0x01); // Second byte of the length

  for( uint32_t i = 0; i < 20; ++i)
  {
    buffer.push(0x46); 
    buffer.push(0x6f);
    buffer.push(0x6f);
    buffer.push(0x20);
    buffer.push(0x62);
    buffer.push(0x61);
    buffer.push(0x72);
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(140, msg.get_txt().get_length());
}

#endif // PARTIAL_DESERIALIZATION_ENABLED

TEST(FieldString, deserialize_error_invalid_wiretype) 
{
  InSequence s;

  text<10> msg;
  Mocks::ReadBufferMock buffer;

  // The first byte is an invalid wiretype
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(0x09), Return(true)));
  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.get_txt().get_length());
}

TEST(FieldString, deserialize_array_full) 
{
  text<3> msg;

  ::EmbeddedProto::ReadBufferFixedSize<6> buffer({0x0a, 0x04, 0x61, 0x62, 0x63, 0x64});

  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.deserialize(buffer));
  EXPECT_EQ(0, msg.get_txt().get_length());
  EXPECT_STREQ(msg.txt(), "");
}

TEST(FieldString, deserialize_end_of_buffer) 
{
  text<10> msg;

  ::EmbeddedProto::ReadBufferFixedSize<7> buffer({0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62});

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

  // The tag and number of characters.
  std::array<uint8_t, 2> expected = {0x0a, 0x07};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillRepeatedly(Return(99));

  // The actual data but it does not matter what as long as there are seven characters.
  EXPECT_CALL(buffer, push(_, 7)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_txt().get_max_length());
}

TEST(FieldString, oneof_deserialize) 
{
  string_or_bytes<3, 3, 10, 10> msg;

  ::EmbeddedProto::ReadBufferFixedSize<9> buffer({0x0a, 0x07, 0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72});

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

  std::array<uint8_t, 2> expected = {0x0a, 0x04};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  EXPECT_CALL(buffer, push(_, 4)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_b().get_max_length());
}

TEST(FieldBytes, deserialize) 
{
  raw_bytes<10> msg;
 
  ::EmbeddedProto::ReadBufferFixedSize<6> buffer({0x0a, 0x04, 0x01, 0x02, 0x03, 0x00});

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(4, msg.get_b().get_length());
  EXPECT_EQ(1, msg.get_b()[0]);
  EXPECT_EQ(2, msg.get_b()[1]);
  EXPECT_EQ(3, msg.get_b()[2]);
  EXPECT_EQ(0, msg.get_b()[3]);
}

#ifdef PARTIAL_DESERIALIZATION_ENABLED

TEST(FieldBytes, deserialize_partial) 
{
  raw_bytes<10> msg;
 
  ::EmbeddedProto::ReadBufferFixedSize<6> buffer({0x0a, 0x04, 0x01});

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0x02);
  buffer.push(0x03);
  buffer.push(0x00);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(4, msg.get_b().get_length());
  EXPECT_EQ(1, msg.get_b()[0]);
  EXPECT_EQ(2, msg.get_b()[1]);
  EXPECT_EQ(3, msg.get_b()[2]);
  EXPECT_EQ(0, msg.get_b()[3]);
}

#endif // PARTIAL_DESERIALIZATION_ENABLED

TEST(FieldBytes, deserialize_error_invalid_wiretype) 
{
  InSequence s;

  raw_bytes<10> msg;
  Mocks::ReadBufferMock buffer;

  // The first byte is an invalid wiretype
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(0x09), Return(true)));
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

  // The tag and size
  std::array<uint8_t, 2> expected = {0x12, 0x04};
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // get_available_size() is called after writing tag and size
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillRepeatedly(Return(17));

  // The actual data but it does not matter what as long as there are four bytes.
  EXPECT_CALL(buffer, push(_, 4)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(10, msg.get_txt().get_max_length());
}

TEST(FieldBytes, oneof_deserialize) 
{
  string_or_bytes<3, 3, 10, 10> msg;

  ::EmbeddedProto::ReadBufferFixedSize<6> buffer({0x12, 0x04, 0x01, 0x02, 0x03, 0x00});

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

  // The first string.
  // Id and size of array of txt.
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x09)).Times(1).WillOnce(Return(true));

  // get_available_size() is called after writing tag and size for first string
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(24));

  // The string is pushed as an array, we do not know the pointer value so use _, but we do know
  // the size.
  EXPECT_CALL(buffer, push(_, 9)).Times(1).WillOnce(Return(true));
  

  // The empty string
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  // get_available_size() is called after writing tag and size for empty string
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(15));

  // The last string
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x09)).Times(1).WillOnce(Return(true));

  // get_available_size() is called after writing tag and size for last string
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(9));

  EXPECT_CALL(buffer, push(_, 9)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedStringBytes, deserialize) 
{ 
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;

  ::EmbeddedProto::ReadBufferFixedSize<24> buffer( {
      0x0a, 0x09, // Pop the tag and size of the first string
      0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72, 0x20, 0x31,
      0x0a, 0x00, // Pop the tag and size of the second string
      0x0a, 0x09, // Pop the tag and size of the third string
      0x46, 0x6f, 0x6f, 0x20, 0x62, 0x61, 0x72, 0x20, 0x33,
    } );

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(3, msg.array_of_txt().get_length());
  EXPECT_EQ(0, msg.array_of_bytes().get_length());
  EXPECT_STREQ(msg.array_of_txt(0).get_const(), "Foo bar 1");
  EXPECT_STREQ(msg.array_of_txt(1).get_const(), "");
  EXPECT_STREQ(msg.array_of_txt(2).get_const(), "Foo bar 3"); 
}

#if (EP_SERIALIZATION_MODE_PARTIAL == EP_SERIALIZATION_MODE)

TEST(RepeatedStringBytes, deserialize_partial_repeated_string_split_size_and_data)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::FieldString<8>, 2> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<10> buffer({0x03, 'A'});

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, field.deserialize_partial_as_field(buffer, state));
  EXPECT_EQ(1, field.get_length());
  EXPECT_STREQ("A", field[0].get_const());

  buffer.push('B');
  buffer.push('C');

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, field.deserialize_partial_as_field(buffer, state));
  EXPECT_EQ(1, field.get_length());
  EXPECT_STREQ("ABC", field[0].get_const());
}

TEST(RepeatedStringBytes, deserialize_partial_repeated_bytes_split_size_and_data)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::FieldBytes<8>, 2> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<10> buffer({0x03, 0xAA});

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, field.deserialize_partial_as_field(buffer, state));
  EXPECT_EQ(1, field.get_length());
  EXPECT_EQ(1, field[0].get_length());
  EXPECT_EQ(0xAA, field[0].get_const(0));

  buffer.push(0xBB);
  buffer.push(0xCC);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, field.deserialize_partial_as_field(buffer, state));
  EXPECT_EQ(1, field.get_length());
  EXPECT_EQ(3, field[0].get_length());
  EXPECT_EQ(0xAA, field[0].get_const(0));
  EXPECT_EQ(0xBB, field[0].get_const(1));
  EXPECT_EQ(0xCC, field[0].get_const(2));
}

TEST(RepeatedStringBytes, deserialize_partial_repeated_string_element_boundary_continuation)
{
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;

  ::EmbeddedProto::ReadBufferFixedSize<11> buffer_a({
      0x0A, 0x09, 'F', 'o', 'o', ' ', 'b', 'a', 'r', ' ', '1'
    });
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer_a));
  EXPECT_EQ(1, msg.array_of_txt().get_length());
  EXPECT_STREQ("Foo bar 1", msg.array_of_txt(0).get_const());

  ::EmbeddedProto::ReadBufferFixedSize<11> buffer_b({
      0x0A, 0x09, 'F', 'o', 'o', ' ', 'b', 'a', 'r', ' ', '2'
    });
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer_b));
  EXPECT_EQ(2, msg.array_of_txt().get_length());
  EXPECT_STREQ("Foo bar 2", msg.array_of_txt(1).get_const());
}

TEST(RepeatedStringBytes, deserialize_partial_repeated_string_array_full)
{
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;

  ::EmbeddedProto::ReadBufferFixedSize<12> buffer({
      0x0A, 0x01, 'A',
      0x0A, 0x01, 'B',
      0x0A, 0x01, 'C',
      0x0A, 0x01, 'D'
    });

  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.deserialize(buffer));
  EXPECT_EQ(3, msg.array_of_txt().get_length());
  EXPECT_STREQ("A", msg.array_of_txt(0).get_const());
  EXPECT_STREQ("B", msg.array_of_txt(1).get_const());
  EXPECT_STREQ("C", msg.array_of_txt(2).get_const());
}

TEST(RepeatedStringBytes, deserialize_partial_repeated_bytes_array_full)
{
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;

  ::EmbeddedProto::ReadBufferFixedSize<12> buffer({
      0x12, 0x01, 0x01,
      0x12, 0x01, 0x02,
      0x12, 0x01, 0x03,
      0x12, 0x01, 0x04
    });

  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.deserialize(buffer));
  EXPECT_EQ(3, msg.array_of_bytes().get_length());
  EXPECT_EQ(1, msg.array_of_bytes(0).get_length());
  EXPECT_EQ(1, msg.array_of_bytes(1).get_length());
  EXPECT_EQ(1, msg.array_of_bytes(2).get_length());
  EXPECT_EQ(0x01, msg.array_of_bytes(0).get_const(0));
  EXPECT_EQ(0x02, msg.array_of_bytes(1).get_const(0));
  EXPECT_EQ(0x03, msg.array_of_bytes(2).get_const(0));
}

#endif // EP_SERIALIZATION_MODE_PARTIAL

#ifndef DISABLE_FIELD_NUMBER_TO_NAME 

TEST(RepeatedStringBytes, field_number_to_name)
{
  using RSB = repeated_string_bytes<3, 15, 3, 15, 3, 3>;
  RSB msg;

  EXPECT_TRUE(0 == strcmp(RSB::field_number_to_name(RSB::FieldNumber::ARRAY_OF_TXT),
                          "array_of_txt"));

  EXPECT_TRUE(0 == strcmp(RSB::field_number_to_name(RSB::FieldNumber::NESTED_BYTES),
                          "nested_bytes"));
}

#endif

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

  constexpr uint32_t TXT_LEN = 1274;
  const char expected_str[TXT_LEN + 1] = "{\n  \"array_of_txt\": [\n                    \"Foo bar 1\",\n                    \"\",\n                    \"Foo bar 3\"\n                  ],\n  \"array_of_bytes\": [\n                      [\n                        0,\n                        1,\n                        2,\n                        3,\n                        4,\n                        5,\n                        6,\n                        7,\n                        8,\n                        9\n                      ],\n                      [\n                        5,\n                        6,\n                        7,\n                        8,\n                        9,\n                        10,\n                        11,\n                        12,\n                        13,\n                        14\n                      ],\n                      [\n                        10,\n                        11,\n                        12,\n                        13,\n                        14,\n                        15,\n                        16,\n                        17,\n                        18,\n                        19\n                      ]\n                    ],\n  \"nested_text\": {\n    \"txt\": \"A.B\"\n  },\n  \"nested_bytes\": {\n    \"b\": [\n           1,\n           2,\n           3\n         ]\n  }\n}";
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);
  EXPECT_EQ(str + TXT_LEN, str_left.data);
}

#endif // MSG_TO_STRING

TEST(RepeatedStringWithLengths, test_both_lengths) {
  repeated_string_with_lengths msg;

  // The array should have exactly 3 elements
  ASSERT_EQ(3, msg.array_of_txt().get_max_length());

  // Each string should have a max length of 10
  msg.mutable_array_of_txt(0) = "1234567890";
  ASSERT_EQ(10, msg.array_of_txt(0).get_max_length());
  ASSERT_STREQ("1234567890", msg.array_of_txt(0).get_const());

  // Try to set a string longer than 10 characters - should be truncated
  msg.mutable_array_of_txt(1) = "1234567890123";
  ASSERT_EQ(10, msg.array_of_txt(1).get_length());
  ASSERT_STREQ("1234567890", msg.array_of_txt(1).get_const());

  EXPECT_EQ(3, msg.array_of_txt().get_max_length());
  EXPECT_EQ(10, msg.array_of_txt(0).get_max_length());
}

TEST(RepeatedBytesWithLengths, test_both_lengths) {
  repeated_bytes_with_lengths msg;

  // The array should have exactly 3 elements
  ASSERT_EQ(3, msg.array_of_bytes().get_max_length());

  // Each bytes field should have a max length of 10
  uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  msg.mutable_array_of_bytes(0).set(data, 10);
  ASSERT_EQ(10, msg.array_of_bytes(0).get_max_length());
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(i, msg.array_of_bytes(0)[i]);
  }

  // Try to set more bytes than the max length - should be truncated
  uint8_t big_data[15] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.mutable_array_of_bytes(1).set(big_data, 15));
  ASSERT_EQ(0, msg.array_of_bytes(1).get_length());

  // Try to set data within the max length - should work
  uint8_t small_data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.mutable_array_of_bytes(1).set(small_data, 10));
  ASSERT_EQ(10, msg.array_of_bytes(1).get_length());
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(i, msg.array_of_bytes(1)[i]);
  }

  // Clear the array first before setting data
  msg.clear_array_of_bytes();
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.mutable_array_of_bytes(1).set(small_data, 10));
  ASSERT_EQ(10, msg.array_of_bytes(1).get_length());
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(i, msg.array_of_bytes(1)[i]);
  }
  EXPECT_EQ(3, msg.array_of_bytes().get_max_length());
  EXPECT_EQ(10, msg.array_of_bytes(0).get_max_length());
}

// Test case 1: maxLength defined but nestedMaxLength not
// This should result in a C++ template parameter for the string/bytes
TEST(RepeatedStringMaxOnly, test_max_only) {
  // The array should have exactly 3 elements
  repeated_string_max_only<10> msg;
  ASSERT_EQ(3, msg.array_of_txt().get_max_length());

  // Each string should have a template parameter for length
  msg.mutable_array_of_txt(0) = "1234567890";
  ASSERT_EQ(10, msg.array_of_txt(0).get_length());
  
  // Compare byte by byte since get_const() doesn't return a null-terminated string
  const char* expected = "1234567890";
  const char* actual = msg.array_of_txt(0).get_const();
  for (uint32_t i = 0; i < 10; ++i) {
    ASSERT_EQ(expected[i], actual[i]);
  }
}

TEST(RepeatedBytesMaxOnly, test_max_only) {
  // The array should have exactly 3 elements
  repeated_bytes_max_only<10> msg;
  ASSERT_EQ(3, msg.array_of_bytes().get_max_length());

  // Each bytes field should have a template parameter for length
  uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  msg.mutable_array_of_bytes(0).set(data, 10);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(i, msg.array_of_bytes(0)[i]);
  }
}

// Test case 2: nestedMaxLength defined but maxLength not
// This should result in a C++ template parameter for the array
TEST(RepeatedStringNestedOnly, test_nested_only) {
  // The array should have a template parameter for length
  repeated_string_nested_only<10> msg;

  // Each string should have a max length of 10
  msg.mutable_array_of_txt(0) = "1234567890";
  ASSERT_EQ(10, msg.array_of_txt(0).get_length());

  // Compare byte by byte since get_const() doesn't return a null-terminated string
  const char* expected = "1234567890";
  const char* actual = msg.array_of_txt(0).get_const();
  for (uint32_t i = 0; i < 10; ++i) {
    ASSERT_EQ(expected[i], actual[i]);
  }
}

TEST(RepeatedBytesNestedOnly, test_nested_only) {
  // The array should have a template parameter for length
  repeated_bytes_nested_only<10> msg;

  // Each bytes field should have a max length of 10
  uint8_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  msg.mutable_array_of_bytes(0).set(data, 10);
  for (int i = 0; i < 10; ++i) {
    ASSERT_EQ(i, msg.array_of_bytes(0)[i]);
  }
}

} // End of namespace test_EmbeddedAMS_string_bytes

//==============================================================================
// Partial Serialization Tests for String and Bytes Fields
//==============================================================================

#if (EP_SERIALIZATION_MODE_PARTIAL == EP_SERIALIZATION_MODE)

TEST(FieldString, PartialSerialize_String_ShortText_SufficientBuffer)
{
  // Test 15.3.1: Partial serialization of a short string field with sufficient buffer
  text<10> msg;
  msg.mutable_txt() = "Foo bar";  // 7 characters

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, buffer.get_size());  // 1 tag + 1 size + 7 data bytes

  // Verify wire format: tag(0x0a) + size(0x07) + "Foo bar"
  std::array<uint8_t, 9> expected = {0x0a, 0x07, 'F', 'o', 'o', ' ', 'b', 'a', 'r'};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(FieldString, PartialSerialize_String_EmptyString)
{
  // Test 15.3.2: Verify empty string serialization behavior
  text<10> msg;
  msg.mutable_txt() = "";  // empty string

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());  // tag + size (0x0a, 0x00)

  // Verify wire format: tag(0x0a) + size(0x00)
  std::array<uint8_t, 2> expected = {0x0a, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(FieldString, PartialSerialize_String_SingleChar)
{
  // Test 15.3.3: Verify minimum non-empty string serialization
  text<10> msg;
  msg.mutable_txt() = "A";  // 1 character

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(3U, buffer.get_size());  // tag + size + 1 data byte

  // Verify wire format: tag(0x0a) + size(0x01) + 'A'
  std::array<uint8_t, 3> expected = {0x0a, 0x01, 0x41};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "Mismatch at byte " << i;
  }
}

TEST(FieldString, PartialSerialize_String_BufferTooSmallForTag)
{
  // Test 15.3.4: Verify rollback when buffer cannot hold even the tag
  text<10> msg;
  msg.mutable_txt() = "Foo bar";

  // Create a buffer with size 1 and make it appear full by filling it
  ::EmbeddedProto::WriteBufferFixedSize<1> bufferA;
  bufferA.push(0xFF);  // Fill the buffer to make it appear full
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(1U, bufferA.get_size());  // still contains the dummy byte

  // Second buffer with sufficient space should succeed
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, bufferB.get_size());
}

TEST(FieldString, PartialSerialize_String_BufferOnlyFitsTag)
{
  // Test 15.3.5: Tag+size are written atomically for length-delimited fields.
  text<10> msg;
  msg.mutable_txt() = "Foo bar";  // 7 chars, size fits in 1 byte

  // Buffer that fits tag only (1 byte), not size
  ::EmbeddedProto::WriteBufferFixedSize<1> bufferA;
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(0U, bufferA.get_size());  // No partial tag write

  // Second buffer with sufficient space should complete the field
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, bufferB.get_size());  // tag (1) + size (1) + data (7)
}

TEST(FieldString, PartialSerialize_String_SplitInData)
{
  // Test 15.3.6: Verify string data can span multiple buffers
  text<10> msg;
  msg.mutable_txt() = "Foo bar";  // 7 characters, total 9 bytes

  // Buffer A: fits tag + size + 3 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferA;
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(5U, bufferA.get_size());
  
  // Verify state
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.root().phase);
  EXPECT_EQ(4U, state.root().bytes_remaining);

  // Buffer B: fits remaining 4 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(4U, bufferB.get_size());
  
  // Verify complete data
  std::array<uint8_t, 5> expectedA = {0x0a, 0x07, 'F', 'o', 'o'};
  std::array<uint8_t, 4> expectedB = {' ', 'b', 'a', 'r'};
  
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]);
  }
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]);
  }
}

TEST(FieldString, PartialSerialize_String_SplitAfterTagSize)
{
  // Test 15.3.7: Verify split can occur exactly after tag+size, before any data
  text<10> msg;
  msg.mutable_txt() = "Foo bar";  // 7 characters

  // Buffer A: fits exactly tag + size
  ::EmbeddedProto::WriteBufferFixedSize<2> bufferA;
  text<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(2U, bufferA.get_size());
  
  // Verify state
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.root().phase);
  EXPECT_EQ(7U, state.root().bytes_remaining);

  // Buffer B: fits all data
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(7U, bufferB.get_size());
  
  // Verify data
  std::array<uint8_t, 2> expectedA = {0x0a, 0x07};
  std::array<uint8_t, 7> expectedB = {'F', 'o', 'o', ' ', 'b', 'a', 'r'};
  
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]);
  }
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]);
  }
}

TEST(FieldString, PartialSerialize_String_DataOneByteAtATime)
{
  // Test 15.3.8: Verify string can be serialized one byte at a time in DATA phase
  text<10> msg;
  msg.mutable_txt() = "ABC";  // 3 characters, total 5 bytes

  text<10>::StateStack state;
  std::array<uint8_t, 10> collected;
  uint32_t total = 0;

  // First buffer: tag + size
  ::EmbeddedProto::WriteBufferFixedSize<2> buf1;
  auto result = msg.serialize_partial(buf1, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  memcpy(&collected[total], buf1.get_data(), buf1.get_size());
  total += buf1.get_size();

  // Remaining bytes one at a time
  for(int i = 0; i < 3; ++i)
  {
    ::EmbeddedProto::WriteBufferFixedSize<1> buf;
    result = msg.serialize_partial(buf, state.root());
    memcpy(&collected[total], buf.get_data(), buf.get_size());
    total += buf.get_size();
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(5U, total);
  
  // Verify complete data
  std::array<uint8_t, 5> expected = {0x0a, 0x03, 'A', 'B', 'C'};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], collected[i]);
  }
}

TEST(FieldString, PartialSerialize_String_LargeString_MultipleBuffers)
{
  // Test 15.3.9: Verify large string serialization across many small buffers
  text<140> msg;
  std::string longText = "Foo bar ";
  for(int i = 0; i < 20; ++i) {
    longText += "Foo bar ";
  }
  // This creates 140 characters
  longText = longText.substr(0, 140);
  msg.mutable_txt() = longText.c_str();

  text<140>::StateStack state;
  std::vector<uint8_t> collected;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result)
  {
    ::EmbeddedProto::WriteBufferFixedSize<20> buf;
    result = msg.serialize_partial(buf, state.root());
    for(uint32_t i = 0; i < buf.get_size(); ++i)
    {
      collected.push_back(buf.get_data()[i]);
    }
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(143U, collected.size());  // 1 tag + 2 size + 140 data
  
  // Verify tag and size
  EXPECT_EQ(0x0a, collected[0]);  // tag
  EXPECT_EQ(0x8c, collected[1]);  // first byte of size (140)
  EXPECT_EQ(0x01, collected[2]);  // second byte of size (140)
}

TEST(FieldString, PartialSerialize_String_SizeVarintTwoBytes)
{
  // Test 15.3.10: Verify correct handling when size requires 2-byte varint
  text<140> msg;
  std::string longText(140, 'A');  // 140 characters
  msg.mutable_txt() = longText.c_str();

  ::EmbeddedProto::WriteBufferFixedSize<200> buffer;
  text<140>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(143U, buffer.get_size());  // 1 tag + 2 size + 140 data
  
  // Verify wire format starts correctly
  EXPECT_EQ(0x0a, buffer.get_data()[0]);  // tag
  EXPECT_EQ(0x8c, buffer.get_data()[1]);  // first byte of size (140)
  EXPECT_EQ(0x01, buffer.get_data()[2]);  // second byte of size (140)
}

TEST(FieldString, PartialSerialize_String_SizeVarintTwoBytes_SplitInSize)
{
  // Test 15.3.11: Verify rollback when buffer can hold tag but not complete size varint
  text<140> msg;
  std::string longText(140, 'A');  // 140 characters (size requires 2 bytes)
  msg.mutable_txt() = longText.c_str();

  // Buffer A: fits tag + 1 byte of size, but size needs 2
  ::EmbeddedProto::WriteBufferFixedSize<2> bufferA;
  text<140>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(0U, bufferA.get_size());  // rollback - tag+size must be atomic

  // Buffer B: sufficient
  ::EmbeddedProto::WriteBufferFixedSize<200> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(143U, bufferB.get_size());
}

// Bytes field tests
TEST(FieldBytes, PartialSerialize_Bytes_SufficientBuffer)
{
  // Test 15.3.12: Verify partial serialization of bytes field with sufficient buffer
  raw_bytes<10> msg;
  std::array<uint8_t, 4> data = {0x01, 0x02, 0x03, 0x00};
  msg.mutable_b().set(data.data(), 4);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  raw_bytes<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(6U, buffer.get_size());  // 1 tag + 1 size + 4 data bytes
  
  // Verify wire format: tag(0x0a) + size(0x04) + data
  std::array<uint8_t, 6> expected = {0x0a, 0x04, 0x01, 0x02, 0x03, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

TEST(FieldBytes, PartialSerialize_Bytes_EmptyBytes)
{
  // Test 15.3.13: Verify empty bytes field serialization behavior
  raw_bytes<10> msg;
  // empty bytes field

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  raw_bytes<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer.get_size());  // tag + size (0x0a, 0x00)
  
  // Verify wire format: tag(0x0a) + size(0x00)
  std::array<uint8_t, 2> expected = {0x0a, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

TEST(FieldBytes, PartialSerialize_Bytes_SplitInData)
{
  // Test 15.3.14: Verify bytes data can span multiple buffers
  raw_bytes<10> msg;
  std::array<uint8_t, 4> data = {0x01, 0x02, 0x03, 0x00};
  msg.mutable_b().set(data.data(), 4);

  // Buffer A: fits tag + size + 2 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<4> bufferA;
  raw_bytes<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(4U, bufferA.get_size());
  
  // Verify state
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.root().phase);
  EXPECT_EQ(2U, state.root().bytes_remaining);

  // Buffer B: fits remaining 2 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, bufferB.get_size());
  
  // Verify complete data
  std::array<uint8_t, 4> expectedA = {0x0a, 0x04, 0x01, 0x02};
  std::array<uint8_t, 2> expectedB = {0x03, 0x00};
  
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]);
  }
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]);
  }
}

TEST(FieldBytes, PartialSerialize_Bytes_BufferTooSmallForTagSize)
{
  // Test 15.3.15: Verify rollback when tag+size cannot fit
  raw_bytes<10> msg;
  std::array<uint8_t, 4> data = {0x01, 0x02, 0x03, 0x00};
  msg.mutable_b().set(data.data(), 4);

  // Buffer A: fits tag only (1 byte), not size
  ::EmbeddedProto::WriteBufferFixedSize<1> bufferA;
  raw_bytes<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(0U, bufferA.get_size());  // rollback

  // Buffer B: sufficient
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(6U, bufferB.get_size());
}

TEST(FieldBytes, PartialSerialize_Bytes_WithZeroBytes)
{
  // Test 15.3.26: Verify bytes field containing zero values serializes correctly
  raw_bytes<10> msg;
  std::array<uint8_t, 3> data = {0x00, 0x00, 0x00};
  msg.mutable_b().set(data.data(), 3);

  ::EmbeddedProto::WriteBufferFixedSize<10> buffer;
  raw_bytes<10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(5U, buffer.get_size());  // 1 tag + 1 size + 3 data bytes
  
  // Verify wire format: tag(0x0a) + size(0x03) + zero data
  std::array<uint8_t, 5> expected = {0x0a, 0x03, 0x00, 0x00, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

TEST(FieldString, PartialDeserializeAsField_SizeSplit)
{
  ::EmbeddedProto::FieldString<140> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<200> buffer(
    {
      0x8C
    });

  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::SIZE, state.phase);
  EXPECT_EQ(0U, field.get_length());

  buffer.push(0x01);
  for(uint32_t i = 0U; i < 20U; ++i)
  {
    buffer.push('A');
  }

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(120U, state.bytes_remaining);
  EXPECT_EQ(20U, field.get_length());

  for(uint32_t i = 0U; i < 120U; ++i)
  {
    buffer.push('A');
  }

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_EQ(140U, field.get_length());
}

TEST(FieldBytes, PartialDeserializeAsField_DataSplit)
{
  ::EmbeddedProto::FieldBytes<10> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer(
    {
      0x04, 0x01, 0x02
    });

  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(2U, state.bytes_remaining);
  EXPECT_EQ(2U, field.get_length());
  EXPECT_EQ(0x01, field.get_const(0));
  EXPECT_EQ(0x02, field.get_const(1));

  buffer.push(0x03);
  buffer.push(0x04);

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_EQ(4U, field.get_length());
  EXPECT_EQ(0x03, field.get_const(2));
  EXPECT_EQ(0x04, field.get_const(3));
}

TEST(FieldBytes, PartialDeserializeAsField_SizeSplit)
{
  ::EmbeddedProto::FieldBytes<140> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<200> buffer(
    {
      0x8C
    });

  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::SIZE, state.phase);
  EXPECT_EQ(0U, field.get_length());

  buffer.push(0x01);
  for(uint32_t i = 0U; i < 30U; ++i)
  {
    buffer.push(static_cast<uint8_t>(i));
  }

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(110U, state.bytes_remaining);
  EXPECT_EQ(30U, field.get_length());

  for(uint32_t i = 30U; i < 140U; ++i)
  {
    buffer.push(static_cast<uint8_t>(i));
  }

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_EQ(140U, field.get_length());
  EXPECT_EQ(0x00, field.get_const(0));
  EXPECT_EQ(0x1D, field.get_const(29));
  EXPECT_EQ(0x1E, field.get_const(30));
  EXPECT_EQ(0x8B, field.get_const(139));
}

TEST(FieldString, PartialDeserializeAsField_DataOneByteAtATime)
{
  ::EmbeddedProto::FieldString<8> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<2> buffer_a({0x05, 'H'});
  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer_a, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(4U, state.bytes_remaining);
  EXPECT_STREQ("H", field.get_const());

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_b({'E'});
  result = field.deserialize_partial_as_field(buffer_b, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(3U, state.bytes_remaining);

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_c({'L'});
  result = field.deserialize_partial_as_field(buffer_c, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(2U, state.bytes_remaining);

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_d({'L'});
  result = field.deserialize_partial_as_field(buffer_d, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(1U, state.bytes_remaining);

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_e({'O'});
  result = field.deserialize_partial_as_field(buffer_e, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_STREQ("HELLO", field.get_const());
}

TEST(FieldBytes, PartialDeserializeAsField_DataOneByteAtATime)
{
  ::EmbeddedProto::FieldBytes<8> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<2> buffer_a({0x05, 0x10});
  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer_a, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(4U, state.bytes_remaining);
  EXPECT_EQ(1U, field.get_length());
  EXPECT_EQ(0x10, field.get_const(0));

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_b({0x11});
  result = field.deserialize_partial_as_field(buffer_b, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(3U, state.bytes_remaining);

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_c({0x12});
  result = field.deserialize_partial_as_field(buffer_c, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(2U, state.bytes_remaining);

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_d({0x13});
  result = field.deserialize_partial_as_field(buffer_d, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(1U, state.bytes_remaining);

  ::EmbeddedProto::ReadBufferFixedSize<1> buffer_e({0x14});
  result = field.deserialize_partial_as_field(buffer_e, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
  EXPECT_EQ(5U, field.get_length());
  EXPECT_EQ(0x10, field.get_const(0));
  EXPECT_EQ(0x11, field.get_const(1));
  EXPECT_EQ(0x12, field.get_const(2));
  EXPECT_EQ(0x13, field.get_const(3));
  EXPECT_EQ(0x14, field.get_const(4));
}

// Oneof field tests
TEST(FieldString, PartialSerialize_Oneof_String_SufficientBuffer)
{
  // Test 15.3.16: Verify oneof string field serialization
  string_or_bytes<3, 3, 10, 10> msg;
  msg.mutable_txt() = "Foo bar";  // oneof selected

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  string_or_bytes<3, 3, 10, 10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(9U, buffer.get_size());  // 1 tag + 1 size + 7 data bytes
  
  // Verify wire format: tag(0x0a) + size(0x07) + "Foo bar"
  std::array<uint8_t, 9> expected = {0x0a, 0x07, 'F', 'o', 'o', ' ', 'b', 'a', 'r'};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

TEST(FieldString, PartialSerialize_Oneof_Bytes_SufficientBuffer)
{
  // Test 15.3.17: Verify oneof bytes field serialization
  string_or_bytes<3, 3, 10, 10> msg;
  std::array<uint8_t, 4> data = {0x01, 0x02, 0x03, 0x00};
  msg.mutable_b().set(data.data(), 4);  // oneof selected

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  string_or_bytes<3, 3, 10, 10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(6U, buffer.get_size());  // 1 tag + 1 size + 4 data bytes
  
  // Verify wire format: tag(0x12) + size(0x04) + data
  std::array<uint8_t, 6> expected = {0x12, 0x04, 0x01, 0x02, 0x03, 0x00};
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

TEST(FieldString, PartialSerialize_Oneof_String_SplitInData)
{
  // Test 15.3.18: Verify oneof string can span multiple buffers
  string_or_bytes<3, 3, 10, 10> msg;
  msg.mutable_txt() = "Foo bar";

  // Buffer A: fits tag + size + 3 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<5> bufferA;
  string_or_bytes<3, 3, 10, 10>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(5U, bufferA.get_size());
  
  // Verify state
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.root().phase);
  EXPECT_EQ(4U, state.root().bytes_remaining);

  // Buffer B: fits remaining 4 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(4U, bufferB.get_size());
  
  // Verify complete data
  std::array<uint8_t, 5> expectedA = {0x0a, 0x07, 'F', 'o', 'o'};
  std::array<uint8_t, 4> expectedB = {' ', 'b', 'a', 'r'};
  
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]);
  }
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]);
  }
}

TEST(FieldString, PartialSerialize_String_StateReset_SerializeTwice)
{
  // Test 15.3.23: Verify state can be reset and reused for string fields
  text<10> msg;
  msg.mutable_txt() = "Test";

  text<10>::StateStack state;

  // First serialization
  ::EmbeddedProto::WriteBufferFixedSize<10> buffer1;
  auto result = msg.serialize_partial(buffer1, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  
  // Reset state and clear buffer
  state.reset();
  ::EmbeddedProto::WriteBufferFixedSize<10> buffer2;
  
  // Second serialization
  result = msg.serialize_partial(buffer2, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  
  // Both buffers should contain identical data
  EXPECT_EQ(buffer1.get_size(), buffer2.get_size());
  for(uint32_t i = 0; i < buffer1.get_size(); ++i)
  {
    EXPECT_EQ(buffer1.get_data()[i], buffer2.get_data()[i]);
  }
}

TEST(FieldString, PartialSerialize_String_VerifyBytesRemainingTracking)
{
  // Test 15.3.24: Verify bytes_remaining in state correctly tracks progress
  text<20> msg;
  msg.mutable_txt() = "1234567890";  // 10 characters

  text<20>::StateStack state;

  // Buffer 1: tag + size only
  ::EmbeddedProto::WriteBufferFixedSize<2> buffer1;
  auto result = msg.serialize_partial(buffer1, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.root().phase);
  EXPECT_EQ(10U, state.root().bytes_remaining);

  // Buffer 2: 4 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<4> buffer2;
  result = msg.serialize_partial(buffer2, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.root().phase);
  EXPECT_EQ(6U, state.root().bytes_remaining);

  // Buffer 3: remaining 6 data bytes
  ::EmbeddedProto::WriteBufferFixedSize<10> buffer3;
  result = msg.serialize_partial(buffer3, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(0U, state.root().bytes_remaining);
}

TEST(FieldString, PartialSerialize_String_LoopSmallBuffers)
{
  // Test 15.3.25: Verify string serialization completes correctly with many small buffers
  text<100> msg;
  msg.mutable_txt() = "The quick brown fox jumps over the lazy dog";  // 43 characters

  text<100>::StateStack state;
  std::vector<uint8_t> collected;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result)
  {
    ::EmbeddedProto::WriteBufferFixedSize<8> buf;
    result = msg.serialize_partial(buf, state.root());
    for(uint32_t i = 0; i < buf.get_size(); ++i)
    {
      collected.push_back(buf.get_data()[i]);
    }
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(45U, collected.size());  // 1 tag + 1 size + 43 data
  
  // Verify tag and size
  EXPECT_EQ(0x0a, collected[0]);  // tag
  EXPECT_EQ(0x2b, collected[1]);  // size (43)
}

TEST(FieldString, PartialSerialize_String_MaxLength_SplitMultipleTimes)
{
  // Test 15.3.27: Verify string at maximum template length serializes correctly
  text<10> msg;
  msg.mutable_txt() = "1234567890";  // 10 characters, at max length

  text<10>::StateStack state;
  std::vector<uint8_t> collected;

  ::EmbeddedProto::Error result = ::EmbeddedProto::Error::BUFFER_FULL;
  while(::EmbeddedProto::Error::BUFFER_FULL == result)
  {
    ::EmbeddedProto::WriteBufferFixedSize<3> buf;
    result = msg.serialize_partial(buf, state.root());
    for(uint32_t i = 0; i < buf.get_size(); ++i)
    {
      collected.push_back(buf.get_data()[i]);
    }
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(12U, collected.size());  // 1 tag + 1 size + 10 data
}

// Repeated field tests
TEST(RepeatedStringBytes, PartialSerialize_RepeatedString_ThreeStrings_LargeBuffer)
{
  // Test 15.3.19: Verify repeated string field serialization with sufficient buffer
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  
  ::EmbeddedProto::FieldString<15> str;
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(0) = "Foo bar 1";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(1) = "";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(2) = "Foo bar 3";

  ::EmbeddedProto::WriteBufferFixedSize<50> buffer;
  repeated_string_bytes<3, 15, 3, 15, 3, 3>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(24U, buffer.get_size());  // Expected wire format: 24 bytes
  
  // Verify wire format
  std::array<uint8_t, 24> expected = {
    0x0a, 0x09, 'F', 'o', 'o', ' ', 'b', 'a', 'r', ' ', '1',  // String 1
    0x0a, 0x00,                                                  // Empty string
    0x0a, 0x09, 'F', 'o', 'o', ' ', 'b', 'a', 'r', ' ', '3'   // String 3
  };
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

TEST(RepeatedStringBytes, PartialSerialize_RepeatedString_SplitBetweenElements)
{
  // Test 15.3.20: Verify split occurs cleanly between repeated string elements
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  
  ::EmbeddedProto::FieldString<15> str;
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(0) = "Foo bar 1";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(1) = "Foo bar 2";

  // Buffer A: exactly fits first element
  ::EmbeddedProto::WriteBufferFixedSize<11> bufferA;
  repeated_string_bytes<3, 15, 3, 15, 3, 3>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(11U, bufferA.get_size());
  
  // Buffer B: fits second element
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(11U, bufferB.get_size());
  
  // Verify data
  std::array<uint8_t, 11> expectedA = {0x0a, 0x09, 'F', 'o', 'o', ' ', 'b', 'a', 'r', ' ', '1'};
  std::array<uint8_t, 11> expectedB = {0x0a, 0x09, 'F', 'o', 'o', ' ', 'b', 'a', 'r', ' ', '2'};
  
  for(uint32_t i = 0; i < expectedA.size(); ++i)
  {
    EXPECT_EQ(expectedA[i], bufferA.get_data()[i]);
  }
  for(uint32_t i = 0; i < expectedB.size(); ++i)
  {
    EXPECT_EQ(expectedB[i], bufferB.get_data()[i]);
  }
}

TEST(RepeatedStringBytes, PartialSerialize_RepeatedString_SplitWithinElement)
{
  // Test 15.3.21: Verify split can occur within a repeated string element's data
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  
  ::EmbeddedProto::FieldString<15> str;
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(0) = "Foo bar 1";
  msg.add_array_of_txt(str);
  msg.mutable_array_of_txt(1) = "Foo bar 2";

  // Buffer A: tag + size + 4 chars of first string
  ::EmbeddedProto::WriteBufferFixedSize<6> bufferA;
  repeated_string_bytes<3, 15, 3, 15, 3, 3>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(bufferA, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(6U, bufferA.get_size());
  
  // Buffer B: remaining 5 chars + partial second
  ::EmbeddedProto::WriteBufferFixedSize<10> bufferB;
  result = msg.serialize_partial(bufferB, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  
  // Buffer C: rest
  ::EmbeddedProto::WriteBufferFixedSize<20> bufferC;
  result = msg.serialize_partial(bufferC, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
}

TEST(RepeatedStringBytes, PartialSerialize_RepeatedBytes_ThreeArrays_LargeBuffer)
{
  // Test 15.3.22: Verify repeated bytes field serialization
  repeated_string_bytes<3, 15, 3, 15, 3, 3> msg;
  
  ::EmbeddedProto::FieldBytes<15> bytes;
  uint8_t data1[2] = {0x01, 0x02};
  bytes.set(data1, 2);
  msg.add_array_of_bytes(bytes);
  
  uint8_t data2[3] = {0x03, 0x04, 0x05};
  bytes.set(data2, 3);
  msg.add_array_of_bytes(bytes);
  
  uint8_t data3[1] = {0x06};
  bytes.set(data3, 1);
  msg.add_array_of_bytes(bytes);

  ::EmbeddedProto::WriteBufferFixedSize<20> buffer;
  repeated_string_bytes<3, 15, 3, 15, 3, 3>::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(buffer, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(12U, buffer.get_size());  // 12 bytes total
  
  // Verify wire format
  std::array<uint8_t, 12> expected = {
    0x12, 0x02, 0x01, 0x02,      // First array
    0x12, 0x03, 0x03, 0x04, 0x05,  // Second array
    0x12, 0x01, 0x06              // Third array
  };
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

#endif // EP_SERIALIZATION_MODE_PARTIAL
