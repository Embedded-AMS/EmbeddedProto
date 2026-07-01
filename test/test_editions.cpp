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

// ED-3 custom default values ------------------------------------------------

// A freshly constructed message returns the custom default for every field.
TEST(EditionsDefaults, construct_to_default)
{
  DefaultsMessage msg;

  EXPECT_EQ(42, msg.get_a());
  EXPECT_TRUE(msg.get_b());
  EXPECT_FLOAT_EQ(1.5F, msg.get_f());
  EXPECT_EQ(DefaultColor::DC_BLUE, msg.get_c());
  EXPECT_EQ(-9000000000LL, msg.get_big());

  // The fields are explicit-presence; the default does not imply presence.
  EXPECT_FALSE(msg.has_a());
}

// clear_*() returns the field to its custom default, not to zero.
TEST(EditionsDefaults, clear_restores_default)
{
  DefaultsMessage msg;
  msg.set_a(7);
  EXPECT_TRUE(msg.has_a());
  EXPECT_EQ(7, msg.get_a());

  msg.clear_a();
  EXPECT_FALSE(msg.has_a());
  EXPECT_EQ(42, msg.get_a());
}

// Deserializing a buffer that omits a field leaves it at its custom default.
TEST(EditionsDefaults, absent_field_yields_default)
{
  // Build a buffer that carries only field b (set to a non-default value) so
  // fields a and c are absent on the wire.
  DefaultsMessage source;
  source.set_b(false);
  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, source.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  DefaultsMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(read_buffer));
  EXPECT_FALSE(msg.get_b());            // received
  EXPECT_EQ(42, msg.get_a());           // absent -> default
  EXPECT_EQ(DefaultColor::DC_BLUE, msg.get_c());  // absent -> default
  EXPECT_FALSE(msg.has_a());
}

// A field set to a non-default value round-trips and a default-valued absent
// field stays at its default after a round trip.
TEST(EditionsDefaults, round_trip)
{
  DefaultsMessage msg;
  msg.set_a(100);
  msg.set_c(DefaultColor::DC_GREEN);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  DefaultsMessage result;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  EXPECT_EQ(100, result.get_a());
  EXPECT_EQ(DefaultColor::DC_GREEN, result.get_c());
  // b/f/big were never set: they keep their defaults.
  EXPECT_TRUE(result.get_b());
  EXPECT_FLOAT_EQ(1.5F, result.get_f());
}

// ED-4 repeated_field_encoding ----------------------------------------------

// PACKED (edition 2023 default): a single length-delimited block.
TEST(EditionsRepeated, packed_emits_single_block)
{
  RepeatedEncodingMessage<10, 10> msg;
  msg.add_packed_values(1);
  msg.add_packed_values(2);
  msg.add_packed_values(3);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // field 1, LEN (0x0A), length 3, then 01 02 03.
  const std::array<uint8_t, 5> expected = { 0x0A, 0x03, 0x01, 0x02, 0x03 };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

// EXPANDED: one tag+value per element.
TEST(EditionsRepeated, expanded_emits_tag_per_element)
{
  RepeatedEncodingMessage<10, 10> msg;
  msg.add_expanded_values(1);
  msg.add_expanded_values(2);
  msg.add_expanded_values(3);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // field 2, VARINT (0x10), repeated per element.
  const std::array<uint8_t, 6> expected = { 0x10, 0x01, 0x10, 0x02, 0x10, 0x03 };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

// The decoder accepts both forms for either field. An EXPANDED-emitted buffer and
// a PACKED buffer for the same field both decode to the same vector.
TEST(EditionsRepeated, decode_accepts_both_forms)
{
  // Expanded round trip.
  RepeatedEncodingMessage<10, 10> msg;
  msg.add_expanded_values(1);
  msg.add_expanded_values(2);
  msg.add_expanded_values(3);
  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }
  RepeatedEncodingMessage<10, 10> result;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  ASSERT_EQ(3U, result.get_expanded_values().get_length());
  EXPECT_EQ(1, result.expanded_values(0).get());
  EXPECT_EQ(2, result.expanded_values(1).get());
  EXPECT_EQ(3, result.expanded_values(2).get());

  // Feed a PACKED buffer for the EXPANDED field (field 2, LEN 0x12); it must
  // decode to the same vector.
  ::EmbeddedProto::ReadBufferFixedSize<8> packed_for_expanded(
      {0x12, 0x03, 0x01, 0x02, 0x03});
  RepeatedEncodingMessage<10, 10> result2;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result2.deserialize(packed_for_expanded));
  ASSERT_EQ(3U, result2.get_expanded_values().get_length());
  EXPECT_EQ(1, result2.expanded_values(0).get());
  EXPECT_EQ(3, result2.expanded_values(2).get());
}

// ED-5 enum_type (OPEN / CLOSED) --------------------------------------------

// A CLOSED enum stores a declared value.
TEST(EditionsEnum, closed_accepts_declared_value)
{
  // field 2 (VARINT) value 2 == CE_C.
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x10, 0x02});

  EnumTypeMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_TRUE(msg.has_closed_field());
  EXPECT_EQ(ClosedEnum::CE_C, msg.get_closed_field());
}

// A CLOSED enum drops an undeclared value: the field stays unset / at its default.
TEST(EditionsEnum, closed_drops_unknown_value)
{
  // field 2 (VARINT) value 5 is not a declared ClosedEnum value.
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x10, 0x05});

  EnumTypeMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_FALSE(msg.has_closed_field());
  EXPECT_EQ(ClosedEnum::CE_A, msg.get_closed_field());  // default
}

// A CLOSED enum whose first enumerator is non-zero defaults to that enumerator,
// not a raw zero (which is not a member of the enum).
TEST(EditionsEnum, closed_nonzero_default_is_first_enumerator)
{
  ClosedDefaultMessage msg;
  EXPECT_EQ(ClosedNonZeroEnum::CNZ_A, msg.get_e());
}

// Dropping an unknown value on such a field leaves it at the first enumerator.
TEST(EditionsEnum, closed_nonzero_drop_resets_to_default)
{
  // field 1 (VARINT) value 7 is not a declared ClosedNonZeroEnum value.
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x08, 0x07});
  ClosedDefaultMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_FALSE(msg.has_e());
  EXPECT_EQ(ClosedNonZeroEnum::CNZ_A, msg.get_e());  // dropped -> default
}

// A CLOSED enum oneof member accepts a declared value (control).
TEST(EditionsEnum, closed_oneof_accepts_declared_value)
{
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x08, 0x02});  // ce = CE_C
  ClosedEnumOneofMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_TRUE(msg.has_ce());
  EXPECT_EQ(ClosedEnum::CE_C, msg.get_ce());
}

// Dropping an unknown value on a CLOSED enum oneof member leaves the oneof unset
// rather than reporting the member as present.
TEST(EditionsEnum, closed_oneof_drop_leaves_oneof_unset)
{
  // field 1 (VARINT) value 5 is not a declared ClosedEnum value.
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x08, 0x05});
  ClosedEnumOneofMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_FALSE(msg.has_ce());
  EXPECT_EQ(ClosedEnumOneofMessage::FieldNumber::NOT_SET, msg.get_which_choice());
}

// An OPEN enum (control) stores any value, including undeclared ones.
TEST(EditionsEnum, open_stores_unknown_value)
{
  // field 1 (VARINT) value 5 is not a declared OpenEnum value.
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x08, 0x05});

  EnumTypeMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_TRUE(msg.has_open_field());
  EXPECT_EQ(5U, static_cast<uint32_t>(msg.get_open_field()));
}

// Both enums round-trip a valid value.
TEST(EditionsEnum, round_trip_valid)
{
  EnumTypeMessage msg;
  msg.set_open_field(OpenEnum::OE_B);
  msg.set_closed_field(ClosedEnum::CE_B);

  ::EmbeddedProto::WriteBufferFixedSize<16> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<16> read_buffer;
  for(uint32_t i = 0; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  EnumTypeMessage result;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  EXPECT_EQ(OpenEnum::OE_B, result.get_open_field());
  EXPECT_EQ(ClosedEnum::CE_B, result.get_closed_field());
}

// ED-6 message_encoding = DELIMITED: single-pass encode --------------------

// A DELIMITED nested message is framed with START_GROUP / END_GROUP, no length.
TEST(EditionsDelimited, encode_group_framing)
{
  DelimitedMessage msg;
  msg.mutable_sub().set_x(150);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  // START_GROUP(field 3) = 0x1B, inner x=1 (field 1) = 0x08 0x96 0x01,
  // END_GROUP(field 3) = 0x1C.
  const std::array<uint8_t, 5> expected = { 0x1B, 0x08, 0x96, 0x01, 0x1C };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }
}

// serialized_size() accounts for the group framing (START + END tags).
TEST(EditionsDelimited, serialized_size_matches)
{
  DelimitedMessage msg;
  msg.mutable_sub().set_x(150);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(buffer.get_size(), msg.serialized_size());
}

// An empty (present) delimited message is a START_GROUP immediately followed by
// END_GROUP.
TEST(EditionsDelimited, encode_empty_group)
{
  DelimitedMessage msg;
  msg.mutable_sub().clear();  // present but empty

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  const std::array<uint8_t, 2> expected = { 0x1B, 0x1C };
  ASSERT_EQ(expected.size(), buffer.get_size());
  EXPECT_EQ(expected[0], buffer.get_data()[0]);
  EXPECT_EQ(expected[1], buffer.get_data()[1]);
}

// ED-7 message_encoding = DELIMITED: decode + group skip --------------------

// A DELIMITED buffer decodes back to the original message.
TEST(EditionsDelimited, decode_round_trip)
{
  DelimitedMessage msg;
  msg.mutable_sub().set_x(150);
  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  DelimitedMessage result;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  EXPECT_TRUE(result.has_sub());
  EXPECT_EQ(150, result.get_sub().get_x());
}

// A DELIMITED-encoded message and its LENGTH_PREFIXED twin decode to the same
// field values.
TEST(EditionsDelimited, cross_encoding_equivalence)
{
  // DELIMITED bytes for sub.x = 150.
  ::EmbeddedProto::ReadBufferFixedSize<8> delimited({0x1B, 0x08, 0x96, 0x01, 0x1C});
  DelimitedMessage from_group;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, from_group.deserialize(delimited));

  // LENGTH_PREFIXED bytes for the same field number 3: tag 0x1A, len 3, inner.
  ::EmbeddedProto::ReadBufferFixedSize<8> length_prefixed({0x1A, 0x03, 0x08, 0x96, 0x01});
  LengthPrefixedMessage from_len;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, from_len.deserialize(length_prefixed));

  EXPECT_EQ(from_group.get_sub().get_x(), from_len.get_sub().get_x());
  EXPECT_EQ(150, from_group.get_sub().get_x());
}

// A DELIMITED field that arrives with the wrong wire type (LENGTH_PREFIXED
// instead of START_GROUP) is rejected instead of misreading the length prefix
// as group content.
TEST(EditionsDelimited, wrong_wire_type_rejected)
{
  // Field 3 as LENGTH_DELIMITED (tag 0x1A) although the schema marks it DELIMITED.
  ::EmbeddedProto::ReadBufferFixedSize<8> length_prefixed({0x1A, 0x03, 0x08, 0x96, 0x01});
  DelimitedMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::INVALID_WIRETYPE, msg.deserialize(length_prefixed));
}

// An unknown DELIMITED field is skipped; known fields around it still decode.
TEST(EditionsDelimited, unknown_group_is_skipped)
{
  // before(1)=7, unknown group field 3 with an inner field, after(5)=9.
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer(
      {0x08, 0x07,                   // before = 7
       0x1B, 0x08, 0x96, 0x01, 0x1C, // unknown group (field 3)
       0x28, 0x09});                 // after = 9
  GroupSkipMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.get_before());
  EXPECT_EQ(9, msg.get_after());
}

// Nested unknown groups are skipped to the correct matching END_GROUP.
TEST(EditionsDelimited, nested_unknown_groups_skipped)
{
  // before(1)=7, group(3){ group(4){ x=1 } }, after(5)=9.
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer(
      {0x08, 0x07,             // before = 7
       0x1B,                   // START_GROUP field 3
         0x23,                 // START_GROUP field 4
           0x08, 0x01,         // inner field 1 = 1
         0x24,                 // END_GROUP field 4
       0x1C,                   // END_GROUP field 3
       0x28, 0x09});           // after = 9
  GroupSkipMessage msg;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));
  EXPECT_EQ(7, msg.get_before());
  EXPECT_EQ(9, msg.get_after());
}

// ED-8 message_encoding = DELIMITED: partial state machine ------------------
#ifdef PARTIAL_SERIALIZATION_ENABLED

// Partial-serialize a DELIMITED message through a tiny (chunked) buffer; the
// reassembled output equals the full-mode golden bytes.
TEST(EditionsDelimitedPartial, serialize_chunked)
{
  DelimitedMessage msg;
  msg.mutable_sub().set_x(150);

  DelimitedMessage::StateStack state;
  std::array<uint8_t, 8> out = {0};
  uint32_t out_len = 0;
  ::EmbeddedProto::Error r = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t guard = 0;
  while((::EmbeddedProto::Error::BUFFER_FULL == r) && (guard++ < 32))
  {
    // Small enough to split the group across calls (here the END_GROUP tag lands
    // in a later chunk), but at least as large as the widest atomic scalar field.
    ::EmbeddedProto::WriteBufferFixedSize<4> chunk;
    r = msg.serialize_partial(chunk, state.root());
    for(uint32_t i = 0; i < chunk.get_size(); ++i)
    {
      out[out_len++] = chunk.get_data()[i];
    }
  }
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, r);

  const std::array<uint8_t, 5> expected = { 0x1B, 0x08, 0x96, 0x01, 0x1C };
  ASSERT_EQ(expected.size(), out_len);
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], out[i]);
  }
}

// Partial-deserialize a DELIMITED message fed one byte at a time.
TEST(EditionsDelimitedPartial, deserialize_chunked)
{
  const std::array<uint8_t, 5> bytes = { 0x1B, 0x08, 0x96, 0x01, 0x1C };

  ::EmbeddedProto::ReadBufferFixedSize<8> buffer;
  DelimitedMessage result;
  DelimitedMessage::StateStack state;
  ::EmbeddedProto::Error r = ::EmbeddedProto::Error::END_OF_BUFFER;
  for(uint32_t i = 0; i < bytes.size(); ++i)
  {
    buffer.push(bytes[i]);
    r = result.deserialize_partial(buffer, state.root());
  }
  // After all bytes the parent waits for more input -> END_OF_BUFFER, but the
  // group has been fully decoded.
  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, r);
  EXPECT_TRUE(result.has_sub());
  EXPECT_EQ(150, result.get_sub().get_x());
}

// ED-4 repeated_field_encoding = EXPANDED must be honored by *partial*
// serialization as well. The chunked output must equal the full-mode expanded
// golden bytes (one tag+value per element), not a single packed block.
TEST(EditionsRepeatedPartial, expanded_serialize_chunked)
{
  RepeatedEncodingMessage<10, 10> msg;
  msg.add_expanded_values(1);
  msg.add_expanded_values(2);
  msg.add_expanded_values(3);

  RepeatedEncodingMessage<10, 10>::StateStack state;
  std::array<uint8_t, 16> out = {0};
  uint32_t out_len = 0;
  ::EmbeddedProto::Error r = ::EmbeddedProto::Error::BUFFER_FULL;
  uint32_t guard = 0;
  while((::EmbeddedProto::Error::BUFFER_FULL == r) && (guard++ < 64))
  {
    // Small enough to split the expanded elements across multiple calls.
    ::EmbeddedProto::WriteBufferFixedSize<4> chunk;
    r = msg.serialize_partial(chunk, state.root());
    for(uint32_t i = 0; i < chunk.get_size(); ++i)
    {
      out[out_len++] = chunk.get_data()[i];
    }
  }
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, r);

  // field 2, VARINT (0x10), one tag+value per element -- NOT a packed block.
  const std::array<uint8_t, 6> expected = { 0x10, 0x01, 0x10, 0x02, 0x10, 0x03 };
  ASSERT_EQ(expected.size(), out_len);
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], out[i]);
  }
}

// A group nested deeper than the available state stack returns NESTING_TOO_DEEP
// instead of crashing.
TEST(EditionsDelimitedPartial, nesting_too_deep)
{
  // DelimitedMessage needs depth 2 (itself + Inner). A depth-1 stack has no child
  // state for the group body.
  ::EmbeddedProto::MessageStateStack<1> shallow;

  ::EmbeddedProto::ReadBufferFixedSize<8> buffer({0x1B, 0x08, 0x96, 0x01, 0x1C});
  DelimitedMessage result;
  EXPECT_EQ(::EmbeddedProto::Error::NESTING_TOO_DEEP,
            result.deserialize_partial(buffer, shallow.root()));
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // End of namespace test_EmbeddedAMS_editions
