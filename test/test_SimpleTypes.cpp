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
#include <string.h>

// EAMS message definitions
#include <simple_types.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_SimpleTypes 
{

TEST(SimpleTypes, zero) 
{
  InSequence s;

  // See if an empty message results in no data been pushed.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}


TEST(SimpleTypes, serialize_one) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B);

  std::array<uint8_t, 60> expected = {0x08, 0x01, 
                                      0x10, 0x01, 
                                      0x18, 0x01, 
                                      0x20, 0x01, 
                                      0x28, 0x02, 
                                      0x30, 0x02, 
                                      0x38, 0x01, 
                                      0x40, 0x01, 
                                      0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                      0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                      0x65, 0x01, 0x00, 0x00, 0x00, 
                                      0x6d, 0x01, 0x00, 0x00, 0x00, 
                                      0x75, 0x00, 0x00, 0x80, 0x3f,
                                      0x78, 0x01};

  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(60, msg.serialized_size());
}

TEST(SimpleTypes, serialize_max) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_int32(std::numeric_limits<int32_t>::max());   
  msg.set_a_int64(std::numeric_limits<int64_t>::max());     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::max());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sint32(std::numeric_limits<int32_t>::max());
  msg.set_a_sint64(std::numeric_limits<int64_t>::max());
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::TWOBILLION);
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::max());
  msg.set_a_double(std::numeric_limits<double>::max());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::max());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::max()); 
  msg.set_a_float(std::numeric_limits<float>::max());


  std::array<uint8_t, 100> expected = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
                                        0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                        0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                        0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                        0x28, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 
                                        0x30, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                        0x38, 0x01, 
                                        0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07,
                                        0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                        0x51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                        0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, 
                                        0x65, 0xFF, 0xFF, 0xFF, 0xFF, 
                                        0x6D, 0xFF, 0xFF, 0xFF, 0x7F, 
                                        0x75, 0xFF, 0xFF, 0x7F, 0x7F };
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(100, msg.serialized_size());
}

TEST(SimpleTypes, serialize_min) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_int32(std::numeric_limits<int32_t>::min());   
  msg.set_a_int64(std::numeric_limits<int64_t>::min());     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::min());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::min());
  msg.set_a_sint32(std::numeric_limits<int32_t>::min());
  msg.set_a_sint64(std::numeric_limits<int64_t>::min());
  msg.set_a_bool(false);
  msg.set_a_enum(Test_Enum::ZERO);
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::min());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::min());
  msg.set_a_double(std::numeric_limits<double>::lowest());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::min());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::min()); 
  msg.set_a_float(std::numeric_limits<float>::lowest());

  std::array<uint8_t, 62> expected = {0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                                      0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                                      0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                      0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                                      0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
                                      0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 
                                      0x6D, 0x00, 0x00, 0x00, 0x80, 
                                      0x75, 0xFF, 0xFF, 0x7F, 0xFF};
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(62, msg.serialized_size());
}

TEST(SimpleTypes, serialize_smalest_real) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  msg.set_a_double(std::numeric_limits<double>::min());
  msg.set_a_float(std::numeric_limits<float>::min());


  std::array<uint8_t, 14>  expected = { 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 
                                        0x75, 0x00, 0x00, 0x80, 0x00 };
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(14, msg.serialized_size());
}

TEST(SimpleTypes, serialize_fault_buffer_full_varint)
{
    InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::WriteBufferMock buffer;

  // Just set some large value.
  msg.set_a_uint32(std::numeric_limits<uint32_t>::max());

  // Allow for some bytes to be serialized
  EXPECT_CALL(buffer, push(_)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(_)).Times(1).WillOnce(Return(true));  
  // And then fail
  EXPECT_CALL(buffer, push(_)).Times(2).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, msg.serialize(buffer));

}

TEST(SimpleTypes, deserialize_zero) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  ::Test_Simple_Types msg;

  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(0, msg.get_a_int32());   
  EXPECT_EQ(0, msg.get_a_int64());     
  EXPECT_EQ(0U, msg.get_a_uint32());    
  EXPECT_EQ(0U, msg.get_a_uint64());
  EXPECT_EQ(0, msg.get_a_sint32());
  EXPECT_EQ(0, msg.get_a_sint64());
  EXPECT_EQ(false, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ZERO, msg.get_a_enum());
  EXPECT_EQ(0U, msg.get_a_fixed64());
  EXPECT_EQ(0, msg.get_a_sfixed64());
  EXPECT_EQ(0.0, msg.get_a_double());
  EXPECT_EQ(0U, msg.get_a_fixed32());
  EXPECT_EQ(0, msg.get_a_sfixed32()); 
  EXPECT_EQ(0.0F, msg.get_a_float());
}

TEST(SimpleTypes, deserialize_one) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(58));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 58>  referee = { 0x08, 0x01, 
                                         0x10, 0x01, 
                                         0x18, 0x01, 
                                         0x20, 0x01, 
                                         0x28, 0x02, 
                                         0x30, 0x02, 
                                         0x38, 0x01, 
                                         0x40, 0x01, 
                                         0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                         0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
                                         0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
                                         0x65, 0x01, 0x00, 0x00, 0x00, 
                                         0x6d, 0x01, 0x00, 0x00, 0x00, 
                                         0x75, 0x00, 0x00, 0x80, 0x3f};

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a_int32());   
  EXPECT_EQ(1, msg.get_a_int64());     
  EXPECT_EQ(1U, msg.get_a_uint32());    
  EXPECT_EQ(1U, msg.get_a_uint64());
  EXPECT_EQ(1, msg.get_a_sint32());
  EXPECT_EQ(1, msg.get_a_sint64());
  EXPECT_EQ(true, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ONE, msg.get_a_enum());
  EXPECT_EQ(1U, msg.get_a_fixed64());
  EXPECT_EQ(1, msg.get_a_sfixed64());
  EXPECT_EQ(1.0, msg.get_a_double());
  EXPECT_EQ(1U, msg.get_a_fixed32());
  EXPECT_EQ(1, msg.get_a_sfixed32()); 
  EXPECT_EQ(1.0F, msg.get_a_float());
}

TEST(SimpleTypes, deserialize_10_byte_int32)
{
  // Some implementations serialize 32bit integers in 10 bytes instead of the theoretical 5.
  // Data in the higher bytes should be ignored.

  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  constexpr uint32_t N_BYTES = 11;

  ON_CALL(buffer, get_size()).WillByDefault(Return(N_BYTES));

  ::Test_Simple_Types msg;

  std::array<uint8_t, N_BYTES>  referee = { 0x08, 0x81, 0x80, 0x80, 0x80, 0x80, 0x8F, 0x8F, 0x8F, 0x8F, 0x0F };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a_int32());   
}

TEST(SimpleTypes, deserialize_max) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(100));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 100> referee = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
                                       0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                       0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                       0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                       0x28, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 
                                       0x30, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                                       0x38, 0x01, 
                                       0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07,
                                       0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                                       0x51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                                       0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, 
                                       0x65, 0xFF, 0xFF, 0xFF, 0xFF, 
                                       0x6D, 0xFF, 0xFF, 0xFF, 0x7F, 
                                       0x75, 0xFF, 0xFF, 0x7F, 0x7F };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(),  msg.get_a_int32());   
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),  msg.get_a_int64());     
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());    
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), msg.get_a_uint64());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(),  msg.get_a_sint32());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),  msg.get_a_sint64());
  EXPECT_EQ(true, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::TWOBILLION, msg.get_a_enum());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), msg.get_a_fixed64());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(),  msg.get_a_sfixed64());
  EXPECT_EQ(std::numeric_limits<double>::max(),   msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_fixed32());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(),  msg.get_a_sfixed32()); 
  EXPECT_EQ(std::numeric_limits<float>::max(),    msg.get_a_float());
}

TEST(SimpleTypes, deserialize_min) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(58));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 62>  referee = { 0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                                       0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                                       0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                                       0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                                       0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
                                       0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 
                                       0x6D, 0x00, 0x00, 0x00, 0x80, 
                                       0x75, 0xFF, 0xFF, 0x7F, 0xFF };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::min(),  msg.get_a_int32());   
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),  msg.get_a_int64());     
  EXPECT_EQ(std::numeric_limits<uint32_t>::min(), msg.get_a_uint32());    
  EXPECT_EQ(std::numeric_limits<uint64_t>::min(), msg.get_a_uint64());
  EXPECT_EQ(std::numeric_limits<int32_t>::min(),  msg.get_a_sint32());
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),  msg.get_a_sint64());
  EXPECT_EQ(false, msg.get_a_bool());
  EXPECT_EQ(Test_Enum::ZERO, msg.get_a_enum());
  EXPECT_EQ(std::numeric_limits<uint64_t>::min(),  msg.get_a_fixed64());
  EXPECT_EQ(std::numeric_limits<int64_t>::min(),   msg.get_a_sfixed64());
  EXPECT_EQ(std::numeric_limits<double>::lowest(), msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<uint32_t>::min(),  msg.get_a_fixed32());
  EXPECT_EQ(std::numeric_limits<int32_t>::min(),   msg.get_a_sfixed32()); 
  EXPECT_EQ(std::numeric_limits<float>::lowest(),  msg.get_a_float());
}

TEST(SimpleTypes, deserialize_smalest_real) 
{
  InSequence s;
  
  ::Test_Simple_Types msg;
  Mocks::ReadBufferMock buffer;

  std::array<uint8_t, 14> referee = { 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 
                                      0x75, 0x00, 0x00, 0x80, 0x00 };
  
  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<double>::min(), msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<float>::min(),  msg.get_a_float());
}

TEST(SimpleTypes, deserialize_fault_end_of_buffer_fixed)
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(58));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 3> referee = {0x49, 0xFF, 0xFF}; // End half way through a fixed size value.

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
}

TEST(SimpleTypes, deserialize_fault_end_of_buffer_bool)
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(58));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 1> referee = {0x38}; // Just the tag of a bool but no data

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));
}

TEST(SimpleTypes, deserialize_enum_beond_range)
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(2));

  ::Test_Simple_Types msg;

  // This enum value is beond the range known to this code. Decodation should not fail. The value
  // should however not match to any of the known enum values.
  std::array<uint8_t, 2> referee = {0x78, 0x03}; 

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_NE(::Test_Simple_Types::Nested_Enum::NE_A, msg.get_a_nested_enum());
  EXPECT_NE(::Test_Simple_Types::Nested_Enum::NE_B, msg.get_a_nested_enum());
  EXPECT_NE(::Test_Simple_Types::Nested_Enum::NE_C, msg.get_a_nested_enum());

}

TEST(SimpleTypes, deserialize_fault_overlong_varint)
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  constexpr uint32_t N_BYTES = 11;

  ON_CALL(buffer, get_size()).WillByDefault(Return(N_BYTES));

  ::Test_Simple_Types msg;

  std::array<uint8_t, N_BYTES> referee = { 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };// Invalid closing byte for a_int64

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.deserialize(buffer));
}

TEST(SimpleTypes, field_number_to_name)
{
  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(::Test_Simple_Types::FieldNumber::A_INT32),
                          "a_int32"));

  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(::Test_Simple_Types::FieldNumber::A_NESTED_ENUM),
                          "a_nested_enum"));

  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(static_cast<::Test_Simple_Types::FieldNumber>(0)),
                          "Invalid FieldNumber"));

  EXPECT_TRUE(0 == strcmp(::Test_Simple_Types::field_number_to_name(static_cast<::Test_Simple_Types::FieldNumber>(99)),
                          "Invalid FieldNumber"));
}

#ifdef MSG_TO_STRING

TEST(SimpleTypes, to_string)
{
  constexpr uint32_t N = 1024;
  char str[N];
  ::EmbeddedProto::string_view str_view = { str, N };
 
  ::Test_Simple_Types msg;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);
  msg.set_a_nested_enum(::Test_Simple_Types::Nested_Enum::NE_B); 

  ::EmbeddedProto::string_view str_left = msg.to_string(str_view);

  // std::cout << std::endl << str << std::endl;
  
  constexpr uint32_t TXT_LEN = 278;
  const char expected_str[TXT_LEN + 1] = "{\n  \"a_int32\": 1,\n  \"a_int64\": 1,\n  \"a_uint32\": 1,\n  \"a_uint64\": 1,\n  \"a_sint32\": 1,\n  \"a_sint64\": 1,\n  \"a_bool\": true,\n  \"a_enum\": 1,\n  \"a_fixed64\": 1,\n  \"a_sfixed64\": 1,\n  \"a_double\": 1.000000,\n  \"a_fixed32\": 1,\n  \"a_sfixed32\": 1,\n  \"a_float\": 1.000000,\n  \"a_nested_enum\": 1\n}";
  ASSERT_STREQ(expected_str, str);
  EXPECT_EQ(N - TXT_LEN, str_left.size);
  EXPECT_EQ(str + TXT_LEN, str_left.data);  

}

#endif // MSG_TO_STRING

} // End of namespace test_EmbeddedAMS_SimpleTypes
