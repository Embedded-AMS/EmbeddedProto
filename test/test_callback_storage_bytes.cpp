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

//! End-to-end proof of the dedicated callbackStorage generator option on bytes/string.
/*!
  A singular bytes field (blob) and a singular string field (label) are emitted by the
  generator as concrete BytesStringCallback types. The message serialize() writes the LEN
  prefix from size() and pulls the payload from a bound source; deserialize() streams each
  parsed element to a bound sink. A full source -> serialize -> deserialize -> sink round
  trip therefore runs through the generated message with nothing resident.
*/

#include "gtest/gtest.h"

#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/BytesStringCallback.h>
#include <EmbeddedProto/MessageState.h>
#include <EmbeddedProto/Errors.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <callback_storage_bytes.h>

namespace test_EmbeddedAMS_CallbackStorageBytes
{

using ::EmbeddedProto::Error;

using Msg = callback_storage_bytes::CallbackBlobMsg;
using BytesField = ::EmbeddedProto::BytesStringCallback<uint8_t>;
using StringField = ::EmbeddedProto::BytesStringCallback<char>;

//! Size callback reporting a fixed payload length.
struct SizeReporter
{
  uint32_t value = 0U;
  uint32_t operator()() const { return value; }
};

//! Source yielding a fixed list of elements, then signalling end-of-stream.
template<class T, std::size_t N>
struct Producer
{
  std::array<T, N> values{};
  std::size_t size = 0U;
  std::size_t index = 0U;
  bool operator()(T& element)
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

//! Sink recording received elements into a fixed buffer.
template<class T, std::size_t N>
struct Collector
{
  std::array<T, N> values{};
  std::size_t count = 0U;
  Error operator()(const T& element)
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

// A source-backed message serializes the declared payload and a sink-backed message streams
// it back; the same generated type drives both bytes and string fields.
TEST(CallbackStorageBytes, round_trip_source_to_sink)
{
  SizeReporter blob_size;
  blob_size.value = 4U;
  Producer<uint8_t, 4> blob_producer;
  blob_producer.values = {0xDEU, 0xADU, 0xBEU, 0xEFU};
  blob_producer.size = 4U;
  BytesField::SizeCallback blob_size_cb;
  BytesField::SourceCallback blob_source;
  blob_size_cb.set(blob_size);
  blob_source.set(blob_producer);

  SizeReporter label_size;
  label_size.value = 2U;
  Producer<char, 2> label_producer;
  label_producer.values = {'h', 'i'};
  label_producer.size = 2U;
  StringField::SizeCallback label_size_cb;
  StringField::SourceCallback label_source;
  label_size_cb.set(label_size);
  label_source.set(label_producer);

  Msg out;
  out.mutable_blob().set_size(blob_size_cb);
  out.mutable_blob().set_source(blob_source);
  out.mutable_label().set_size(label_size_cb);
  out.mutable_label().set_source(label_source);

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));

  Collector<uint8_t, 8> blob_collector;
  BytesField::SinkCallback blob_sink;
  blob_sink.set(blob_collector);
  Collector<char, 8> label_collector;
  StringField::SinkCallback label_sink;
  label_sink.set(label_collector);

  Msg in;
  in.mutable_blob().set_sink(blob_sink);
  in.mutable_label().set_sink(label_sink);

  ::EmbeddedProto::ReadBufferFixedSize<64> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));

  ASSERT_EQ(4U, blob_collector.count);
  EXPECT_EQ(0xDEU, blob_collector.values[0]);
  EXPECT_EQ(0xADU, blob_collector.values[1]);
  EXPECT_EQ(0xBEU, blob_collector.values[2]);
  EXPECT_EQ(0xEFU, blob_collector.values[3]);

  ASSERT_EQ(2U, label_collector.count);
  EXPECT_EQ('h', label_collector.values[0]);
  EXPECT_EQ('i', label_collector.values[1]);
}

// With no size/source bound both fields report get_length()==0, so the message serializes empty.
TEST(CallbackStorageBytes, serialize_without_binding_emits_nothing)
{
  Msg out; // nothing bound

  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
  EXPECT_EQ(0U, buffer.get_size());
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

// Partial serialize-out: a small write buffer forces the payload to split, and each element is
// pulled once; the bytes round-trip back through a sink-backed message.
TEST(CallbackStorageBytes, partial_serialize_out_across_split_round_trips)
{
  SizeReporter blob_size;
  blob_size.value = 5U;
  Producer<uint8_t, 5> blob_producer;
  blob_producer.values = {1U, 2U, 3U, 4U, 5U};
  blob_producer.size = 5U;
  BytesField::SizeCallback blob_size_cb;
  BytesField::SourceCallback blob_source;
  blob_size_cb.set(blob_size);
  blob_source.set(blob_producer);

  Msg out;
  out.mutable_blob().set_size(blob_size_cb);
  out.mutable_blob().set_source(blob_source);
  Msg::StateStack out_state;

  ::EmbeddedProto::WriteBufferFixedSize<4> chunk;
  ::EmbeddedProto::ReadBufferFixedSize<64> wire;
  Error result = Error::BUFFER_FULL;
  uint32_t guard = 0U;
  while((Error::BUFFER_FULL == result) && (guard < 100U))
  {
    result = out.serialize_partial(chunk, out_state.root());
    ASSERT_TRUE((Error::NO_ERRORS == result) || (Error::BUFFER_FULL == result));
    for(uint32_t i = 0U; i < chunk.get_size(); ++i)
    {
      wire.push(chunk.get_data()[i]);
    }
    chunk.clear();
    ++guard;
  }
  ASSERT_EQ(Error::NO_ERRORS, result);

  Collector<uint8_t, 8> blob_collector;
  BytesField::SinkCallback blob_sink;
  blob_sink.set(blob_collector);

  Msg in;
  in.mutable_blob().set_sink(blob_sink);
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(wire));

  ASSERT_EQ(5U, blob_collector.count);
  for(uint8_t i = 0U; i < 5U; ++i)
  {
    EXPECT_EQ(static_cast<uint8_t>(i + 1U), blob_collector.values[i]);
  }
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_CallbackStorageBytes
