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

// EAMS message definitions
#include <field_options.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace test_EmbeddedAMS_FieldOptions
{
TEST(FieldOptions, get_max_length) 
{
  Options::ConfigUpdate<3> msg;
  // The ten used here is defined in field_options.proto as a custom option of embedded proto.
  EXPECT_EQ(10, msg.a().get_max_length());
  EXPECT_EQ(3, msg.enum_values().get_max_length());

  Options::NestedConfigUpdate<3> msgNested;
  EXPECT_EQ(10, msgNested.update().a().get_max_length());


  Options::BytesMaxLength bMsg;
  EXPECT_EQ(100, bMsg.get_b().get_max_length());


  Options::StringMaxLength sMsg;
  EXPECT_EQ(256, sMsg.get_s().get_max_length());

}

TEST(FieldOptions, oneof_clear) 
{
  // When in a oneof the clear function is influenced by the get_short_type function which changed 
  // for the options.
  Options::OneofWithMaxLength msg;
  uint8_t data[] = {1, 2, 3, 4, 5};
  msg.mutable_b().set(data, 5);
  EXPECT_EQ(100, msg.get_b().get_max_length());
  EXPECT_EQ(5, msg.get_b().get_length());

  msg.clear();
  EXPECT_EQ(0, msg.mutable_b().get_length());

  msg.mutable_s() = "hello";
  EXPECT_EQ(256, msg.get_s().get_max_length());
  EXPECT_EQ(5, msg.get_s().get_length());

  msg.clear();
  EXPECT_EQ(0, msg.mutable_s().get_length());

}

} // End of namespace test_EmbeddedAMS_FieldOptions
