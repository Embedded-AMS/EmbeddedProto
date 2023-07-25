/*
 *  Copyright (C) 2021 Embedded AMS B.V. - All Rights Reserved
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

// In this file we test what happens when we send a newer vesion of a message and deserialize 
// that data in an older object.

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
using ::testing::DoAll;

namespace test_EmbeddedAMS_UnknownFields
{

TEST(UnknownFields, varint) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  constexpr uint32_t N_BYTES = 19;

  ON_CALL(buffer, get_size()).WillByDefault(Return(N_BYTES));

  ::Test_Simple_Types msg;

  std::array<uint8_t, N_BYTES> referee = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                                           0x90, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // Additional uint32
                                           0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F,  // a_uint32
                                         };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

TEST(UnknownFields, fixed32) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  constexpr uint32_t N_BYTES = 18;

  ON_CALL(buffer, get_size()).WillByDefault(Return(N_BYTES));

  ::Test_Simple_Types msg;

  std::array<uint8_t, N_BYTES> referee = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                                           0x9D, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, // Additional fixed32
                                           0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // a_uint32
                                         };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

TEST(UnknownFields, fixed64) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  constexpr uint32_t N_BYTES = 22;

  ON_CALL(buffer, get_size()).WillByDefault(Return(N_BYTES));

  ::Test_Simple_Types msg;

  std::array<uint8_t, N_BYTES> referee = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                                           0xA1, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Additional fixed64
                                           0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // a_uint32
                                         };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

TEST(UnknownFields, length_delimited) 
{
  InSequence s;
  Mocks::ReadBufferMock buffer;
  
  constexpr uint32_t N_BYTES = 20;

  ON_CALL(buffer, get_size()).WillByDefault(Return(N_BYTES));

  ::Test_Simple_Types msg;

  std::array<uint8_t, 6> referee_1 = { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07}; // a_int32
  for(auto r: referee_1) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }

  // Id and size of additional bytes.
  std::array<uint8_t, 3> referee_2 = { 0xaa, 0x03, 0x05}; 
  for(auto r: referee_2) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }

  // 0x01, 0x02, 0x03, 0x04, 0x05, 
  EXPECT_CALL(buffer, advance(5));

  std::array<uint8_t, 6> referee_3 = { 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F}; // a_uint32
  for(auto r: referee_3) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }

  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

} // End of namespace test_EmbeddedAMS_UnknownFields
