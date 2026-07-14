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

#include <Fields.h>
#include <RepeatedField.h>
#include <RepeatedFieldCallback.h>
#include <RepeatedFieldFixedSize.h>
#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>
#include <WireFormatter.h>
#include <Errors.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace test_EmbeddedAMS_RepeatedFieldCallback
{

using ::EmbeddedProto::Error;
using ::EmbeddedProto::int32;
using Callback = ::EmbeddedProto::RepeatedFieldCallback<int32>;

// A callback field must be usable anywhere a RepeatedField<int32> is expected
// (this is what the customStorage static_assert relies on).
static_assert(std::is_base_of<::EmbeddedProto::RepeatedField<int32>, Callback>::value,
              "RepeatedFieldCallback must derive from RepeatedField.");

TEST(RepeatedFieldCallback, default_state)
{
  Callback field;

  EXPECT_EQ(0U, field.get_length());
  EXPECT_EQ(0U, field.get_size());
  EXPECT_FALSE(field.is_source_set());
  EXPECT_FALSE(field.is_sink_set());

  // Streaming is unbounded, so the engine's capacity checks must never trip.
  EXPECT_GT(field.get_max_length(), field.get_length());
}

TEST(RepeatedFieldCallback, add_counts_elements)
{
  Callback field;
  int32 value;
  value.set(42);

  EXPECT_EQ(Error::NO_ERRORS, field.add(value));
  EXPECT_EQ(Error::NO_ERRORS, field.add(value));
  EXPECT_EQ(2U, field.get_length());
}

TEST(RepeatedFieldCallback, clear_resets_length)
{
  Callback field;
  int32 value;
  value.set(7);
  (void)field.add(value);
  (void)field.add(value);
  ASSERT_EQ(2U, field.get_length());

  field.clear();
  EXPECT_EQ(0U, field.get_length());
}

TEST(RepeatedFieldCallback, random_access_is_unsupported)
{
  Callback field;
  int32 out;
  EXPECT_EQ(Error::INDEX_OUT_OF_BOUND, field.get_const(0U, out));

  // set_data cannot store into a streaming field.
  int32 data[2];
  EXPECT_EQ(Error::ARRAY_FULL, field.set_data(data, 2U));
}

bool produce_nothing(int32& element)
{
  static_cast<void>(element);
  return false;
}

Error swallow(const int32& element)
{
  static_cast<void>(element);
  return Error::NO_ERRORS;
}

TEST(RepeatedFieldCallback, source_binds_and_clears)
{
  Callback field;
  EXPECT_FALSE(field.is_source_set());

  Callback::SourceCallback source;
  source.set(&produce_nothing);
  field.set_source(source);
  EXPECT_TRUE(field.is_source_set());

  field.clear_source();
  EXPECT_FALSE(field.is_source_set());
}

TEST(RepeatedFieldCallback, sink_binds_and_clears)
{
  Callback field;
  EXPECT_FALSE(field.is_sink_set());

  Callback::SinkCallback sink;
  sink.set(&swallow);
  field.set_sink(sink);
  EXPECT_TRUE(field.is_sink_set());

  field.clear_sink();
  EXPECT_FALSE(field.is_sink_set());
}

TEST(RepeatedFieldCallback, clear_keeps_bindings)
{
  Callback field;
  Callback::SinkCallback sink;
  sink.set(&swallow);
  field.set_sink(sink);

  int32 value;
  value.set(1);
  (void)field.add(value);

  field.clear();
  EXPECT_EQ(0U, field.get_length());
  EXPECT_TRUE(field.is_sink_set());
}

// --- Step 2: deserialize (push) --------------------------------------------

using ::EmbeddedProto::WireFormatter;

//! Sink that records every element it receives, in order, into a fixed buffer
//! (no dynamic allocation, so it is representative of MCU usage).
template<std::size_t N>
struct Collector
{
  std::array<int32_t, N> values{};
  std::size_t count = 0U;
  Error operator()(const int32& element)
  {
    Error return_value = Error::ARRAY_FULL;
    if(count < N)
    {
      values[count] = element.get();
      ++count;
      return_value = Error::NO_ERRORS;
    }
    return return_value;
  }
};

//! Assert a Collector received exactly the expected values, in order.
template<std::size_t N>
static void expect_collected(const Collector<N>& collector,
                             std::initializer_list<int32_t> expected)
{
  ASSERT_EQ(expected.size(), collector.count);
  std::size_t i = 0U;
  for(int32_t value : expected)
  {
    EXPECT_EQ(value, collector.values[i]);
    ++i;
  }
}

TEST(RepeatedFieldCallback, deserialize_packed_streams_to_sink)
{
  Collector<8> collector;
  Callback::SinkCallback sink;
  sink.set(collector);

  Callback field;
  field.set_sink(sink);

  // Packed block: [size=3][1][2][3] (each value a single-byte varint).
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x03U, 0x01U, 0x02U, 0x03U});
  EXPECT_EQ(Error::NO_ERRORS,
            field.deserialize_check_type(buffer, WireFormatter::WireType::LENGTH_DELIMITED));

  expect_collected(collector, {1, 2, 3});
  EXPECT_EQ(3U, field.get_length());
}

TEST(RepeatedFieldCallback, deserialize_expanded_streams_to_sink)
{
  Collector<8> collector;
  Callback::SinkCallback sink;
  sink.set(collector);

  Callback field;
  field.set_sink(sink);

  // Expanded: the message loop calls deserialize_check_type once per element
  // with the element's own (VARINT) wire type. Feed three single-byte varints.
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x0AU, 0x14U, 0x1EU});
  for(uint32_t i = 0; i < 3U; ++i)
  {
    EXPECT_EQ(Error::NO_ERRORS,
              field.deserialize_check_type(buffer, WireFormatter::WireType::VARINT));
  }

  expect_collected(collector, {10, 20, 30});
  EXPECT_EQ(3U, field.get_length());
}

TEST(RepeatedFieldCallback, deserialize_without_sink_discards)
{
  Callback field; // no sink bound, not strict

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x03U, 0x01U, 0x02U, 0x03U});
  EXPECT_EQ(Error::NO_ERRORS,
            field.deserialize_check_type(buffer, WireFormatter::WireType::LENGTH_DELIMITED));

  // Elements were drained (counted) but not stored anywhere.
  EXPECT_EQ(3U, field.get_length());
}

TEST(RepeatedFieldCallback, deserialize_strict_without_sink_errors)
{
  Callback field;
  field.set_strict(true);

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x03U, 0x01U, 0x02U, 0x03U});
  EXPECT_EQ(Error::CALLBACK_NOT_SET,
            field.deserialize_check_type(buffer, WireFormatter::WireType::LENGTH_DELIMITED));
  EXPECT_EQ(0U, field.get_length());
}

// --- Step 3: serialize (pull) as EXPANDED + sequence guard -----------------

//! Source that yields a fixed list of values, then signals end-of-stream. Uses
//! a statically sized buffer (no dynamic allocation), like MCU code would.
template<std::size_t N>
struct Producer
{
  std::array<int32_t, N> values{};
  std::size_t size = 0U;
  std::size_t index = 0U;
  bool operator()(int32& element)
  {
    const bool produced = index < size;
    if(produced)
    {
      element.set(values[index]);
      ++index;
    }
    return produced;
  }
};

//! Serialize a resident field EXPANDED the same way the generator does: one
//! serialize_with_id() per element. Used as the reference byte stream.
static void serialize_reference_expanded(const ::EmbeddedProto::RepeatedField<int32>& field,
                                         uint32_t field_number,
                                         ::EmbeddedProto::WriteBufferInterface& buffer)
{
  for(uint32_t i = 0U; i < field.get_length(); ++i)
  {
    (void)field.get_const(i).serialize_with_id(field_number, buffer, true);
  }
}

TEST(RepeatedFieldCallback, serialize_expanded_matches_resident_field)
{
  const uint32_t field_number = 5U;

  // Reference: a resident field holding {1, 2, 3} serialized EXPANDED.
  ::EmbeddedProto::RepeatedFieldFixedSize<int32, 3> resident;
  for(int32_t v : {1, 2, 3})
  {
    int32 value;
    value.set(v);
    (void)resident.add(value);
  }
  ::EmbeddedProto::WriteBufferFixedSize<32> expected;
  serialize_reference_expanded(resident, field_number, expected);

  // Actual: a callback field pulling the same values from a source.
  Producer<3> producer;
  producer.values = {1, 2, 3};
  producer.size = 3U;
  Callback::SourceCallback source;
  source.set(producer);

  Callback field;
  field.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<32> actual;
  EXPECT_EQ(Error::NO_ERRORS, field.serialize_expanded(field_number, actual));

  ASSERT_EQ(expected.get_size(), actual.get_size());
  EXPECT_EQ(0, std::memcmp(expected.get_data(), actual.get_data(), expected.get_size()));
  EXPECT_EQ(3U, field.get_length());
}

TEST(RepeatedFieldCallback, serialize_expanded_without_source_emits_nothing)
{
  Callback field; // no source bound, not strict

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(Error::NO_ERRORS, field.serialize_expanded(7U, buffer));
  EXPECT_EQ(0U, buffer.get_size());
  EXPECT_EQ(0U, field.get_length());
}

TEST(RepeatedFieldCallback, serialize_expanded_strict_without_source_errors)
{
  Callback field;
  field.set_strict(true);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(Error::CALLBACK_NOT_SET, field.serialize_expanded(7U, buffer));
  EXPECT_EQ(0U, buffer.get_size());
}

TEST(RepeatedFieldCallback, packed_or_size_pass_is_rejected)
{
  // The base serialize() is what a packed serialize and a size pass reach; a
  // streaming field must refuse it rather than drain the source twice.
  Producer<3> producer;
  producer.values = {1, 2, 3};
  producer.size = 3U;
  Callback::SourceCallback source;
  source.set(producer);

  Callback field;
  field.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  EXPECT_EQ(Error::CALLBACK_SEQUENCE, field.serialize(buffer));
  EXPECT_EQ(0U, buffer.get_size());
  EXPECT_EQ(0U, field.get_length());
}

} // namespace test_EmbeddedAMS_RepeatedFieldCallback
