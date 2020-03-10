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
 *  For commercial and closed source application please vist:
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
#include <include_other_files.h>
#include <repeated_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace test_EmbeddedAMS_IncludeOtherFiles
{


static constexpr uint32_t RF_SIZE = 3;

TEST(IncludeOtherFiles, zero) 
{
  InSequence s;

  // See if an empty message results in no data been pushed.
  ::IncludedMessages<RF_SIZE> msg;
  Mocks::WriteBufferMock buffer;

  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).WillRepeatedly(Return(99));

  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}

TEST(IncludeOtherFiles, set) 
{
  InSequence s;

  // See if an empty message results in no data been pushed.
  ::IncludedMessages<RF_SIZE> msg;
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


  ON_CALL(buffer, get_available_size()).WillByDefault(Return(99));

  uint8_t expected[] = { 0x08, 0x01, // state
                         // cmsg
                         0x12, 0x07, 
                         0x08, 0x01, // msg.a
                         0x15, 0x00, 0x00, 0x80, 0x3f, // msg.b
                         // rf
                         0x1a, 0x09, 
                         0x08, 0x01, // rf.x
                         0x12, 0x03, 0x01, 0x01, 0x01, // rf.y
                         0x18, 0x01}; // rf.z

  for(auto e : expected) 
  {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }   

  EXPECT_TRUE(msg.serialize(buffer));

}


} // End of namespace
