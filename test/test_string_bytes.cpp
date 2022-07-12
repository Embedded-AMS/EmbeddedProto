/*
 *  Copyright (C) 2020-2022 Embedded AMS B.V. - All Rights Reserved
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

  // Teat assigning a string by array pointer. 
  char text[] = "Foo bar 3";
  msg.mutable_txt() = text;
  EXPECT_EQ(9, msg.get_txt().get_length());
  ASSERT_STREQ("Foo bar 3", msg.get_txt().get_const());

  const char* text2 = msg.get_txt().get_const();
  ASSERT_STREQ("Foo bar 3", text2);
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
}

TEST(FieldBytes, assign_msg) 
{
  raw_bytes<10> msgA;
  raw_bytes<10> msgB;
  const std::array<uint8_t, 10> data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  msgA.mutable_b().set(data.data(), 10);
  msgB = msgA;

  for(uint8_t i = 0; i < 10; ++i) {
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


} // End of namespace test_EmbeddedAMS_string_bytes