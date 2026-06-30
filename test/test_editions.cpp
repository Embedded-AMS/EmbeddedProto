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
#include "gmock/gmock.h"

#include "edition_2023.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>
#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>

namespace test_EmbeddedAMS_editions
{

TEST(Editions, construction)
{
  Edition2023Message message_2023;

  message_2023.set_value(123U);

  EXPECT_EQ(123U, message_2023.get_value());
}

// ED-2 field_presence -------------------------------------------------------

// An EXPLICIT scalar (edition 2023 default) tracks presence and gets has_*().
TEST(EditionsPresence, explicit_has_lifecycle)
{
  PresenceMessage msg;

  EXPECT_FALSE(msg.has_explicit_field());
  msg.set_explicit_field(0U);              // set to the default value
  EXPECT_TRUE(msg.has_explicit_field());   // still present
  msg.clear_explicit_field();
  EXPECT_FALSE(msg.has_explicit_field());
}

// A freshly constructed message only serializes the LEGACY_REQUIRED field; the
// EXPLICIT field is absent and the IMPLICIT field is at its default value.
TEST(EditionsPresence, required_always_serialized)
{
  PresenceMessage msg;

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // Only required_field (field 3, varint) at value 0 -> 0x18 0x00.
  const std::array<uint8_t, 2> expected = { 0x18, 0x00 };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

// An EXPLICIT field set to the default value still serializes (it is present),
// unlike a proto3 implicit field.
TEST(EditionsPresence, explicit_at_default_is_serialized)
{
  PresenceMessage msg;
  msg.set_explicit_field(0U);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // explicit_field (field 1) 0x08 0x00, then required_field (field 3) 0x18 0x00.
  const std::array<uint8_t, 4> expected = { 0x08, 0x00, 0x18, 0x00 };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

// An IMPLICIT field at its default value is not serialized; a non-default value
// is.
TEST(EditionsPresence, implicit_only_serialized_when_non_default)
{
  PresenceMessage msg;
  msg.set_implicit_field(0U);  // default -> not on the wire

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  const std::array<uint8_t, 2> expected_default = { 0x18, 0x00 };  // only required
  ASSERT_EQ(expected_default.size(), buffer.get_size());

  msg.set_implicit_field(5U);
  ::EmbeddedProto::WriteBufferFixedSize<32> buffer2;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer2));
  // implicit_field (field 2) 0x10 0x05, then required_field 0x18 0x00.
  const std::array<uint8_t, 4> expected = { 0x10, 0x05, 0x18, 0x00 };
  ASSERT_EQ(expected.size(), buffer2.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer2.get_data()[i]);
  }
}

// Round trip: all fields set to non-default values survive serialize/deserialize.
TEST(EditionsPresence, round_trip)
{
  PresenceMessage msg;
  msg.set_explicit_field(7U);
  msg.set_implicit_field(5U);
  msg.set_required_field(9U);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  PresenceMessage result;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  EXPECT_TRUE(result.has_explicit_field());
  EXPECT_EQ(7U, result.get_explicit_field());
  EXPECT_EQ(5U, result.get_implicit_field());
  EXPECT_EQ(9U, result.get_required_field());
}

} // End of namespace test_EmbeddedAMS_editions
