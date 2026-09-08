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

//! End-to-end proof of the dedicated callbackStorage generator option.
/*!
  The message field is emitted by the generator as a concrete RepeatedFieldCallback (no
  hand-written template argument). Unlike the customStorage e2e, serialize-out works here:
  the generated serialize() pulls from a bound source and emits EXPANDED, so a full
  source -> serialize -> deserialize -> sink round-trip runs through the generated message.
*/

#include "gtest/gtest.h"

#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>
#include <RepeatedFieldFixedSize.h>
#include <RepeatedFieldCallback.h>
#include <WireFormatter.h>
#include <MessageState.h>
#include <Errors.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <callback_storage_option.h>

namespace test_EmbeddedAMS_CallbackStorageOption
{

using ::EmbeddedProto::Error;
using ::EmbeddedProto::int32;

using Msg = callback_storage_option::CallbackOptionMsg;
using Field = ::EmbeddedProto::RepeatedFieldCallback<int32>;
using SourceCallback = Field::SourceCallback;
using SinkCallback = Field::SinkCallback;

//! Source yielding a fixed list of values, then signalling end-of-stream (no dynamic allocation).
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

//! Sink recording received elements into a fixed buffer (no dynamic allocation).
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

template<std::size_t N>
static void expect_collected(const Collector<N>& collector, std::initializer_list<int32_t> expected)
{
  ASSERT_EQ(expected.size(), collector.count);
  std::size_t i = 0U;
  for(int32_t value : expected)
  {
    EXPECT_EQ(value, collector.values[i]);
    ++i;
  }
}

//! Reference EXPANDED byte stream: a resident field serialized one tag+value per element.
static void serialize_reference_expanded(std::initializer_list<int32_t> elements,
                                         ::EmbeddedProto::WriteBufferInterface& buffer)
{
  ::EmbeddedProto::RepeatedFieldFixedSize<int32, 8> resident;
  for(int32_t v : elements)
  {
    int32 value;
    value.set(v);
    (void)resident.add(value);
  }
  const uint32_t field_number = static_cast<uint32_t>(Msg::FieldNumber::VALUES);
  for(uint32_t i = 0U; i < resident.get_length(); ++i)
  {
    (void)resident.get_const(i).serialize_with_id(field_number, buffer, true);
  }
}

// The generated serialize() pulls from the source and must emit the same EXPANDED bytes a
// resident field would, proving the callbackStorage option routes to serialize_expanded().
TEST(CallbackStorageOption, serialize_out_matches_expanded_reference)
{
  ::EmbeddedProto::WriteBufferFixedSize<64> expected;
  serialize_reference_expanded({10, 20, 30}, expected);

  Producer<3> producer;
  producer.values = {10, 20, 30};
  producer.size = 3U;
  SourceCallback source;
  source.set(producer);

  Msg out;
  out.mutable_values().set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<64> actual;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(actual));

  ASSERT_EQ(expected.get_size(), actual.get_size());
  EXPECT_EQ(0, std::memcmp(expected.get_data(), actual.get_data(), expected.get_size()));
  EXPECT_EQ(3U, out.values().get_length());
}

// The deferred-from-Step-5 direction: serialize a source-backed message, then deserialize the
// bytes into a second sink-backed message. Both ends are the same generated type.
TEST(CallbackStorageOption, round_trip_source_to_sink)
{
  Producer<3> producer;
  producer.values = {10, 20, 30};
  producer.size = 3U;
  SourceCallback source;
  source.set(producer);

  Msg out;
  out.mutable_values().set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));

  Collector<8> collector;
  SinkCallback sink;
  sink.set(collector);

  Msg in;
  in.mutable_values().set_sink(sink);

  ::EmbeddedProto::ReadBufferFixedSize<64> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));

  expect_collected(collector, {10, 20, 30});
  EXPECT_EQ(3U, in.values().get_length());
}

// Without a bound source the field produces no elements, so the message serializes empty.
TEST(CallbackStorageOption, serialize_without_source_emits_nothing)
{
  Msg out; // no source bound

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
  EXPECT_EQ(0U, buffer.get_size());
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

// Partial serialize-out: a small write buffer forces resume, and each element is pulled once.
TEST(CallbackStorageOption, partial_serialize_out_across_split_round_trips)
{
  Producer<3> producer;
  producer.values = {10, 20, 30};
  producer.size = 3U;
  SourceCallback source;
  source.set(producer);

  Msg out;
  out.mutable_values().set_source(source);
  Msg::StateStack out_state;

  // A buffer just large enough for one element's worst-case size (int32 -> 6 bytes), so each
  // element forces a BUFFER_FULL resume boundary. Flush and clear between calls; the same state
  // resumes without double-pulling the source.
  ::EmbeddedProto::WriteBufferFixedSize<7> chunk;
  std::array<uint8_t, 64> accumulated{};
  std::size_t total = 0U;
  Error result = Error::BUFFER_FULL;
  uint32_t guard = 0U;
  while((Error::BUFFER_FULL == result) && (guard < 100U))
  {
    result = out.serialize_partial(chunk, out_state.root());
    ASSERT_TRUE((Error::NO_ERRORS == result) || (Error::BUFFER_FULL == result));
    ASSERT_GE(accumulated.size() - total, chunk.get_size());
    std::memcpy(accumulated.data() + total, chunk.get_data(), chunk.get_size());
    total += chunk.get_size();
    chunk.clear();
    ++guard;
  }
  ASSERT_EQ(Error::NO_ERRORS, result);

  // The bytes streamed across all resumes are EXPANDED, one tag+value per element, exactly as a
  // resident field emits them. A round-trip alone would not catch a regression to PACKED here,
  // as the deserializer accepts both encodings.
  ::EmbeddedProto::WriteBufferFixedSize<64> expected;
  serialize_reference_expanded({10, 20, 30}, expected);
  ASSERT_EQ(expected.get_size(), total);
  EXPECT_EQ(0, std::memcmp(expected.get_data(), accumulated.data(), total));

  ::EmbeddedProto::ReadBufferFixedSize<64> wire;
  for(std::size_t i = 0U; i < total; ++i)
  {
    wire.push(accumulated[i]);
  }

  Collector<8> collector;
  SinkCallback sink;
  sink.set(collector);

  Msg in;
  in.mutable_values().set_sink(sink);
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(wire));

  expect_collected(collector, {10, 20, 30});
  EXPECT_EQ(3U, in.values().get_length());
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_CallbackStorageOption
