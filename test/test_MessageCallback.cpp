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

//! Unit tests for MessageCallback: streaming a repeated message field as DELIMITED groups.
/*!
  MessageCallback owns no element array. On serialize it pulls each element from a
  bound source into a single transient sub-message and emits it as a START_GROUP /
  body / END_GROUP group; on deserialize every parsed group lands in the transient
  and is pushed to a bound sink. These tests drive the class directly (without the
  generator), simulating the parent message's per-group dispatch.
*/

#include "gtest/gtest.h"

#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>
#include <MessageCallback.h>
#include <MessageState.h>
#include <WireFormatter.h>
#include <Errors.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <callback_storage_message.h>

namespace test_EmbeddedAMS_MessageCallback
{

using ::EmbeddedProto::Error;

using Point = callback_storage_message::Point;
using Callback = ::EmbeddedProto::MessageCallback<Point>;

static constexpr uint32_t FIELD_NUM = 1U;

//! Build a Point from two coordinates.
static Point make_point(int32_t x, int32_t y)
{
  Point point;
  point.set_x(x);
  point.set_y(y);
  return point;
}

//! Source yielding a fixed list of Points, then signalling end-of-stream.
template<std::size_t N>
struct PointProducer
{
  std::array<Point, N> values{};
  std::size_t size = 0U;
  std::size_t index = 0U;
  bool operator()(Point& element)
  {
    const bool produced = index < size;
    if(produced)
    {
      element = values[index];
      ++index;
    }
    return produced;
  }
};

//! Sink recording received Points into a fixed buffer.
template<std::size_t N>
struct PointCollector
{
  std::array<Point, N> values{};
  std::size_t count = 0U;
  Error operator()(const Point& element)
  {
    Error return_value = Error::ARRAY_FULL;
    if(count < N)
    {
      values[count] = element;
      ++count;
      return_value = Error::NO_ERRORS;
    }
    return return_value;
  }
};

//! Copy every written byte into a read buffer.
template<class ReadBuffer, class WriteBuffer>
static void transfer(WriteBuffer& from, ReadBuffer& to)
{
  for(uint32_t i = 0U; i < from.get_size(); ++i)
  {
    to.push(from.get_data()[i]);
  }
}

//! Simulate the parent message's full-deserialize loop: for every START_GROUP tag, deserialize one element.
template<class ReadBuffer>
static Error drive_full_deserialize(Callback& callback, ReadBuffer& buffer)
{
  Error return_value = Error::NO_ERRORS;
  while((buffer.get_size() > 0U) && (Error::NO_ERRORS == return_value))
  {
    ::EmbeddedProto::WireFormatter::WireType wire_type;
    uint32_t id = 0U;
    return_value = ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id);
    if(Error::NO_ERRORS == return_value)
    {
      if((::EmbeddedProto::WireFormatter::WireType::START_GROUP == wire_type) && (FIELD_NUM == id))
      {
        return_value = callback.deserialize(buffer);
      }
      else
      {
        return_value = Error::INVALID_WIRETYPE;
      }
    }
  }
  return return_value;
}

TEST(MessageCallback, default_state)
{
  Callback callback;
  EXPECT_FALSE(callback.is_source_set());
  EXPECT_FALSE(callback.is_sink_set());
  EXPECT_FALSE(callback.is_strict());
  EXPECT_EQ(0U, callback.get_length());
}

TEST(MessageCallback, bindings_set_and_clear)
{
  PointProducer<1> producer;
  PointCollector<1> collector;
  Callback::SourceCallback source;
  Callback::SinkCallback sink;
  source.set(producer);
  sink.set(collector);

  Callback callback;
  callback.set_source(source);
  callback.set_sink(sink);
  EXPECT_TRUE(callback.is_source_set());
  EXPECT_TRUE(callback.is_sink_set());

  callback.set_strict(true);
  EXPECT_TRUE(callback.is_strict());

  callback.clear_source();
  callback.clear_sink();
  EXPECT_FALSE(callback.is_source_set());
  EXPECT_FALSE(callback.is_sink_set());
}

// The core round trip: a source-backed field emits one group per element and a
// sink-backed field streams the same elements back.
TEST(MessageCallback, full_round_trips_through_sink)
{
  PointProducer<3> producer;
  producer.values = {make_point(1, 2), make_point(3, 4), make_point(5, 6)};
  producer.size = 3U;
  Callback::SourceCallback source;
  source.set(producer);

  Callback out;
  out.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<128> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_expanded(FIELD_NUM, buffer));
  EXPECT_EQ(3U, out.get_length());

  PointCollector<8> collector;
  Callback::SinkCallback sink;
  sink.set(collector);

  Callback in;
  in.set_sink(sink);

  ::EmbeddedProto::ReadBufferFixedSize<128> read_buffer;
  transfer(buffer, read_buffer);
  ASSERT_EQ(Error::NO_ERRORS, drive_full_deserialize(in, read_buffer));

  ASSERT_EQ(3U, collector.count);
  EXPECT_EQ(1, collector.values[0].get_x());
  EXPECT_EQ(2, collector.values[0].get_y());
  EXPECT_EQ(3, collector.values[1].get_x());
  EXPECT_EQ(4, collector.values[1].get_y());
  EXPECT_EQ(5, collector.values[2].get_x());
  EXPECT_EQ(6, collector.values[2].get_y());
  EXPECT_EQ(3U, in.get_length());
}

// With no source bound the field reports get_length()==0 and emits nothing.
TEST(MessageCallback, serialize_without_source_emits_nothing)
{
  Callback out; // nothing bound

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_expanded(FIELD_NUM, buffer));
  EXPECT_EQ(0U, buffer.get_size());
  EXPECT_EQ(0U, out.get_length());
}

// Strict mode turns a missing source into an explicit error.
TEST(MessageCallback, serialize_strict_without_source_errors)
{
  Callback out;
  out.set_strict(true);

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  EXPECT_EQ(Error::CALLBACK_NOT_SET, out.serialize_expanded(FIELD_NUM, buffer));
}

// Without a sink the parsed elements are drained and discarded (not an error).
TEST(MessageCallback, deserialize_without_sink_discards)
{
  PointProducer<2> producer;
  producer.values = {make_point(11, 12), make_point(13, 14)};
  producer.size = 2U;
  Callback::SourceCallback source;
  source.set(producer);
  Callback out;
  out.set_source(source);
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_expanded(FIELD_NUM, buffer));

  Callback in; // no sink bound
  ::EmbeddedProto::ReadBufferFixedSize<64> read_buffer;
  transfer(buffer, read_buffer);
  ASSERT_EQ(Error::NO_ERRORS, drive_full_deserialize(in, read_buffer));
  EXPECT_EQ(2U, in.get_length());
}

// Strict mode turns a missing sink into an explicit error.
TEST(MessageCallback, deserialize_strict_without_sink_errors)
{
  PointProducer<1> producer;
  producer.values = {make_point(1, 1)};
  producer.size = 1U;
  Callback::SourceCallback source;
  source.set(producer);
  Callback out;
  out.set_source(source);
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_expanded(FIELD_NUM, buffer));

  Callback in;
  in.set_strict(true); // no sink bound
  ::EmbeddedProto::ReadBufferFixedSize<64> read_buffer;
  transfer(buffer, read_buffer);
  EXPECT_EQ(Error::CALLBACK_NOT_SET, drive_full_deserialize(in, read_buffer));
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

// Partial serialize-out: a small write buffer forces the groups to split, and each
// element is pulled once; the groups round-trip back through a sink-backed field.
TEST(MessageCallback, serialize_partial_resumes_across_split)
{
  PointProducer<3> producer;
  producer.values = {make_point(10, 20), make_point(30, 40), make_point(50, 60)};
  producer.size = 3U;
  Callback::SourceCallback source;
  source.set(producer);

  Callback out;
  out.set_source(source);
  ::EmbeddedProto::MessageStateStack<3> state;

  ::EmbeddedProto::WriteBufferFixedSize<4> chunk;
  ::EmbeddedProto::ReadBufferFixedSize<128> wire;
  Error result = Error::BUFFER_FULL;
  uint32_t guard = 0U;
  while((Error::BUFFER_FULL == result) && (guard < 200U))
  {
    result = out.serialize_partial_as_field_expanded(FIELD_NUM, chunk, state.root(), false);
    ASSERT_TRUE((Error::NO_ERRORS == result) || (Error::BUFFER_FULL == result));
    transfer(chunk, wire);
    chunk.clear();
    ++guard;
  }
  ASSERT_EQ(Error::NO_ERRORS, result);
  EXPECT_EQ(3U, out.get_length());

  PointCollector<8> collector;
  Callback::SinkCallback sink;
  sink.set(collector);
  Callback in;
  in.set_sink(sink);
  ASSERT_EQ(Error::NO_ERRORS, drive_full_deserialize(in, wire));

  ASSERT_EQ(3U, collector.count);
  EXPECT_EQ(10, collector.values[0].get_x());
  EXPECT_EQ(20, collector.values[0].get_y());
  EXPECT_EQ(30, collector.values[1].get_x());
  EXPECT_EQ(40, collector.values[1].get_y());
  EXPECT_EQ(50, collector.values[2].get_x());
  EXPECT_EQ(60, collector.values[2].get_y());
}

// Partial deserialize-in: one element's group body is fed one byte at a time and
// resumes across every split, pushing the element once complete.
TEST(MessageCallback, deserialize_partial_streams_across_split)
{
  PointProducer<1> producer;
  producer.values = {make_point(7, 9)};
  producer.size = 1U;
  Callback::SourceCallback source;
  source.set(producer);
  Callback out;
  out.set_source(source);
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_expanded(FIELD_NUM, buffer));

  // The parent consumes the opening START_GROUP tag before dispatching to the group.
  ::EmbeddedProto::ReadBufferFixedSize<64> full;
  transfer(buffer, full);
  ::EmbeddedProto::WireFormatter::WireType wire_type;
  uint32_t id = 0U;
  ASSERT_EQ(Error::NO_ERRORS, ::EmbeddedProto::WireFormatter::DeserializeTag(full, wire_type, id));
  ASSERT_EQ(::EmbeddedProto::WireFormatter::WireType::START_GROUP, wire_type);

  PointCollector<4> collector;
  Callback::SinkCallback sink;
  sink.set(collector);
  Callback in;
  in.set_sink(sink);

  ::EmbeddedProto::MessageStateStack<3> state;
  state.root().phase = ::EmbeddedProto::FieldProcessingPhase::DATA;

  ::EmbeddedProto::ReadBufferFixedSize<8> feed;
  Error result = Error::END_OF_BUFFER;
  uint32_t guard = 0U;
  while((full.get_size() > 0U)
        && (guard < 100U)
        && (::EmbeddedProto::FieldProcessingPhase::COMPLETE != state.root().phase))
  {
    uint8_t byte = 0U;
    full.pop(byte);
    feed.push(byte);
    result = in.deserialize_partial_as_group(feed, state.root());
    ASSERT_TRUE((Error::NO_ERRORS == result) || (Error::END_OF_BUFFER == result));
    ++guard;
  }

  ASSERT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.root().phase);
  ASSERT_EQ(1U, collector.count);
  EXPECT_EQ(7, collector.values[0].get_x());
  EXPECT_EQ(9, collector.values[0].get_y());
  EXPECT_EQ(1U, in.get_length());
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_MessageCallback
