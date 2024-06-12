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
#include <include_other_files.h>
#include <repeated_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;


namespace test_EmbeddedAMS_IncludeOtherFiles
{

static constexpr uint32_t RF_SIZE = 3;
static constexpr uint32_t ARRAY_SIZE = 2;

TEST(IncludeOtherFiles, zero) 
{
  InSequence s;

  // See if an empty message results in no data been pushed.
  ::IncludedMessages<RF_SIZE, ARRAY_SIZE> msg;
  Mocks::WriteBufferMock buffer;

  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).WillRepeatedly(Return(99));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}

TEST(IncludeOtherFiles, set_time) 
{
  TimeMessage msg;
  msg.mutable_time().set_seconds(12345);
  msg.mutable_time().set_nanos(9876);
  
  EXPECT_EQ(12345, msg.get_time().get_seconds());
  EXPECT_EQ(9876, msg.get_time().get_nanos());
}

TEST(IncludeOtherFiles, set) 
{
  InSequence s;

  ::IncludedMessages<RF_SIZE, ARRAY_SIZE> msg;
  Mocks::WriteBufferMock buffer;

  msg.set_state(some::external::lib::CommonStates::StateA);

  some::external::lib::CommonMessage cmsg;
  cmsg.set_a(1);
  cmsg.set_b(1.0F);
  msg.set_msg(cmsg);

  msg.mutable_rf().set_x(1);
  msg.mutable_rf().add_y(1);
  msg.mutable_rf().add_y(1);
  msg.mutable_rf().add_y(1);
  msg.mutable_rf().set_z(1);

  msg.mutable_sub_msg().set_val(1);
  msg.mutable_sub_msg().add_array(1);
  msg.mutable_sub_msg().add_array(1);
  msg.mutable_sub_msg().set_ne(::sub::package::other_folder_msg<ARRAY_SIZE>::Nested_Enum::NE_B);


  ON_CALL(buffer, get_available_size()).WillByDefault(Return(99));

  std::array<uint8_t, 32> expected = { 0x08, 0x01, // state
                                       // cmsg
                                       0x12, 0x07, 
                                       0x08, 0x01, // msg.a
                                       0x15, 0x00, 0x00, 0x80, 0x3f, // msg.b
                                       // rf
                                       0x1a, 0x09, 
                                       0x08, 0x01, // rf.x
                                       0x12, 0x03, 0x01, 0x01, 0x01, // rf.y
                                       0x18, 0x01, // rf.z
                                       // sub_msg
                                       0x22, 0x08, 
                                       0x08, 0x01, 
                                       0x12, 0x02, 0x01, 0x01,
                                       0x18, 0x01 };

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }   

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

}

TEST(IncludeOtherFiles, get) 
{
  InSequence s;

  ::IncludedMessages<RF_SIZE, ARRAY_SIZE> msg;
  Mocks::ReadBufferMock buffer;
  ON_CALL(buffer, get_size()).WillByDefault(Return(32));

  std::array<uint8_t, 32> referee = { 0x08, 0x01, // state
                                      // cmsg
                                      0x12, 0x07, 
                                      0x08, 0x01, // msg.a
                                      0x15, 0x00, 0x00, 0x80, 0x3f, // msg.b
                                      // rf
                                      0x1a, 0x09, 
                                      0x08, 0x01, // rf.x
                                      0x12, 0x03, 0x01, 0x01, 0x01, // rf.y
                                      0x18, 0x01, // rf.z
                                      // sub_msg
                                      0x22, 0x08, 
                                      0x08, 0x01, 
                                      0x12, 0x02, 0x01, 0x01,
                                      0x18, 0x01 };

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));    
  
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(1, msg.msg().a());
  EXPECT_EQ(1.0F, msg.msg().b());

  EXPECT_EQ(1, msg.rf().x());
  EXPECT_EQ(1, msg.rf().y(0));
  EXPECT_EQ(1, msg.rf().y(1));
  EXPECT_EQ(1, msg.rf().y(2));
  EXPECT_EQ(1, msg.rf().z());
}


} // End of namespace
