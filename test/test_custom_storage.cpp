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

// These tests cover the customStorage option. The option lets the user supply the storage type
// for repeated, string, bytes and message fields. The supplied type must derive from the matching
// base class. The generated message becomes templated on these storage types and exposes no size
// template parameter for the customStorage fields.

#include "gtest/gtest.h"

#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>

#include <MockStringStorage.h>
#include <MockBytesStorage.h>
#include <MockRepeatedFieldStorage.h>

#include <cstdint>
#include <array>

// EAMS message definitions
#include <custom_storage.h>

namespace test_EmbeddedAMS_CustomStorage
{

// Convenience aliases for the storage types used in the tests.
using RepStorage = Mocks::MockRepeatedFieldStorage<EmbeddedProto::uint32, 4U>;
using StrStorage = Mocks::MockStringStorage<16U>;
using BytStorage = Mocks::MockBytesStorage<16U>;
using MsgStorage = custom_storage::Nested; // A plain message satisfies the MessageInterface base.

using CustomMsg = custom_storage::CustomStorageMsg<RepStorage, StrStorage, BytStorage, MsgStorage>;

// The generated message must be usable without any size template argument: the user owns the size
// through the storage types.
TEST(CustomStorage, construction)
{
  CustomMsg msg;
  EXPECT_EQ(0U, msg.values().get_length());
  EXPECT_EQ(0U, msg.get_text().get_length());
  EXPECT_EQ(0U, msg.get_data().get_length());
  EXPECT_EQ(0, msg.get_nested().get_value());
}

TEST(CustomStorage, roundtrip_all_fields)
{
  CustomMsg msg;
  msg.mutable_values().add(10U);
  msg.mutable_values().add(20U);
  msg.mutable_text().set("hi");
  const uint8_t raw[] = {0x01, 0x02, 0x03};
  msg.mutable_data().set(raw, 3U);
  msg.mutable_nested().set_value(7);

  ::EmbeddedProto::WriteBufferFixedSize<128> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<128> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  CustomMsg result;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));

  ASSERT_EQ(2U, result.values().get_length());
  EXPECT_EQ(10U, result.values().get_const(0U));
  EXPECT_EQ(20U, result.values().get_const(1U));
  EXPECT_STREQ("hi", result.text());
  ASSERT_EQ(3U, result.get_data().get_length());
  EXPECT_EQ(0x01, result.get_data().get_const(0U));
  EXPECT_EQ(0x03, result.get_data().get_const(2U));
  EXPECT_EQ(7, result.get_nested().get_value());
}

// The repeated field uses the user supplied storage type and not the default fixed-size storage.
TEST(CustomStorage, repeated_uses_supplied_storage)
{
  CustomMsg msg;
  // The supplied storage limits the array to four elements.
  EXPECT_EQ(4U, msg.values().get_max_length());

  for(uint32_t i = 0U; i < 4U; ++i)
  {
    EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.mutable_values().add(i));
  }
  // The fifth element does not fit anymore.
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, msg.mutable_values().add(99U));
}

TEST(CustomStorage, clear_resets_fields)
{
  CustomMsg msg;
  msg.mutable_values().add(1U);
  msg.mutable_text().set("data");
  msg.mutable_nested().set_value(5);

  msg.clear();

  EXPECT_EQ(0U, msg.values().get_length());
  EXPECT_EQ(0U, msg.get_text().get_length());
  EXPECT_EQ(0, msg.get_nested().get_value());
}

// -----------------------------------------------------------------------------
// optional + customStorage

using OptionalMsg = custom_storage::OptionalCustomStorageMsg<StrStorage>;

TEST(CustomStorage, optional_present_serialize)
{
  OptionalMsg msg;
  EXPECT_FALSE(msg.has_text());

  msg.mutable_text().set("here");
  EXPECT_TRUE(msg.has_text());

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  // tag(0x0a) + size(0x04) + "here"
  EXPECT_EQ(6U, buffer.get_size());

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  OptionalMsg result;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  EXPECT_TRUE(result.has_text());
  EXPECT_STREQ("here", result.text());
}

TEST(CustomStorage, optional_absent_not_serialized)
{
  OptionalMsg msg;

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
  // An absent optional field is not serialized at all.
  EXPECT_EQ(0U, buffer.get_size());
}

TEST(CustomStorage, optional_clear)
{
  OptionalMsg msg;
  msg.mutable_text().set("value");
  EXPECT_TRUE(msg.has_text());

  msg.clear_text();
  EXPECT_FALSE(msg.has_text());
  EXPECT_EQ(0U, msg.get_text().get_length());
}

// -----------------------------------------------------------------------------
// oneof + customStorage

using OneofMsg = custom_storage::OneofCustomStorageMsg<StrStorage>;

TEST(CustomStorage, oneof_select_customStorage_arm)
{
  OneofMsg msg;
  EXPECT_EQ(OneofMsg::FieldNumber::NOT_SET, msg.get_which_choice());

  msg.mutable_one_text().set("oneof");
  EXPECT_EQ(OneofMsg::FieldNumber::ONE_TEXT, msg.get_which_choice());
  EXPECT_STREQ("oneof", msg.one_text());
}

// Switching to another arm must clear the customStorage object: the set callbacks/data are gone.
TEST(CustomStorage, oneof_switch_clears_previous_arm)
{
  OneofMsg msg;
  msg.mutable_one_text().set("oneof");
  ASSERT_EQ(OneofMsg::FieldNumber::ONE_TEXT, msg.get_which_choice());

  msg.set_one_scalar(42);
  EXPECT_EQ(OneofMsg::FieldNumber::ONE_SCALAR, msg.get_which_choice());
  EXPECT_EQ(42, msg.one_scalar());

  // Switching back gives a fresh, empty storage object.
  msg.mutable_one_text().set("again");
  EXPECT_EQ(OneofMsg::FieldNumber::ONE_TEXT, msg.get_which_choice());
  EXPECT_STREQ("again", msg.one_text());
}

TEST(CustomStorage, oneof_clear)
{
  OneofMsg msg;
  msg.mutable_one_text().set("value");
  ASSERT_EQ(OneofMsg::FieldNumber::ONE_TEXT, msg.get_which_choice());

  msg.clear_one_text();
  EXPECT_EQ(OneofMsg::FieldNumber::NOT_SET, msg.get_which_choice());
}

TEST(CustomStorage, oneof_roundtrip_customStorage_arm)
{
  OneofMsg msg;
  msg.mutable_one_text().set("rt");

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  OneofMsg result;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, result.deserialize(read_buffer));
  ASSERT_EQ(OneofMsg::FieldNumber::ONE_TEXT, result.get_which_choice());
  EXPECT_STREQ("rt", result.one_text());
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

// Partial serialization must work through the custom storage types as it does for the defaults.
TEST(CustomStorage, partial_serialize_resumes_after_full_buffer)
{
  CustomMsg msg;
  msg.mutable_text().set("partial text");

  // A buffer too small to hold everything in one go forces the partial path to resume.
  ::EmbeddedProto::WriteBufferFixedSize<4> small_buffer;
  CustomMsg::StateStack state;

  ::EmbeddedProto::Error result = msg.serialize_partial(small_buffer, state.root());
  EXPECT_EQ(::EmbeddedProto::Error::BUFFER_FULL, result);

  // Collect everything across multiple small buffers.
  ::EmbeddedProto::WriteBufferFixedSize<128> collected;
  for(uint32_t i = 0U; i < small_buffer.get_size(); ++i)
  {
    collected.push(small_buffer.get_data()[i]);
  }

  while(::EmbeddedProto::Error::BUFFER_FULL == result)
  {
    small_buffer.clear();
    result = msg.serialize_partial(small_buffer, state.root());
    for(uint32_t i = 0U; i < small_buffer.get_size(); ++i)
    {
      collected.push(small_buffer.get_data()[i]);
    }
  }
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, result);

  ::EmbeddedProto::ReadBufferFixedSize<128> read_buffer;
  for(uint32_t i = 0U; i < collected.get_size(); ++i)
  {
    read_buffer.push(collected.get_data()[i]);
  }

  CustomMsg deserialized;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, deserialized.deserialize(read_buffer));
  EXPECT_STREQ("partial text", deserialized.text());
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_CustomStorage
