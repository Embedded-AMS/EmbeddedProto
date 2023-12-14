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

#include <cstdint>    
#include <limits>
#include <array>

// EAMS message definitions
#include <repeated_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_RepeatedFieldMessage
{

static constexpr uint32_t Y_SIZE = 3;

TEST(RepeatedFieldMessage, construction) 
{
  repeated_fields<Y_SIZE> msg;
  repeated_message<Y_SIZE> msg2;
}

TEST(RepeatedFieldMessage, serialize_empty_fields) 
{
  repeated_fields<Y_SIZE> msg;

  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).WillRepeatedly(Return(99));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}

TEST(RepeatedFieldMessage, serialize_empty_message) 
{
  repeated_message<Y_SIZE> msg;

  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).WillRepeatedly(Return(99));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}

TEST(RepeatedFieldMessage, serialize_array_zero_fields)
{ 
  InSequence s;
  
  Mocks::WriteBufferMock buffer;
  repeated_fields<Y_SIZE> msg;

  msg.add_y(0);
  msg.add_y(0);
  msg.add_y(0);

  // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x03)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(3));

  std::array<uint8_t, 3> expected = {0x00, 0x00, 0x00}; // data of y
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, serialize_array_zero_messages)
{ 
  InSequence s;
  
  Mocks::WriteBufferMock buffer;
  repeated_message<Y_SIZE> msg;

  repeated_nested_message rnm;
  rnm.set_u(0);
  rnm.set_v(0);

  msg.add_b(rnm);
  msg.add_b(rnm);
  msg.add_b(rnm);

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(6));

  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, serialize_array_zero_one_zero)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.add_y(0);
  msg.add_y(1);
  msg.add_y(0);

  // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x03)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(3));

  std::array<uint8_t, 3> expected = {0x00, 0x01, 0x00}; // data of y

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, serialize_array_zero_one_zero_messages)
{ 
  InSequence s;
  
  Mocks::WriteBufferMock buffer;
  repeated_message<Y_SIZE> msg;

  repeated_nested_message rnm;
  
  rnm.set_u(0);
  rnm.set_v(0);
  msg.add_b(rnm);

  rnm.set_u(1);
  rnm.set_v(1);
  msg.add_b(rnm);
  
  rnm.set_u(0);
  rnm.set_v(0);
  msg.add_b(rnm);

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x04)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x08)).Times(1).WillOnce(Return(true)); 
  EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x10)).Times(1).WillOnce(Return(true)); 
  EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, serialize_array_one)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.add_y(1);
  msg.add_y(1);
  msg.add_y(1);

  // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x03)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(3));

  std::array<uint8_t, 3> expected = {0x01, 0x01, 0x01}; // data of y

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}



TEST(RepeatedFieldMessage, serialize_array_max)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());

    // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x0F)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(15));

  std::array<uint8_t, 15> expected = {0xff, 0xff, 0xff, 0xff, 0x0f, 
                                      0xff, 0xff, 0xff, 0xff, 0x0f, 
                                      0xff, 0xff, 0xff, 0xff, 0x0f}; // data of y

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}



TEST(RepeatedFieldMessage, serialize_one)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.set_x(1);
  msg.add_y(1);
  msg.add_y(1);
  msg.add_y(1);
  msg.set_z(1);

  std::array<uint8_t, 2> expected_x = {0x08, 0x01};  // x
  
  for(auto e : expected_x) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }                  
  
    // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x03)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(5));
  
  std::array<uint8_t, 5> expected = {0x01, 0x01, 0x01, // data of y
                                     0x18, 0x01}; // z
  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}


TEST(RepeatedFieldMessage, serialize_max)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.set_x(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.set_z(std::numeric_limits<uint32_t>::max());

  std::array<uint8_t, 6> expected_x = {0x08, 0xff, 0xff, 0xff, 0xff, 0x0f};  // x
  
  for(auto e : expected_x) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }                  
  
    // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x0F)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(21));

  std::array<uint8_t, 21> expected = {0xff, 0xff, 0xff, 0xff, 0x0f, 
                                      0xff, 0xff, 0xff, 0xff, 0x0f, 
                                      0xff, 0xff, 0xff, 0xff, 0x0f, // data of y
                                      0x18, 0xff, 0xff, 0xff, 0xff, 0x0f}; // z

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, serialize_fault_buffer_full)
{
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.add_y(1);
  msg.add_y(1);
  msg.add_y(1);                 
  
  // Tag and size of y
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x03)).Times(1).WillOnce(Return(true));

  // Need 3 bytes but got only 2.
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(2));

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, deserialize_empty_array) 
{
  repeated_fields<Y_SIZE> msg;

  Mocks::ReadBufferMock buffer;
  EXPECT_CALL(buffer, pop(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(buffer, get_size()).WillRepeatedly(Return(0));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

}

TEST(RepeatedFieldMessage, deserialize_empty_message_array) 
{
  repeated_message<Y_SIZE> msg;

  Mocks::ReadBufferMock buffer;
  EXPECT_CALL(buffer, pop(_)).WillRepeatedly(Return(false));
  EXPECT_CALL(buffer, get_size()).WillRepeatedly(Return(0));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
}

TEST(RepeatedFieldMessage, deserialize_one) 
{
  InSequence s;

  repeated_fields<Y_SIZE> msg;
  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 9;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  std::array<uint8_t, SIZE> referee = { 0x08, 0x01, // x
                                        0x12, 0x03, 0x01, 0x01, 0x01, // y
                                        0x18, 0x01}; // z 

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(3, msg.get_y().get_length());
  EXPECT_EQ(1, msg.y(0));
  EXPECT_EQ(1, msg.y(1));
  EXPECT_EQ(1, msg.y(2));
  EXPECT_EQ(1, msg.get_z());

}

TEST(RepeatedFieldMessage, deserialize_one_message_array) 
{
  InSequence s;

  repeated_message<Y_SIZE> msg;
  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 14;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  std::array<uint8_t, SIZE> referee = { 0x08, 0x01, // x
                                        0x12, 0x00, 0x12, 0x04, 0x08, 0x01, 0x10, 0x01, 0x12, 0x00, // y
                                        0x18, 0x01}; // z 

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(3, msg.get_b().get_length());
  EXPECT_EQ(0, msg.b(0).u());
  EXPECT_EQ(0, msg.b(0).v());
  EXPECT_EQ(1, msg.b(1).u());
  EXPECT_EQ(1, msg.b(1).v());
  EXPECT_EQ(0, msg.b(2).u());
  EXPECT_EQ(0, msg.b(2).v());
  EXPECT_EQ(1, msg.get_c());
}

TEST(RepeatedFieldMessage, deserialize_mixed_message_array) 
{
  // I should be possible to read in the non packed data mixed with other fields. All elements 
  // should be added to the array.

  InSequence s;

  repeated_message<Y_SIZE> msg;
  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 14;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  std::array<uint8_t, SIZE> referee = { 0x12, 0x00, // y[0]
                                        0x08, 0x01, // x
                                        0x12, 0x04, 0x08, 0x01, 0x10, 0x01, // y[1]
                                        0x18, 0x01, // z
                                        0x12, 0x00, }; // y[2] 

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(3, msg.get_b().get_length());
  EXPECT_EQ(0, msg.b(0).u());
  EXPECT_EQ(0, msg.b(0).v());
  EXPECT_EQ(1, msg.b(1).u());
  EXPECT_EQ(1, msg.b(1).v());
  EXPECT_EQ(0, msg.b(2).u());
  EXPECT_EQ(0, msg.b(2).v());
  EXPECT_EQ(1, msg.get_c());
}

TEST(RepeatedFieldMessage, deserialize_max) 
{
  InSequence s;

  repeated_fields<Y_SIZE> msg;
  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 29;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  std::array<uint8_t, SIZE> referee = { 0x08, 0xff, 0xff, 0xff, 0xff, 0x0f,  // x
                                        0x12, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, // y
                                        0x18, 0xff, 0xff, 0xff, 0xff, 0x0f}; // z

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_x());
  EXPECT_EQ(3, msg.get_y().get_length());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.y(0));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.y(1));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.y(2));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_z());

}

TEST(RepeatedFieldMessage, assign_a_nested_message) 
{
  // Assign a nested message which holds an repeated field.
  nested_repeated_message<Y_SIZE>  top_level_msg;

  // The nexted message
  repeated_fields<Y_SIZE> nested_msg;

  // Fill the array with some data.
  nested_msg.add_y(1);
  nested_msg.add_y(2);
  nested_msg.add_y(3);


  // And assign
  top_level_msg.set_rf(nested_msg);
  
  EXPECT_EQ(3, top_level_msg.get_rf().get_y().get_length());
  EXPECT_EQ(1, top_level_msg.get_rf().get_y()[0]);
  EXPECT_EQ(2, top_level_msg.get_rf().get_y()[1]);
  EXPECT_EQ(3, top_level_msg.get_rf().get_y()[2]);

}

TEST(RepeatedFieldMessage, assign_repeated_enum) 
{
  repeated_enum<Y_SIZE> enum_msg;
  enum_msg.add_enum_values(SomeEnum::SE_A);
  enum_msg.add_enum_values(SomeEnum::SE_B);
  enum_msg.add_enum_values(SomeEnum::SE_C);

  EXPECT_EQ(SomeEnum::SE_A, enum_msg.get_enum_values()[0]);
  EXPECT_EQ(SomeEnum::SE_B, enum_msg.get_enum_values()[1]);
  EXPECT_EQ(SomeEnum::SE_C, enum_msg.get_enum_values()[2]);
}

TEST(RepeatedFieldMessage, serialize_repeated_enum)
{
  InSequence s;

  repeated_enum<Y_SIZE> enum_msg;
  Mocks::WriteBufferMock buffer;

  enum_msg.add_enum_values(SomeEnum::SE_A);
  enum_msg.add_enum_values(SomeEnum::SE_B);
  enum_msg.add_enum_values(SomeEnum::SE_C);
  
  EXPECT_CALL(buffer, push(0x0a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x03)).Times(1).WillOnce(Return(true));

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(3));

  std::array<uint8_t, 3> expected = {0x00, 0x01, 0x02}; // enum values

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, enum_msg.serialize(buffer));
}

TEST(RepeatedFieldMessage, deserialize_repeated_enum)
{
  InSequence s;

  repeated_enum<Y_SIZE> enum_msg;
  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 5;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  std::array<uint8_t, SIZE> referee = { 0x0a, 0x03, 0x00, 0x01, 0x02}; 

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, enum_msg.deserialize(buffer));
  
  EXPECT_EQ(3, enum_msg.get_enum_values().get_length());
  EXPECT_EQ(SomeEnum::SE_A, enum_msg.get_enum_values()[0]);
  EXPECT_EQ(SomeEnum::SE_B, enum_msg.get_enum_values()[1]);
  EXPECT_EQ(SomeEnum::SE_C, enum_msg.get_enum_values()[2]);
}

#ifdef MSG_TO_STRING

TEST(RepeatedFieldMessage, to_string)
{
  repeated_message<Y_SIZE> msg;
  repeated_nested_message rnm;

  constexpr uint32_t N = 1024;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };

  rnm.set_u(0);
  rnm.set_v(1);
  msg.add_b(rnm);

  rnm.set_u(2);
  rnm.set_v(3);
  msg.add_b(rnm);

  rnm.set_u(4);
  rnm.set_v(5);
  msg.add_b(rnm);

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);
  
  // std::cout << std::endl << str << std::endl;

  constexpr uint32_t TXT_LEN = 220;
  const char expected_str[TXT_LEN + 1] = "{\n  \"a\": 0,\n  \"b\": [\n         {\n           \"u\": 0,\n           \"v\": 1\n         },\n         {\n           \"u\": 2,\n           \"v\": 3\n         },\n         {\n           \"u\": 4,\n           \"v\": 5\n         }\n       ],\n  \"c\": 0\n}"; 
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);
  EXPECT_EQ(str + TXT_LEN, str_left.data);
}

#endif // MSG_TO_STRING

} // End of namespace test_EmbeddedAMS_RepeatedFieldMessage
