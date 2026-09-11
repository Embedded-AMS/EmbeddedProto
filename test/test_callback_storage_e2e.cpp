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

//! End-to-end proof that RepeatedFieldCallback drives a real generated message via customStorage.
/*!
  Driven through the existing customStorage option, with no generator change. A message
  serialized with a resident storage type is deserialized into the same message parameterised
  with RepeatedFieldCallback, whose sink receives every element. Serialize-out via the message
  needs the generator to emit an EXPANDED pull; that arrives with the callbackStorage option.
*/

#include "gtest/gtest.h"

#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/RepeatedFieldFixedSize.h>
#include <EmbeddedProto/RepeatedFieldCallback.h>
#include <EmbeddedProto/MessageState.h>
#include <EmbeddedProto/Errors.h>

#include <array>
#include <cstddef>
#include <cstdint>

#include <callback_storage.h>

namespace test_EmbeddedAMS_CallbackStorageE2E
{

using ::EmbeddedProto::Error;
using ::EmbeddedProto::int32;

//! The element type the generator uses for the repeated Status field.
/*!
    A repeated enum field is stored as ::EmbeddedProto::enumeration, not as the bare enum, so the
    storage type handed to the message has to use the same element type. The serialized size is that
    of the largest enumerator, STATUS_ERROR, which equals two.
*/
using StatusField = ::EmbeddedProto::enumeration<callback_storage::Status,
                                                 ::EmbeddedProto::WireFormatter::VarintSize(2)>;

//! The same generated message, once with resident storage (to produce bytes) and once with callback storage (to consume them). Both satisfy the customStorage static_assert.
using ResidentMsg = callback_storage::CallbackStorageMsg<
    ::EmbeddedProto::RepeatedFieldFixedSize<int32, 8>,
    ::EmbeddedProto::RepeatedFieldFixedSize<StatusField, 8>>;
using CallbackMsg = callback_storage::CallbackStorageMsg<
    ::EmbeddedProto::RepeatedFieldCallback<int32>,
    ::EmbeddedProto::RepeatedFieldCallback<StatusField>>;
using IntSinkCallback = ::EmbeddedProto::RepeatedFieldCallback<int32>::SinkCallback;
using StatusSinkCallback = ::EmbeddedProto::RepeatedFieldCallback<StatusField>::SinkCallback;

//! Sink recording received elements into a fixed buffer (no dynamic allocation).
template<typename T, std::size_t N>
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

//! Compare the collected elements against the expected values.
/*!
    The collector holds field objects while the expected values are written as plain ints or
    enumerators, hence the separate type for the initializer list.
*/
template<typename T, std::size_t N, typename EXPECTED_TYPE>
static void expect_collected(const Collector<T, N>& collector,
                             std::initializer_list<EXPECTED_TYPE> expected)
{
  ASSERT_EQ(expected.size(), collector.count);
  std::size_t i = 0U;
  for(const EXPECTED_TYPE& value : expected)
  {
    EXPECT_EQ(value, collector.values[i]);
    ++i;
  }
}

//! Serialize int32 values {10, 20, 30} through a resident-storage instance of the message.
static void serialize_int_reference(::EmbeddedProto::WriteBufferInterface& buffer)
{
  ResidentMsg out;
  (void)out.mutable_values().add(10);
  (void)out.mutable_values().add(20);
  (void)out.mutable_values().add(30);
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
}

//! Serialize Status enum values {STATUS_OFF, STATUS_ON, STATUS_ERROR} through a resident-storage instance of the message.
static void serialize_enum_reference(::EmbeddedProto::WriteBufferInterface& buffer)
{
  ResidentMsg out;
  (void)out.mutable_statuses().add(callback_storage::Status::STATUS_OFF);
  (void)out.mutable_statuses().add(callback_storage::Status::STATUS_ON);
  (void)out.mutable_statuses().add(callback_storage::Status::STATUS_ERROR);
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
}

TEST(CallbackStorageE2E, deserialize_int_streams_to_sink)
{
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  serialize_int_reference(buffer);

  Collector<int32, 8> collector;
  IntSinkCallback sink;
  sink.set(collector);

  CallbackMsg in;
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

TEST(CallbackStorageE2E, deserialize_int_without_sink_discards)
{
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  serialize_int_reference(buffer);

  CallbackMsg in; // no sink bound

  ::EmbeddedProto::ReadBufferFixedSize<64> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));
  // Elements were drained and counted, but stored nowhere.
  EXPECT_EQ(3U, in.values().get_length());
}

TEST(CallbackStorageE2E, deserialize_enum_streams_to_sink)
{
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  serialize_enum_reference(buffer);

  Collector<StatusField, 8> collector;
  StatusSinkCallback sink;
  sink.set(collector);

  CallbackMsg in;
  in.mutable_statuses().set_sink(sink);

  ::EmbeddedProto::ReadBufferFixedSize<64> read_buffer;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    read_buffer.push(buffer.get_data()[i]);
  }

  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));

  expect_collected(collector, {
      callback_storage::Status::STATUS_OFF,
      callback_storage::Status::STATUS_ON,
      callback_storage::Status::STATUS_ERROR});
  EXPECT_EQ(3U, in.statuses().get_length());
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

TEST(CallbackStorageE2E, partial_deserialize_int_streams_to_sink_across_split)
{
  ::EmbeddedProto::WriteBufferFixedSize<64> buffer;
  serialize_int_reference(buffer);
  const uint32_t total = buffer.get_size();
  ASSERT_LT(3U, total); // need at least a few bytes to split

  Collector<int32, 8> collector;
  IntSinkCallback sink;
  sink.set(collector);

  CallbackMsg in;
  in.mutable_values().set_sink(sink);
  CallbackMsg::StateStack state;

  // Feed the message in two halves so the packed block straddles the refill.
  const uint32_t split = total / 2U;

  ::EmbeddedProto::ReadBufferFixedSize<64> buffer1;
  for(uint32_t i = 0U; i < split; ++i)
  {
    buffer1.push(buffer.get_data()[i]);
  }
  Error result = in.deserialize_partial(buffer1, state.root());
  EXPECT_EQ(Error::END_OF_BUFFER, result);

  ::EmbeddedProto::ReadBufferFixedSize<64> buffer2;
  for(uint32_t i = split; i < total; ++i)
  {
    buffer2.push(buffer.get_data()[i]);
  }
  // Partial deserialize reports END_OF_BUFFER once the input is drained (the engine ran out
  // of bytes looking for the next field tag); the message content is complete at that point.
  result = in.deserialize_partial(buffer2, state.root());
  EXPECT_EQ(Error::END_OF_BUFFER, result);

  // Every element streamed to the sink exactly once across the split.
  expect_collected(collector, {10, 20, 30});
  EXPECT_EQ(3U, in.values().get_length());
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // namespace test_EmbeddedAMS_CallbackStorageE2E
