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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <EmbeddedProto/Fields.h>
#include <EmbeddedProto/RepeatedFieldFixedSize.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <EmbeddedProto/MessageState.h>

#include <WriteBufferMock.h>
#include "mock/ReadBufferMock.h"

#include <array>
#include <cstring>

namespace test_EmbeddedAMS_RepeatedFieldFixedSize
{

using ::testing::_;
using ::testing::An;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

static constexpr int32_t UINT32_SIZE = sizeof(::EmbeddedProto::uint32);

TEST(RepeatedFieldFixedSize, construction) 
{
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  // Copy to the same size
  x.add(1);
  x.add(2);
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> y(x);
  EXPECT_EQ(2, y.get_length());
  EXPECT_EQ(1U, y.get_const(0));
  EXPECT_EQ(2U, y.get_const(1));

  // Copy to a larger array
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 2*LENGTH> z(x);
  

}

TEST(RepeatedFieldFixedSize, size_uint32_t) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  auto size = x.get_size();
  EXPECT_EQ(0, size);

  auto max_size = x.get_max_size();
  EXPECT_EQ(LENGTH*UINT32_SIZE, max_size);

  auto length = x.get_length();
  EXPECT_EQ(0, length);

  auto max_length = x.get_max_length();
  EXPECT_EQ(LENGTH, max_length);
}

TEST(RepeatedFieldFixedSize, add_data) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  x.add(1);
  x.add(2);
  EXPECT_EQ(2*UINT32_SIZE, x.get_size());
  EXPECT_EQ(2, x.get_length());

  auto result = x.add(3);
  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, result);

  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_size());
  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_max_size());
  EXPECT_EQ(LENGTH, x.get_length());
  EXPECT_EQ(LENGTH, x.get_max_length());

  // Check if we can add more than the limit.
  result = x.add(4);
  EXPECT_EQ(EmbeddedProto::Error::ARRAY_FULL, result);
}

TEST(RepeatedFieldFixedSize, get) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  // Get (non-const) should modify the number of items in the array.

  // Set the second element
  x.get(1) = 1;
  EXPECT_EQ(2, x.get_length());
  EXPECT_EQ(0U, x.get_const(0));
  EXPECT_EQ(1U, x.get_const(1));

  // When going out of bound the last element should be returned.
  EXPECT_EQ(0U, x.get_const(2));
  x.get(3) = 3U;
  EXPECT_EQ(3U, x.get_const(2));
  EXPECT_EQ(3U, x.get_const(3));

  // Test the get_const which will return out of bound errors.
  ::EmbeddedProto::uint32 value = 0;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, x.get_const(1, value));
  EXPECT_EQ(1, value);

  value = 99;
  EXPECT_EQ(::EmbeddedProto::Error::INDEX_OUT_OF_BOUND, x.get_const(3, value));
  // Value should not have changed.
  EXPECT_EQ(99, value);
}

TEST(RepeatedFieldFixedSize, set_data_array) 
{  
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  EmbeddedProto::uint32 data3[] = {1, 2, 3};

  auto result = x.set_data(&(data3[0]), 3U);
  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, result);  

  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_size());
  EXPECT_EQ(LENGTH*UINT32_SIZE, x.get_max_size());
  EXPECT_EQ(LENGTH, x.get_length());
  EXPECT_EQ(LENGTH, x.get_max_length());

  // Check if we can add more than the limit.
  EmbeddedProto::uint32 data4[] = {1, 2, 3, 4};
  result = x.set_data(&(data4[0]), 4U);
  EXPECT_EQ(EmbeddedProto::Error::ARRAY_FULL, result);
}

TEST(RepeatedFieldFixedSize, set_element) 
{
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;

  // First add a value in the middle and see if we have a size of two.
  x.set(1, 2);
  EXPECT_EQ(2U, x.get_const(1));
  EXPECT_EQ(2U, x.get_length());

  x.set(0, 1);
  EXPECT_EQ(1U, x.get_const(0));

  x.set(2, 3);
  EXPECT_EQ(3U, x.get_const(2));
}

TEST(RepeatedFieldFixedSize, clear)
{
  static constexpr uint32_t LENGTH = 3;
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, LENGTH> x;
  x.add(1);
  x.add(2);
  x.clear();
  EXPECT_EQ(0U, x.get_const(0));
  EXPECT_EQ(0U, x.get_const(1));
  EXPECT_EQ(0U, x.get_length());
}

// --------------------------------------------------------------------------
// Batched push for packed repeated fixed-width fields.
//
// These tests exercise the packed serialization at the buffer seam: a packed
// fixed-width field must reach the buffer as batched push(bytes, length) calls
// rather than one virtual push(byte) per byte, while remaining byte-identical
// and wire-compatible with the previous per-byte implementation.
// --------------------------------------------------------------------------

#if EMBEDDED_PROTO_LITTLE_ENDIAN
// On a little-endian target the whole packed block is written with a single
// push(bytes, length) call; no per-byte push(byte) calls are made.
TEST(RepeatedFieldPacked, full_serialize_fixed32_is_one_whole_block_push)
{
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 4> field;
  field.add(0x11223344U);
  field.add(0x55667788U);
  field.add(0x99AABBCCU);
  field.add(0xDDEEFF00U);

  Mocks::WriteBufferMock buffer;
  // Whole 4 * 4 = 16 byte payload in a single array push, and never a per-byte push.
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_, 16U)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, field.serialize(buffer));
}

TEST(RepeatedFieldPacked, full_serialize_fixed64_is_one_whole_block_push)
{
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed64, 3> field;
  field.add(0x0102030405060708ULL);
  field.add(0x1122334455667788ULL);
  field.add(0xAABBCCDDEEFF0011ULL);

  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_, 24U)).Times(1).WillOnce(Return(true));

  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, field.serialize(buffer));
}
#endif // EMBEDDED_PROTO_LITTLE_ENDIAN

// The serialized bytes are the little-endian packed layout, byte-for-byte, and
// round-trip cleanly back through deserialize (wire compatible).
TEST(RepeatedFieldPacked, full_serialize_fixed32_bytes_and_roundtrip)
{
  const std::array<uint32_t, 4> values = {0x11223344U, 0x55667788U, 0x99AABBCCU, 0xDDEEFF00U};

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 8> field;
  for(const uint32_t v : values) { field.add(v); }

  EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(EmbeddedProto::Error::NO_ERRORS, field.serialize(buffer));
  ASSERT_EQ(values.size() * 4U, buffer.get_size());

  // Expected little-endian packed layout.
  const uint8_t* data = buffer.get_data();
  for(uint32_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(static_cast<uint8_t>(values[i] & 0xFF),         data[(i * 4) + 0]);
    EXPECT_EQ(static_cast<uint8_t>((values[i] >> 8) & 0xFF),  data[(i * 4) + 1]);
    EXPECT_EQ(static_cast<uint8_t>((values[i] >> 16) & 0xFF), data[(i * 4) + 2]);
    EXPECT_EQ(static_cast<uint8_t>((values[i] >> 24) & 0xFF), data[(i * 4) + 3]);
  }

  // Round-trip: [size varint][packed data] -> deserialize.
  EmbeddedProto::ReadBufferFixedSize<64> read;
  read.push(static_cast<uint8_t>(values.size() * 4U)); // size fits in one varint byte
  for(uint32_t i = 0; i < buffer.get_size(); ++i) { read.push(data[i]); }

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 8> restored;
  ASSERT_EQ(EmbeddedProto::Error::NO_ERRORS, restored.deserialize(read));
  ASSERT_EQ(values.size(), restored.get_length());
  for(uint32_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(values[i], restored.get_const(i));
  }
}

// Float payloads survive the batched path bit-for-bit.
TEST(RepeatedFieldPacked, full_serialize_float_roundtrip)
{
  const std::array<float, 3> values = {1.5F, -2.25F, 3.0e10F};

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::floatfixed, 4> field;
  for(const float v : values) { field.add(v); }

  EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(EmbeddedProto::Error::NO_ERRORS, field.serialize(buffer));
  ASSERT_EQ(values.size() * 4U, buffer.get_size());

  EmbeddedProto::ReadBufferFixedSize<64> read;
  read.push(static_cast<uint8_t>(values.size() * 4U));
  for(uint32_t i = 0; i < buffer.get_size(); ++i) { read.push(buffer.get_data()[i]); }

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::floatfixed, 4> restored;
  ASSERT_EQ(EmbeddedProto::Error::NO_ERRORS, restored.deserialize(read));
  ASSERT_EQ(values.size(), restored.get_length());
  for(uint32_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(values[i], restored.get_const(i));
  }
}

// A packed varint (non-fixed) field is unaffected: it still serializes
// element-by-element through the base implementation and round-trips.
TEST(RepeatedFieldPacked, full_serialize_varint_unaffected)
{
  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 4> field;
  field.add(1);
  field.add(300);   // two-byte varint
  field.add(2);

  EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(EmbeddedProto::Error::NO_ERRORS, field.serialize(buffer));
  // 1 + 2 + 1 = 4 bytes of varint data.
  EXPECT_EQ(4U, buffer.get_size());
}

// --------------------------------------------------------------------------
// Batched read for packed repeated fixed-width deserialization (receive side).
// --------------------------------------------------------------------------

#if EMBEDDED_PROTO_LITTLE_ENDIAN
// On a little-endian target a full packed fixed-width block is read out of the
// buffer with a single batched pop(bytes, length) call. The only per-byte peek
// is the length prefix; the payload is never read byte-by-byte.
TEST(RepeatedFieldPacked, full_deserialize_fixed32_is_one_block_pop)
{
  const std::array<uint32_t, 4> values = {0x11223344U, 0x55667788U, 0x99AABBCCU, 0xDDEEFF00U};
  const uint8_t payload[16] = {0x44, 0x33, 0x22, 0x11, 0x88, 0x77, 0x66, 0x55,
                               0xCC, 0xBB, 0xAA, 0x99, 0x00, 0xFF, 0xEE, 0xDD};

  NiceMock<Mocks::ReadBufferMock> buffer;
  ON_CALL(buffer, get_size()).WillByDefault(Return(16));

  // Size varint (0x10 == 16) is the only per-byte peek.
  EXPECT_CALL(buffer, peek(_, _)).Times(1)
      .WillOnce(DoAll(SetArgReferee<1>(0x10), Return(true)));
  EXPECT_CALL(buffer, advance(1)).Times(1).WillOnce(Return(true));
  // Whole 16 byte block read in a single batched pop, never a per-byte pop.
  EXPECT_CALL(buffer, pop(_, 16U)).Times(1).WillOnce(
      [&](uint8_t* dst, uint32_t n){ memcpy(dst, payload, n); return true; });
  EXPECT_CALL(buffer, pop(An<uint8_t&>())).Times(0);

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 8> field;
  ASSERT_EQ(EmbeddedProto::Error::NO_ERRORS, field.deserialize(buffer));
  ASSERT_EQ(values.size(), field.get_length());
  for(uint32_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(values[i], field.get_const(i));
  }
}
#endif // EMBEDDED_PROTO_LITTLE_ENDIAN

#ifdef PARTIAL_SERIALIZATION_ENABLED
// Partial (chunked) packed serialization must batch per element and resume at
// the correct element on BUFFER_FULL. Using 4-byte chunks forces every fixed32
// element to be written by an atomic, exactly-filling push; the reassembled
// output equals the full-mode packed encoding (tag + size + little-endian data).
TEST(RepeatedFieldPacked, partial_serialize_fixed32_resumes_per_element)
{
  const std::array<uint32_t, 4> values = {0x11223344U, 0x55667788U, 0x99AABBCCU, 0xDDEEFF00U};

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 8> field;
  for(const uint32_t v : values) { field.add(v); }

  EmbeddedProto::MessageState state;
  std::array<uint8_t, 32> out = {0};
  uint32_t out_len = 0;
  uint32_t guard = 0;
  EmbeddedProto::Error r = EmbeddedProto::Error::NO_ERRORS;
  while((EmbeddedProto::FieldProcessingPhase::COMPLETE != state.phase) && (guard++ < 100))
  {
    // 4-byte chunks: holds tag+size in one call, then exactly one element each.
    EmbeddedProto::WriteBufferFixedSize<4> chunk;
    r = field.serialize_partial_as_field(1, chunk, state, false);
    ASSERT_TRUE((EmbeddedProto::Error::NO_ERRORS == r)
                || (EmbeddedProto::Error::BUFFER_FULL == r));
    for(uint32_t i = 0; i < chunk.get_size(); ++i) { out[out_len++] = chunk.get_data()[i]; }
  }
  EXPECT_EQ(EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);

  // Golden: tag (field 1, LEN) = 0x0A, size = 16, then the LE fixed32 payload.
  std::array<uint8_t, 2 + (4 * 4)> expected = {0x0A, 0x10};
  for(uint32_t i = 0; i < values.size(); ++i)
  {
    expected[2 + (i * 4) + 0] = static_cast<uint8_t>(values[i] & 0xFF);
    expected[2 + (i * 4) + 1] = static_cast<uint8_t>((values[i] >> 8) & 0xFF);
    expected[2 + (i * 4) + 2] = static_cast<uint8_t>((values[i] >> 16) & 0xFF);
    expected[2 + (i * 4) + 3] = static_cast<uint8_t>((values[i] >> 24) & 0xFF);
  }
  ASSERT_EQ(expected.size(), out_len);
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], out[i]) << "byte " << i;
  }
}

// Partial (chunked) packed deserialization must batch fixed-width reads yet keep
// resuming correctly across a buffer refill, including when a fixed-width element
// straddles the split. The all-or-nothing batched read must stop cleanly at the
// section boundary (END_OF_BUFFER) leaving element_index/bytes_remaining pointing
// at the straddling element, which is then read whole from the next buffer.
TEST(RepeatedFieldPacked, partial_deserialize_fixed32_resumes_across_split)
{
  const std::array<uint32_t, 4> values = {0x11223344U, 0x55667788U, 0x99AABBCCU, 0xDDEEFF00U};

  EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 8> field;
  EmbeddedProto::MessageState state;
  // Enter as the message loop would after reading the field tag.
  state.phase = EmbeddedProto::FieldProcessingPhase::SIZE;

  // Buffer 1: size prefix (0x10 == 16) + element 0 + the first 2 bytes of
  // element 1 (element 1 straddles the split).
  EmbeddedProto::ReadBufferFixedSize<7> buffer1(
      { 0x10, 0x44, 0x33, 0x22, 0x11, 0x88, 0x77 });

  EXPECT_EQ(EmbeddedProto::Error::END_OF_BUFFER,
            field.deserialize_partial_as_field(buffer1, state));

  // Only element 0 fully present: the straddling element is not half-consumed.
  EXPECT_EQ(EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(1U, state.element_index);
  EXPECT_EQ(12U, state.bytes_remaining);
  ASSERT_EQ(1U, field.get_length());
  EXPECT_EQ(values[0], field.get_const(0));

  // Buffer 2: the retained tail re-presents element 1 whole, then elements 2 & 3.
  EmbeddedProto::ReadBufferFixedSize<12> buffer2(
      { 0x88, 0x77, 0x66, 0x55, 0xCC, 0xBB, 0xAA, 0x99, 0x00, 0xFF, 0xEE, 0xDD });

  EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS,
            field.deserialize_partial_as_field(buffer2, state));

  EXPECT_EQ(EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  ASSERT_EQ(values.size(), field.get_length());
  for(uint32_t i = 0; i < values.size(); ++i)
  {
    EXPECT_EQ(values[i], field.get_const(i));
  }
}
#endif // PARTIAL_SERIALIZATION_ENABLED


// --------------------------------------------------------------------------
// Zero capacity. A field with MAX_LENGTH 0 holds no element and reserves no
// element storage on any standard library. Out of range access lands on a
// scratch element and never changes the length.
// --------------------------------------------------------------------------

static_assert(sizeof(::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 0>)
                <= sizeof(::EmbeddedProto::RepeatedField<::EmbeddedProto::uint32>) + sizeof(uint32_t) + alignof(void*),
              "A zero capacity repeated field must not reserve element storage.");

TEST(RepeatedFieldZeroLength, holds_nothing)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 0> field;
  EXPECT_EQ(0U, field.get_length());
  EXPECT_EQ(0U, field.get_max_length());
  EXPECT_EQ(0U, field.get_size());
  EXPECT_EQ(0U, field.get_max_size());

  ::EmbeddedProto::uint32 value = 7;
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, field.add(value));
  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, field.set_data(&value, 1));
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, field.set_data(&value, 0));
  EXPECT_EQ(::EmbeddedProto::Error::INDEX_OUT_OF_BOUND, field.get_const(0, value));
  EXPECT_EQ(::EmbeddedProto::Error::INDEX_OUT_OF_BOUND, field.erase(0));

  // Out of range access is memory safe, reads back the default and leaves the length zero.
  field.set(5, value);
  field.get(5) = 9;
  EXPECT_EQ(0U, field.get_const(5).get());
  EXPECT_EQ(0U, field.get_length());
  field.clear();
  EXPECT_EQ(0U, field.get_length());

  // Tag plus a zero size, nothing else.
  EXPECT_EQ(2U, field.max_serialized_size(1));
}

TEST(RepeatedFieldZeroLength, serialize_pushes_nothing)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::fixed32, 0> fixed;
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 0> varint;

  Mocks::WriteBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_, _)).Times(0);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, fixed.serialize(buffer));
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, varint.serialize(buffer));
}

TEST(RepeatedFieldZeroLength, deserialize_one_element_is_array_full)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<::EmbeddedProto::uint32, 0> field;
  // Packed block: size 1, one varint element with value 5.
  ::EmbeddedProto::ReadBufferFixedSize<4> buffer({0x01, 0x05});

  EXPECT_EQ(::EmbeddedProto::Error::ARRAY_FULL, field.deserialize(buffer));
  EXPECT_EQ(0U, field.get_length());
}

} // End namespace test_EmbeddedAMS_RepeatedField
