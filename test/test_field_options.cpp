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
TEST(FieldOptions, repeated_get_max_length) 
{
  ConfigUpdate<3> msg;
  // The ten used here is defined in field_options.proto as a custom option of embedded proto.
  EXPECT_EQ(10, msg.a().get_max_length());

  NestedConfigUpdate<3> msgNested;
  EXPECT_EQ(10, msgNested.update().a().get_max_length());
}

TEST(FieldOptions, repeated_clear) 
{
  // The clear function is influenced by the get_short_type function which changed for the options.
  ConfigUpdate<3> msg;
  // Add some data
  msg.mutable_a().add(1);
  msg.mutable_a().add(2);
  EXPECT_EQ(2, msg.a().get_length());

  msg.mutable_a().clear();
  EXPECT_EQ(0U, msg.a().get_length());
}

} // End of namespace test_EmbeddedAMS_FieldOptions
