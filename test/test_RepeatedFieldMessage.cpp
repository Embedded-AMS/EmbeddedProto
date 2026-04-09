/*
 *  Copyright (C) 2020-2025 Embedded AMS B.V. - All Rights Reserved
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
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
#include <WriteBufferFixedSize.h>

#include <ReadBufferFixedSize.h>

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

  for(uint32_t i = 0; i < 3; ++i) 
  {
    EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));
  }

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

  ON_CALL(buffer, get_available_size()).WillByDefault(Return(10));
  
  // Empty messages (size=0) 
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  // The non-empty message (second one)
  EXPECT_CALL(buffer, push(0x12)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x04)).Times(1).WillOnce(Return(true));

  // The nested message content
  EXPECT_CALL(buffer, push(0x08)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x10)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
  
  // Empty messages (size=0) 
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
  InSequence s;
  repeated_fields<Y_SIZE> msg;

  Mocks::ReadBufferMock buffer;
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(0x12), Return(true))); // Tag of y
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(0x00), Return(true)));// Size of y = 0
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

}

#ifdef PARTIAL_DESERIALIZATION_ENABLED

TEST(RepeatedFieldMessage, deserialize_empty_message_array) 
{
  InSequence s;

  repeated_message<Y_SIZE> msg;

  Mocks::ReadBufferMock buffer;
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(0x12), Return(true)));// Tag of b
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(0x00), Return(true)));// Size of b = 0
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
}

#endif 

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
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(3, msg.get_y().get_length());
  EXPECT_EQ(1, msg.y(0));
  EXPECT_EQ(1, msg.y(1));
  EXPECT_EQ(1, msg.y(2));
  EXPECT_EQ(1, msg.get_z());

}

#ifdef PARTIAL_DESERIALIZATION_ENABLED

TEST(RepeatedFieldMessage, deserialize_one_partial) 
{
  repeated_fields<Y_SIZE> msg;

  static constexpr uint32_t SIZE = 9;

  EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({  
                                    0x08, 0x01, // x tag and value
                                    0x12}); // y tag 

  buffer.push(0x03); //y size                                    
                                    
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

          
  buffer.push(0x01);  // start of y data.                                
  buffer.push(0x01);
  buffer.push(0x01);                                
  
  buffer.push(0x18); // z tag
  buffer.push(0x01); // z value

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(3, msg.get_y().get_length());
  EXPECT_EQ(1, msg.y(0));
  EXPECT_EQ(1, msg.y(1));
  EXPECT_EQ(1, msg.y(2));
  EXPECT_EQ(1, msg.get_z());

}

TEST(RepeatedFieldMessage, deserialize_incomplete_set_of_bytes) 
{
  repeated_fields<Y_SIZE> msg;

  static constexpr uint32_t SIZE = 6;

  EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({  
                                    0x08, 0x01, // x tag and value
                                    0x12, 0x03}); // y tag and size.
                                    
                                    
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
                                 
  buffer.push(0x01);  // start of y data.                                
  buffer.push(0x01);        
  // We are missing one byte here.                     

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
 
}

TEST(RepeatedFieldMessage, deserialize_split_after_tag) 
{
  repeated_fields<Y_SIZE> msg;

  static constexpr uint32_t SIZE = 9;

  EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({  
                                    0x08, 0x01, // x tag and value
                                    0x12}); // just y tag.
                                    
                                    
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0x03);  // y size                                 
  buffer.push(0x01);  // start of y data.                                
  buffer.push(0x01);
  buffer.push(0x01);                                
  
  buffer.push(0x18); // z tag
  buffer.push(0x01); // z value

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(3, msg.get_y().get_length());
  EXPECT_EQ(1, msg.y(0));
  EXPECT_EQ(1, msg.y(1));
  EXPECT_EQ(1, msg.y(2));
  EXPECT_EQ(1, msg.get_z());

}

TEST(RepeatedFieldMessage, deserialize_split_varint) 
{
  repeated_fields<128> msg;

  static constexpr uint32_t SIZE = 133;
  
  EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({  
                                    0x08, 0x01, // x tag and value
                                    0x12, 0x80}); // y size is 128 bytes (0x80 0x01)

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));                                    

  buffer.push(0x01); // rest of size

  for(int i = 0; i < 128; ++i) {
    buffer.push(0x0D); // Push 128 bytes of value 0x0D
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(128, msg.get_y().get_length());
  EXPECT_EQ(13, msg.y(0));
  EXPECT_EQ(13, msg.y(127));
}

TEST(RepeatedFieldMessage, deserialize_split_between_elements) 
{
  repeated_fields<Y_SIZE> msg;

  static constexpr uint32_t SIZE = 16;
  
  EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({  
                                    0x08, 0x01, // x tag and value
                                    0x12, 0x0A, 0xFF, 0xFF, 0xFF, 0xFF, 0x07}); // left over y data 
                                    
                                    
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  buffer.push(0xFF); // Rest of y data
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0xFF);
  buffer.push(0x07);
  buffer.push(0x18); // z tag           

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));  

  buffer.push(0x01); // z value                  
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(2, msg.get_y().get_length());
  EXPECT_EQ(2147483647, msg.y(0));
  EXPECT_EQ(2147483647, msg.y(1));
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
                                        0x12, 0x00, 0x12, 0x04, 0x08, 0x01, 0x10, 0x01, 0x12, 0x00, // b
                                        0x18, 0x01}; // z 

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

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

  std::array<uint8_t, SIZE> referee = { 0x12, 0x00, // b[0]
                                        0x08, 0x01, // x
                                        0x12, 0x04, 0x08, 0x01, 0x10, 0x01, // b[1]
                                        0x18, 0x01, // z
                                        0x12, 0x00, }; // b[2] 

  for(auto r: referee) 
  {
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

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

#endif // PARTIAL_DESERIALIZATION_ENABLED

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
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

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
    EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(DoAll(SetArgReferee<1>(r), Return(true)));
  }
  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, enum_msg.deserialize(buffer));
  
  EXPECT_EQ(3, enum_msg.get_enum_values().get_length());
  EXPECT_EQ(SomeEnum::SE_A, enum_msg.get_enum_values()[0]);
  EXPECT_EQ(SomeEnum::SE_B, enum_msg.get_enum_values()[1]);
  EXPECT_EQ(SomeEnum::SE_C, enum_msg.get_enum_values()[2]);
}

#if (EP_SERIALIZATION_MODE_PARTIAL == EP_SERIALIZATION_MODE)

TEST(RepeatedFieldMessage, PartialDeserialize_RepeatedPacked_SizeSplit)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 130> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<132> buffer({0x82});

  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::SIZE, state.phase);
  EXPECT_EQ(0U, field.get_length());

  buffer.push(0x01);
  for(uint32_t i = 0U; i < 130U; ++i)
  {
    buffer.push(0x01);
  }

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(130U, field.get_length());
  EXPECT_EQ(1U, field[0].get());
  EXPECT_EQ(1U, field[129].get());
}

TEST(RepeatedFieldMessage, PartialDeserialize_RepeatedPacked_DataSplit)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 3> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<8> buffer({0x03, 0x01});

  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(2U, state.bytes_remaining);
  EXPECT_EQ(1U, field.get_length());
  EXPECT_EQ(1U, field[0].get());

  buffer.push(0x02);
  buffer.push(0x03);

  result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(3U, field.get_length());
  EXPECT_EQ(2U, field[1].get());
  EXPECT_EQ(3U, field[2].get());
}

TEST(RepeatedFieldMessage, PartialDeserialize_RepeatedPacked_ArrayFull)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 2> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<8> buffer({0x03, 0x01, 0x02, 0x03});

  const ::EmbeddedProto::Error result = field.deserialize_partial_as_field(buffer, state);
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, result);
  EXPECT_EQ(2U, field.get_length());
  EXPECT_EQ(1U, field[0].get());
  EXPECT_EQ(2U, field[1].get());
}

TEST(RepeatedFieldMessage, PartialDeserialize_RepeatedUnpackedBytes_DataSplit)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::FieldBytes<8>, 2> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<8> first_element({0x03, 0xAA});

  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(first_element, state);
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(1U, field.get_length());
  EXPECT_EQ(1U, field[0].get_length());
  EXPECT_EQ(0xAA, field[0].get_const()[0]);

  first_element.push(0xBB);
  first_element.push(0xCC);

  result = field.deserialize_partial_as_field(first_element, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(1U, state.element_index);
  EXPECT_EQ(3U, field[0].get_length());
  EXPECT_EQ(0xCC, field[0].get_const()[2]);

  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
  ::EmbeddedProto::ReadBufferFixedSize<8> second_element({0x02, 0x11, 0x22});
  result = field.deserialize_partial_as_field(second_element, state);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(2U, state.element_index);
  EXPECT_EQ(2U, field.get_length());
  EXPECT_EQ(0x11, field[1].get_const()[0]);
  EXPECT_EQ(0x22, field[1].get_const()[1]);
}

TEST(RepeatedFieldMessage, PartialDeserialize_RepeatedUnpackedBytes_ArrayFull)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::FieldBytes<8>, 1> field;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;

  ::EmbeddedProto::ReadBufferFixedSize<8> first_element({0x01, 0x7F});
  ::EmbeddedProto::Error result = field.deserialize_partial_as_field(first_element, state);
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(1U, field.get_length());

  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
  ::EmbeddedProto::ReadBufferFixedSize<8> second_element({0x01, 0x55});
  result = field.deserialize_partial_as_field(second_element, state);

  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, result);
  EXPECT_EQ(2U, second_element.get_size());
  EXPECT_EQ(1U, field.get_length());
}

TEST(RepeatedFieldMessage, PartialSerialize_RepeatedMessage_FreshChildState_MakesProgress)
{
  repeated_message<Y_SIZE> msg;
  repeated_nested_message e0;
  e0.set_u(1);
  e0.set_v(1);
  repeated_nested_message e1;
  e1.set_u(2);
  e1.set_v(2);
  msg.add_b(e0);
  msg.add_b(e1);

  repeated_message<Y_SIZE>::StateStack state;
  ASSERT_NE(nullptr, state.root().child);

  // Emulate we are in the DATA phase of element 1 (second element), with a clean child state.
  state.root().field_id = static_cast<uint32_t>(repeated_message<Y_SIZE>::FieldNumber::B);
  state.root().phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
  state.root().element_index = 1;
  state.root().bytes_remaining = msg.b(1).serialized_size();

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;

  const uint32_t size_before = buffer.get_size();
  const uint32_t remaining_before = state.root().bytes_remaining;
  const uint32_t index_before = state.root().element_index;

  const ::EmbeddedProto::Error result = msg.mutable_b().serialize_partial_as_field(
      static_cast<uint32_t>(repeated_message<Y_SIZE>::FieldNumber::B),
      buffer,
      state.root(),
      false);

  const bool made_progress =
      (buffer.get_size() != size_before) ||
      (state.root().bytes_remaining != remaining_before) ||
      (state.root().element_index != index_before) ||
      (state.root().phase != ::EmbeddedProto::FieldProcessingPhase::DATA) ||
      (result != ::EmbeddedProto::Error::NO_ERRORS);

  EXPECT_TRUE(made_progress)
      << "Expected progress with clean child state, but serializer made no progress.";
}

TEST(RepeatedFieldMessage, PartialSerialize_RepeatedPacked_SplitInData)
{
  repeated_fields<Y_SIZE> msg;
  msg.add_y(1);
  msg.add_y(2);
  msg.add_y(3);

  repeated_fields<Y_SIZE>::StateStack state;

  ::EmbeddedProto::WriteBufferFixedSize<3> buffer_a;
  ::EmbeddedProto::Error result = msg.serialize_partial(buffer_a, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(3U, buffer_a.get_size());
  EXPECT_EQ(0x12, buffer_a.get_data()[0]);
  EXPECT_EQ(0x03, buffer_a.get_data()[1]);
  EXPECT_EQ(0x01, buffer_a.get_data()[2]);

  ::EmbeddedProto::WriteBufferFixedSize<3> buffer_b;
  result = msg.serialize_partial(buffer_b, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer_b.get_size());
  EXPECT_EQ(0x02, buffer_b.get_data()[0]);
  EXPECT_EQ(0x03, buffer_b.get_data()[1]);
}

TEST(RepeatedFieldMessage, PartialSerialize_RepeatedMessage_SplitBetweenElements)
{
  repeated_message<Y_SIZE> msg;

  repeated_nested_message rnm;
  rnm.set_u(0);
  rnm.set_v(0);
  msg.add_b(rnm);
  msg.add_b(rnm);
  msg.add_b(rnm);

  repeated_message<Y_SIZE>::StateStack state;

  ::EmbeddedProto::WriteBufferFixedSize<4> buffer_a;
  ::EmbeddedProto::Error result = msg.serialize_partial(buffer_a, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);
  EXPECT_EQ(4U, buffer_a.get_size());
  EXPECT_EQ(0x12, buffer_a.get_data()[0]);
  EXPECT_EQ(0x00, buffer_a.get_data()[1]);
  EXPECT_EQ(0x12, buffer_a.get_data()[2]);
  EXPECT_EQ(0x00, buffer_a.get_data()[3]);

  ::EmbeddedProto::WriteBufferFixedSize<4> buffer_b;
  result = msg.serialize_partial(buffer_b, state.root());

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);
  EXPECT_EQ(2U, buffer_b.get_size());
  EXPECT_EQ(0x12, buffer_b.get_data()[0]);
  EXPECT_EQ(0x00, buffer_b.get_data()[1]);
}

#endif // EP_SERIALIZATION_MODE_PARTIAL

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
