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

//! End-to-end proof of the callbackStorage generator option on a repeated message field.
/*!
  A repeated message field (points) marked callbackStorage and DELIMITED-encoded is
  emitted by the generator as a concrete MessageCallback type. The message
  serialize() pulls each element from a bound source and writes it as a group;
  deserialize() streams each parsed group to a bound sink. A full source ->
  serialize -> deserialize -> sink round trip therefore runs through the generated
  message with nothing resident.
*/

#include "gtest/gtest.h"

#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/MessageCallback.h>
#include <EmbeddedProto/MessageState.h>
#include <EmbeddedProto/Errors.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include <callback_storage_message.h>

namespace test_EmbeddedAMS_CallbackStorageMessage
{

using ::EmbeddedProto::Error;

using Msg = callback_storage_message::PointStreamMsg;
using Point = callback_storage_message::Point;
using PointField = ::EmbeddedProto::MessageCallback<Point>;

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

// A source-backed message serializes the streamed elements and a sink-backed message
// streams them back through the same generated type.
TEST(CallbackStorageMessage, round_trip_source_to_sink)
{
  PointProducer<3> producer;
  producer.values = {make_point(1, 2), make_point(3, 4), make_point(5, 6)};
  producer.size = 3U;
  PointField::SourceCallback source;
  source.set(producer);

  Msg out;
  out.mutable_points().set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<128> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
  EXPECT_EQ(3U, out.get_points().get_length());

  PointCollector<8> collector;
  PointField::SinkCallback sink;
  sink.set(collector);

  Msg in;
  in.mutable_points().set_sink(sink);

  ::EmbeddedProto::ReadBufferFixedSize<128> read_buffer;
  transfer(buffer, read_buffer);
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));

  ASSERT_EQ(3U, collector.count);
  EXPECT_EQ(1, collector.values[0].get_x());
  EXPECT_EQ(2, collector.values[0].get_y());
  EXPECT_EQ(3, collector.values[1].get_x());
  EXPECT_EQ(4, collector.values[1].get_y());
  EXPECT_EQ(5, collector.values[2].get_x());
  EXPECT_EQ(6, collector.values[2].get_y());
}

// With no source bound the field streams nothing, so the message serializes empty.
TEST(CallbackStorageMessage, serialize_without_binding_emits_nothing)
{
  Msg out; // nothing bound

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
  EXPECT_EQ(0U, buffer.get_size());
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

//! Reference DELIMITED byte stream: every Point serialized as its own group.
/*!
    This is the encoding the field must produce, a start-group tag, the element body and an
    end-group tag per element, with no length prefix anywhere. Building it from resident Points
    keeps the expectation independent of the MessageCallback code under test.
*/
static void serialize_reference_groups(std::initializer_list<Point> elements,
                                       ::EmbeddedProto::WriteBufferInterface& buffer)
{
  const uint32_t field_number = static_cast<uint32_t>(Msg::FieldNumber::POINTS);
  for(const Point& point : elements)
  {
    (void)point.serialize_as_group(field_number, buffer);
  }
}

// Partial serialize-out: a small write buffer forces the groups to split, and each
// element is pulled once; the elements round-trip back through a sink-backed message.
TEST(CallbackStorageMessage, partial_serialize_out_across_split_round_trips)
{
  PointProducer<4> producer;
  producer.values = {make_point(10, 20), make_point(30, 40),
                     make_point(50, 60), make_point(70, 80)};
  producer.size = 4U;
  PointField::SourceCallback source;
  source.set(producer);

  Msg out;
  out.mutable_points().set_source(source);
  Msg::StateStack out_state;

  ::EmbeddedProto::WriteBufferFixedSize<4> chunk;
  std::array<uint8_t, 128> accumulated{};
  std::size_t total = 0U;
  Error result = Error::BUFFER_FULL;
  uint32_t guard = 0U;
  while((Error::BUFFER_FULL == result) && (guard < 200U))
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

  // The bytes streamed across all resumes are group framed, one start-group tag, body and
  // end-group tag per element. A round-trip alone would not catch a regression to a length
  // prefixed encoding here, as the deserializer accepts both.
  ::EmbeddedProto::WriteBufferFixedSize<128> expected;
  serialize_reference_groups({make_point(10, 20), make_point(30, 40),
                              make_point(50, 60), make_point(70, 80)}, expected);
  ASSERT_EQ(expected.get_size(), total);
  EXPECT_EQ(0, std::memcmp(expected.get_data(), accumulated.data(), total));

  ::EmbeddedProto::ReadBufferFixedSize<128> wire;
  for(std::size_t i = 0U; i < total; ++i)
  {
    wire.push(accumulated[i]);
  }

  PointCollector<8> collector;
  PointField::SinkCallback sink;
  sink.set(collector);

  Msg in;
  in.mutable_points().set_sink(sink);
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(wire));

  ASSERT_EQ(4U, collector.count);
  EXPECT_EQ(10, collector.values[0].get_x());
  EXPECT_EQ(20, collector.values[0].get_y());
  EXPECT_EQ(30, collector.values[1].get_x());
  EXPECT_EQ(40, collector.values[1].get_y());
  EXPECT_EQ(50, collector.values[2].get_x());
  EXPECT_EQ(60, collector.values[2].get_y());
  EXPECT_EQ(70, collector.values[3].get_x());
  EXPECT_EQ(80, collector.values[3].get_y());
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_CallbackStorageMessage
