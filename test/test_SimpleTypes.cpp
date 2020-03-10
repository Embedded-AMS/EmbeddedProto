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
 *  Postal adress:
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

// EAMS message definitions
#include <simple_types.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

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

  EXPECT_TRUE(msg.serialize(buffer));

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

  uint8_t expected[] = {0x08, 0x01, 
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

  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(58, msg.serialized_size());
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


  uint8_t expected[] = {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
                        0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                        0x7F, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
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
                        0x75, 0xFF, 0xFF, 0x7F, 0x7F};
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
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

  uint8_t expected[] = {0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
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
  
  EXPECT_TRUE(msg.serialize(buffer));
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


  uint8_t expected[] = {0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 
                        0x75, 0x00, 0x00, 0x80, 0x00 };
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  EXPECT_TRUE(msg.serialize(buffer));
  EXPECT_EQ(14, msg.serialized_size());
}

TEST(SimpleTypes, deserialize_zero) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  ::Test_Simple_Types msg;

  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));
  EXPECT_TRUE(msg.deserialize(buffer));

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

  uint8_t referee[] = { 0x08, 0x01, 
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

  EXPECT_TRUE(msg.deserialize(buffer));

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

TEST(SimpleTypes, deserialize_max) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  ON_CALL(buffer, get_size()).WillByDefault(Return(58));

  ::Test_Simple_Types msg;

  uint8_t referee[] = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 
                        0x07, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                        0x7F, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
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
                        0x75, 0xFF, 0xFF, 0x7F, 0x7F};

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

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

  uint8_t referee[] = { 0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                        0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                        0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                        0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                        0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
                        0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 
                        0x6D, 0x00, 0x00, 0x00, 0x80, 
                        0x75, 0xFF, 0xFF, 0x7F, 0xFF};

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

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

  uint8_t referee[] = {0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 
                       0x75, 0x00, 0x00, 0x80, 0x00 };
  
  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));
  
  EXPECT_TRUE(msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<double>::min(), msg.get_a_double());
  EXPECT_EQ(std::numeric_limits<float>::min(),  msg.get_a_float());
}

} // End of namespace test_EmbeddedAMS_SimpleTypes
