/*
 *  Copyright (C) 2020-2021 Embedded AMS B.V. - All Rights Reserved
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

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

// EAMS message definitions
#include <optional_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace test_EmbeddedAMS_OptionalFields
{

TEST(OptionalFields, zero) 
{
  ::optional_fields<5,10> msg;

  // Test an int set to zero.
  EXPECT_FALSE(msg.has_b());
  msg.set_b(0);
  EXPECT_TRUE(msg.has_b());
  msg.clear_b();
  EXPECT_FALSE(msg.has_b());

  // Test a floating point to zero.
  EXPECT_FALSE(msg.has_y());
  msg.set_y(0.0F);
  EXPECT_TRUE(msg.has_y());
  msg.clear_y();
  EXPECT_FALSE(msg.has_y());

  // Address a nested message but do not set anything in it.
  EXPECT_FALSE(msg.has_pos());
  msg.mutable_pos().clear();
  EXPECT_TRUE(msg.has_pos());
  msg.clear_pos();
  EXPECT_FALSE(msg.has_pos());

  // Test an enum set to zero.
  EXPECT_FALSE(msg.has_state());
  msg.set_state(states::A);
  EXPECT_TRUE(msg.has_state());
  msg.clear_state();
  EXPECT_FALSE(msg.has_state());

  // Address a bytes array but do not set anything in it.
  EXPECT_FALSE(msg.has_bytes_array());
  msg.mutable_bytes_array().clear();
  EXPECT_TRUE(msg.has_bytes_array());
  msg.clear_bytes_array();
  EXPECT_FALSE(msg.has_bytes_array());

  // Address a string but do not set anything in it.
  EXPECT_FALSE(msg.has_str());
  msg.mutable_str().clear();
  EXPECT_TRUE(msg.has_str());
  msg.clear_str();
  EXPECT_FALSE(msg.has_str());

}



} // End of namespace test_EmbeddedAMS_OptionalFields
