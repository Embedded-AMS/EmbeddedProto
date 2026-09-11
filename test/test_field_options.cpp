/*
 *  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
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

#include <EmbeddedProto/WireFormatter.h>
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

TEST(FieldOptions, nested_config_update_repeated)
{
  Options::NestedConfigUpdateRepeated<3, 10> msg;
  // No maxLength constraint on the repeated field, so we just test basic functionality
  EXPECT_EQ(0, msg.update().get_length());

  // Add some items to verify the repeated field works
  Options::ConfigUpdate<10> update1;
  update1.mutable_a().add(1);
  update1.mutable_a().add(2);
  msg.add_update(update1);

  Options::ConfigUpdate<10> update2;
  update2.mutable_a().add(3);
  msg.add_update(update2);

  EXPECT_EQ(2, msg.update().get_length());
  EXPECT_EQ(2, msg.update()[0].a().get_length());
  EXPECT_EQ(1, msg.update()[1].a().get_length());
}

TEST(FieldOptions, nested_config_update_repeated_max_length)
{
  Options::NestedConfigUpdateRepeatedMaxLength<3> msg;
  // This should have a maxLength of 3 on the repeated field
  EXPECT_EQ(0, msg.update().get_length());
  EXPECT_EQ(3, msg.update().get_max_length());

  // Add items up to the max length
  Options::ConfigUpdate<3> update1;
  msg.add_update(update1);

  Options::ConfigUpdate<3> update2;
  msg.add_update(update2);

  Options::ConfigUpdate<3> update3;
  msg.add_update(update3);

  EXPECT_EQ(3, msg.update().get_length());

  // Adding a fourth item should fail or be constrained
  // Note: The actual behavior depends on how maxLength is enforced in the implementation
  // This test might need adjustment based on the actual behavior
}

} // End of namespace test_EmbeddedAMS_FieldOptions
