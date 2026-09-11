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

#include <EmbeddedProto/Fields.h>
#include <EmbeddedProto/FieldStringBytes.h>
#include <EmbeddedProto/BytesStringCallback.h>
#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/WireFormatter.h>
#include <EmbeddedProto/MessageState.h>
#include <EmbeddedProto/Errors.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace test_EmbeddedAMS_BytesStringCallback
{

using ::EmbeddedProto::Error;
using ::EmbeddedProto::WireFormatter;
using Callback = ::EmbeddedProto::BytesStringCallback<uint8_t>;

// A callback field must be usable anywhere the customStorage static_assert expects
// an internal::BaseStringBytes.
static_assert(std::is_base_of<::EmbeddedProto::internal::BaseStringBytes, Callback>::value,
              "BytesStringCallback must derive from internal::BaseStringBytes.");

// --- Test doubles (fixed-size, no dynamic allocation, representative of MCU use) ---

//! Size callback reporting a fixed payload length (the LEN prefix written on serialize).
struct SizeReporter
{
  uint32_t value = 0U;
  uint32_t operator()() const { return value; }
};

//! Source yielding a fixed list of bytes, then signalling an early end-of-stream.
template<std::size_t N>
struct ByteProducer
{
  std::array<uint8_t, N> values{};
  std::size_t size = 0U;
  std::size_t index = 0U;
  bool operator()(uint8_t& element)
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

//! Sink recording every byte it receives, in order, into a fixed buffer.
template<std::size_t N>
struct ByteCollector
{
  std::array<uint8_t, N> values{};
  std::size_t count = 0U;
  Error operator()(const uint8_t& element)
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

//! Assert a ByteCollector received exactly the expected bytes, in order.
template<std::size_t N>
static void expect_collected(const ByteCollector<N>& collector,
                             std::initializer_list<uint8_t> expected)
{
  ASSERT_EQ(expected.size(), collector.count);
  std::size_t i = 0U;
  for(uint8_t value : expected)
  {
    EXPECT_EQ(value, collector.values[i]);
    ++i;
  }
}

//! Copy the contents of a write buffer into a read buffer, as a transport would.
template<class ReadBuffer, class WriteBuffer>
static void transfer(WriteBuffer& from, ReadBuffer& to)
{
  for(uint32_t i = 0U; i < from.get_size(); ++i)
  {
    to.push(from.get_data()[i]);
  }
}

// --- Binding / default state -----------------------------------------------

TEST(BytesStringCallback, default_state)
{
  Callback field;

  EXPECT_EQ(0U, field.get_length());
  EXPECT_FALSE(field.is_size_set());
  EXPECT_FALSE(field.is_source_set());
  EXPECT_FALSE(field.is_sink_set());
  EXPECT_GT(field.get_max_length(), field.get_length());
}

TEST(BytesStringCallback, bindings_set_and_clear)
{
  Callback field;

  SizeReporter size;
  ByteProducer<1> producer;
  ByteCollector<1> collector;
  Callback::SizeCallback size_cb;
  Callback::SourceCallback source;
  Callback::SinkCallback sink;
  size_cb.set(size);
  source.set(producer);
  sink.set(collector);

  field.set_size(size_cb);
  field.set_source(source);
  field.set_sink(sink);
  EXPECT_TRUE(field.is_size_set());
  EXPECT_TRUE(field.is_source_set());
  EXPECT_TRUE(field.is_sink_set());

  field.clear_size();
  field.clear_source();
  field.clear_sink();
  EXPECT_FALSE(field.is_size_set());
  EXPECT_FALSE(field.is_source_set());
  EXPECT_FALSE(field.is_sink_set());
}

TEST(BytesStringCallback, get_length_reports_the_size_callback)
{
  SizeReporter size;
  size.value = 5U;
  Callback::SizeCallback size_cb;
  size_cb.set(size);

  Callback field;
  field.set_size(size_cb);
  EXPECT_EQ(5U, field.get_length());
}

// --- Serialize (pull) -------------------------------------------------------

TEST(BytesStringCallback, serialize_payload_matches_resident_field)
{
  // Reference: a resident FieldBytes serializes only the raw payload (the caller
  // frames it). A callback field pulling the same bytes must emit the same payload.
  ::EmbeddedProto::FieldBytes<8> resident;
  const uint8_t data[] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
  (void)resident.set(data, 4U);
  ::EmbeddedProto::WriteBufferFixedSize<16> expected;
  ASSERT_EQ(Error::NO_ERRORS, resident.serialize(expected));

  SizeReporter size;
  size.value = 4U;
  ByteProducer<4> producer;
  producer.values = {0xDEU, 0xADU, 0xBEU, 0xEFU};
  producer.size = 4U;
  Callback::SizeCallback size_cb;
  Callback::SourceCallback source;
  size_cb.set(size);
  source.set(producer);

  Callback field;
  field.set_size(size_cb);
  field.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<16> actual;
  ASSERT_EQ(Error::NO_ERRORS, field.serialize(actual));

  ASSERT_EQ(expected.get_size(), actual.get_size());
  EXPECT_EQ(0, std::memcmp(expected.get_data(), actual.get_data(), expected.get_size()));
}

TEST(BytesStringCallback, full_frame_round_trips_through_sink)
{
  // Prove the whole LEN framing: serialize_len writes [tag][size][payload] using
  // get_length() (== size()) as the prefix, and the same bytes stream back through
  // the sink via deserialize_check_type, exactly as the message loop drives it.
  const uint32_t field_number = 3U;

  SizeReporter size;
  size.value = 5U;
  ByteProducer<5> producer;
  producer.values = {1U, 2U, 3U, 4U, 5U};
  producer.size = 5U;
  Callback::SizeCallback size_cb;
  Callback::SourceCallback source;
  size_cb.set(size);
  source.set(producer);

  Callback out;
  out.set_size(size_cb);
  out.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_len(field_number, out.get_length(), buffer, true));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  transfer(buffer, read_buffer);

  // The message loop reads the tag first, then dispatches on the wire type.
  uint32_t tag = 0U;
  ASSERT_EQ(Error::NO_ERRORS, WireFormatter::DeserializeVarint(read_buffer, tag));

  ByteCollector<8> collector;
  Callback::SinkCallback sink;
  sink.set(collector);

  Callback in;
  in.set_sink(sink);
  ASSERT_EQ(Error::NO_ERRORS,
            in.deserialize_check_type(read_buffer, WireFormatter::WireType::LENGTH_DELIMITED));

  expect_collected(collector, {1U, 2U, 3U, 4U, 5U});
}

TEST(BytesStringCallback, serialize_source_shorter_than_size_is_mismatch)
{
  SizeReporter size;
  size.value = 4U; // declare four, produce only two
  ByteProducer<4> producer;
  producer.values = {0xAAU, 0xBBU, 0U, 0U};
  producer.size = 2U;
  Callback::SizeCallback size_cb;
  Callback::SourceCallback source;
  size_cb.set(size);
  source.set(producer);

  Callback field;
  field.set_size(size_cb);
  field.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<16> buffer;
  EXPECT_EQ(Error::CALLBACK_SIZE_MISMATCH, field.serialize(buffer));
}

TEST(BytesStringCallback, serialize_size_without_source_errors)
{
  SizeReporter size;
  size.value = 3U;
  Callback::SizeCallback size_cb;
  size_cb.set(size);

  Callback field; // size declared, but no source bound to fulfil the prefix
  field.set_size(size_cb);

  ::EmbeddedProto::WriteBufferFixedSize<16> buffer;
  EXPECT_EQ(Error::CALLBACK_NOT_SET, field.serialize(buffer));
}

TEST(BytesStringCallback, serialize_empty_emits_nothing)
{
  Callback field; // no size, no source

  ::EmbeddedProto::WriteBufferFixedSize<16> buffer;
  EXPECT_EQ(Error::NO_ERRORS, field.serialize(buffer));
  EXPECT_EQ(0U, buffer.get_size());
}

// --- Deserialize (push) -----------------------------------------------------

TEST(BytesStringCallback, deserialize_without_sink_discards)
{
  Callback field; // no sink bound, not strict

  // [size=3][0x0A][0x0B][0x0C]
  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x03U, 0x0AU, 0x0BU, 0x0CU});
  EXPECT_EQ(Error::NO_ERRORS,
            field.deserialize_check_type(buffer, WireFormatter::WireType::LENGTH_DELIMITED));
  EXPECT_EQ(0U, buffer.get_size());
}

TEST(BytesStringCallback, deserialize_strict_without_sink_errors)
{
  Callback field;
  field.set_strict(true);

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x03U, 0x0AU, 0x0BU, 0x0CU});
  EXPECT_EQ(Error::CALLBACK_NOT_SET,
            field.deserialize_check_type(buffer, WireFormatter::WireType::LENGTH_DELIMITED));
}

TEST(BytesStringCallback, deserialize_rejects_wrong_wire_type)
{
  Callback field;

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer({0x03U, 0x0AU, 0x0BU, 0x0CU});
  EXPECT_EQ(Error::INVALID_WIRETYPE,
            field.deserialize_check_type(buffer, WireFormatter::WireType::VARINT));
}

// --- String (char) instantiation -------------------------------------------

TEST(BytesStringCallback, string_char_round_trips_through_sink)
{
  using StringCallback = ::EmbeddedProto::BytesStringCallback<char>;
  const uint32_t field_number = 1U;

  SizeReporter size;
  size.value = 3U;
  ByteProducer<3> producer; // bytes reinterpreted as chars on the wire
  producer.values = {static_cast<uint8_t>('a'), static_cast<uint8_t>('b'), static_cast<uint8_t>('c')};
  producer.size = 3U;

  // Adapt the byte producer to a char source.
  struct CharProducer
  {
    ByteProducer<3>* inner;
    bool operator()(char& element)
    {
      uint8_t byte = 0U;
      const bool produced = (*inner)(byte);
      element = static_cast<char>(byte);
      return produced;
    }
  } char_producer{&producer};

  StringCallback::SizeCallback size_cb;
  StringCallback::SourceCallback source;
  size_cb.set(size);
  source.set(char_producer);

  StringCallback out;
  out.set_size(size_cb);
  out.set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<32> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize_len(field_number, out.get_length(), buffer, true));

  ::EmbeddedProto::ReadBufferFixedSize<32> read_buffer;
  transfer(buffer, read_buffer);
  uint32_t tag = 0U;
  ASSERT_EQ(Error::NO_ERRORS, WireFormatter::DeserializeVarint(read_buffer, tag));

  std::array<char, 8> received{};
  std::size_t received_count = 0U;
  struct CharCollector
  {
    std::array<char, 8>* out;
    std::size_t* count;
    Error operator()(const char& element)
    {
      (*out)[*count] = element;
      ++(*count);
      return Error::NO_ERRORS;
    }
  } char_collector{&received, &received_count};

  StringCallback::SinkCallback sink;
  sink.set(char_collector);

  StringCallback in;
  in.set_sink(sink);
  ASSERT_EQ(Error::NO_ERRORS,
            in.deserialize_check_type(read_buffer, WireFormatter::WireType::LENGTH_DELIMITED));

  ASSERT_EQ(3U, received_count);
  EXPECT_EQ('a', received[0]);
  EXPECT_EQ('b', received[1]);
  EXPECT_EQ('c', received[2]);
}

// --- Partial (resumable) streaming -----------------------------------------

#ifdef PARTIAL_SERIALIZATION_ENABLED

using ::EmbeddedProto::FieldProcessingPhase;
using ::EmbeddedProto::MessageState;

TEST(BytesStringCallback, serialize_partial_resumes_without_double_pull)
{
  const uint32_t field_number = 3U;

  // Reference: a resident FieldBytes framed in one pass ([tag][size][payload]).
  ::EmbeddedProto::FieldBytes<8> resident;
  const uint8_t data[] = {1U, 2U, 3U, 4U, 5U};
  (void)resident.set(data, 5U);
  ::EmbeddedProto::WriteBufferFixedSize<16> expected;
  ASSERT_EQ(Error::NO_ERRORS, resident.serialize_len(field_number, resident.get_length(), expected, true));

  SizeReporter size;
  size.value = 5U;
  ByteProducer<5> producer;
  producer.values = {1U, 2U, 3U, 4U, 5U};
  producer.size = 5U;
  Callback::SizeCallback size_cb;
  Callback::SourceCallback source;
  size_cb.set(size);
  source.set(producer);

  Callback field;
  field.set_size(size_cb);
  field.set_source(source);

  // A small buffer forces the payload to split across several resumes.
  ::EmbeddedProto::WriteBufferFixedSize<4> buffer;
  MessageState state;

  std::array<uint8_t, 16> accumulated{};
  std::size_t total = 0U;
  uint32_t guard = 0U;
  while((FieldProcessingPhase::COMPLETE != state.phase) && (guard < 100U))
  {
    const Error result = field.serialize_partial_as_field(field_number, buffer, state, true);
    EXPECT_TRUE((Error::NO_ERRORS == result) || (Error::BUFFER_FULL == result));
    std::memcpy(accumulated.data() + total, buffer.get_data(), buffer.get_size());
    total += buffer.get_size();
    buffer.clear();
    ++guard;
  }

  ASSERT_EQ(expected.get_size(), total);
  EXPECT_EQ(0, std::memcmp(expected.get_data(), accumulated.data(), total));
}

TEST(BytesStringCallback, deserialize_partial_streams_across_split)
{
  ByteCollector<8> collector;
  Callback::SinkCallback sink;
  sink.set(collector);

  Callback field;
  field.set_sink(sink);

  MessageState state;
  state.phase = FieldProcessingPhase::SIZE;

  // [size=5][1][2] arrive first; the value straddles the refill boundary.
  ::EmbeddedProto::ReadBufferFixedSize<8> buffer1({0x05U, 0x01U, 0x02U});
  EXPECT_EQ(Error::END_OF_BUFFER, field.deserialize_partial_as_field(buffer1, state));
  expect_collected(collector, {1U, 2U});
  EXPECT_EQ(FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(3U, state.bytes_remaining);

  // The refill delivers the remaining three bytes; none are re-pushed.
  ::EmbeddedProto::ReadBufferFixedSize<8> buffer2({0x03U, 0x04U, 0x05U});
  EXPECT_EQ(Error::NO_ERRORS, field.deserialize_partial_as_field(buffer2, state));
  expect_collected(collector, {1U, 2U, 3U, 4U, 5U});
  EXPECT_EQ(FieldProcessingPhase::COMPLETE, state.phase);
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_BytesStringCallback
