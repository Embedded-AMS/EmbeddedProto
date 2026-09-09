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


/*
  Map fields. A map is stored and encoded as a repeated field of entry messages, so the wire format
  and the (de)serialization come from the repeated message path. What is tested here is the map
  layer on top of it: key lookup, insert and replace, removal, the capacity limit, last value wins
  for a key a peer sent twice, and the streaming (callbackStorage) variant.

  The expected byte streams are those of a standard protoc peer; they are written out literally so a
  change in framing is caught here.
*/

#include "gtest/gtest.h"

#include <ReadBufferFixedSize.h>
#include <WriteBufferFixedSize.h>
#include <MessageCallback.h>
#include <MessageState.h>
#include <Errors.h>

#include <array>
#include <cstdint>
#include <cstring>

#include <map_fields.h>

using ::EmbeddedProto::Error;

namespace test_EmbeddedAMS_map_fields
{

// Copy everything written into a read buffer, to deserialize what was just serialized.
static void transfer(::EmbeddedProto::WriteBufferFixedSize<256>& from,
                     ::EmbeddedProto::ReadBufferFixedSize<256>& to)
{
  for(uint32_t i = 0U; i < from.get_size(); ++i)
  {
    to.push(from.get_data()[i]);
  }
}

// Push a literal byte stream, as a protoc peer would have sent it, into a read buffer.
template<std::size_t N>
static void push(const std::array<uint8_t, N>& bytes, ::EmbeddedProto::ReadBufferFixedSize<256>& to)
{
  for(const uint8_t byte : bytes)
  {
    to.push(byte);
  }
}

// ---------------------------------------------------------------------------------------------
// The map API
// ---------------------------------------------------------------------------------------------

TEST(MapFields, set_get_and_replace)
{
  map_fields::MapTypes msg;

  EXPECT_EQ(0U, msg.by_name_size());
  EXPECT_EQ(4U, msg.by_name_max_size());
  EXPECT_FALSE(msg.has_by_name("ann"));

  EXPECT_EQ(Error::NO_ERRORS, msg.set_by_name("ann", 1));
  EXPECT_EQ(Error::NO_ERRORS, msg.set_by_name("bob", 2));
  EXPECT_EQ(2U, msg.by_name_size());
  EXPECT_TRUE(msg.has_by_name("ann"));
  EXPECT_EQ(1, msg.get_by_name("ann"));
  EXPECT_EQ(2, msg.get_by_name("bob"));

  // Setting an existing key replaces the value, it does not add an entry.
  EXPECT_EQ(Error::NO_ERRORS, msg.set_by_name("ann", 42));
  EXPECT_EQ(2U, msg.by_name_size());
  EXPECT_EQ(42, msg.get_by_name("ann"));
}

TEST(MapFields, absent_key_yields_the_default)
{
  map_fields::MapTypes msg;
  msg.set_by_name("ann", 1);

  EXPECT_FALSE(msg.has_by_name("nobody"));
  EXPECT_EQ(0, msg.get_by_name("nobody"));

  int32_t value = 99;
  EXPECT_EQ(Error::INDEX_OUT_OF_BOUND, msg.get_by_name("nobody", value));
  // The out parameter is left untouched when the key is absent.
  EXPECT_EQ(99, value);

  EXPECT_EQ(Error::NO_ERRORS, msg.get_by_name("ann", value));
  EXPECT_EQ(1, value);
}

TEST(MapFields, remove_and_clear)
{
  map_fields::MapTypes msg;
  msg.set_by_name("ann", 1);
  msg.set_by_name("bob", 2);
  msg.set_by_name("cy", 3);

  EXPECT_EQ(Error::NO_ERRORS, msg.remove_by_name("bob"));
  EXPECT_EQ(2U, msg.by_name_size());
  EXPECT_FALSE(msg.has_by_name("bob"));
  // The entries around the removed one survive.
  EXPECT_EQ(1, msg.get_by_name("ann"));
  EXPECT_EQ(3, msg.get_by_name("cy"));

  EXPECT_EQ(Error::INDEX_OUT_OF_BOUND, msg.remove_by_name("bob"));

  msg.clear_by_name();
  EXPECT_EQ(0U, msg.by_name_size());
}

TEST(MapFields, full_map_reports_array_full)
{
  map_fields::SmallMap msg;
  EXPECT_EQ(Error::NO_ERRORS, msg.set_entries("one", 1));
  EXPECT_EQ(Error::NO_ERRORS, msg.set_entries("two", 2));
  EXPECT_EQ(Error::ARRAY_FULL, msg.set_entries("three", 3));

  // Replacing a value of a key already present still works on a full map.
  EXPECT_EQ(Error::NO_ERRORS, msg.set_entries("one", 11));
  EXPECT_EQ(11, msg.get_entries("one"));
  EXPECT_EQ(2U, msg.entries_size());
}

TEST(MapFields, a_key_or_value_longer_than_its_maximum_is_refused)
{
  // by_name has keyMaxLength 8, by_id has valueMaxLength 8.
  map_fields::MapTypes msg;
  msg.set_by_name("ann", 1);
  msg.set_by_id(7, "seven");

  // A key that does not fit is refused, nothing is stored, and it can not be found afterwards.
  EXPECT_EQ(Error::ARRAY_FULL, msg.set_by_name("a-key-that-is-far-too-long", 2));
  EXPECT_EQ(1U, msg.by_name_size());
  EXPECT_FALSE(msg.has_by_name("a-key-that-is-far-too-long"));

  // A value that does not fit is refused as well, and the value already stored is left alone.
  EXPECT_EQ(Error::ARRAY_FULL, msg.set_by_id(7, "a-value-that-is-far-too-long"));
  EXPECT_STREQ("seven", msg.get_by_id(7));
  EXPECT_EQ(Error::ARRAY_FULL, msg.set_by_id(8, "a-value-that-is-far-too-long"));
  EXPECT_EQ(1U, msg.by_id_size());

  // Exactly the maximum still fits.
  EXPECT_EQ(Error::NO_ERRORS, msg.set_by_name("12345678", 3));
  EXPECT_EQ(3, msg.get_by_name("12345678"));
}

TEST(MapFields, iterate_over_the_entries)
{
  map_fields::MapTypes msg;
  msg.set_by_name("ann", 1);
  msg.set_by_name("bob", 2);

  int32_t sum = 0;
  for(uint32_t i = 0U; i < msg.by_name_size(); ++i)
  {
    sum += msg.by_name(i).get_value();
  }
  EXPECT_EQ(3, sum);

  // An entry may also be changed in place while iterating.
  msg.mutable_by_name(0).set_value(10);
  EXPECT_EQ(10, msg.get_by_name("ann"));
}

TEST(MapFields, every_value_kind_is_accessible)
{
  map_fields::MapTypes msg;

  EXPECT_EQ(Error::NO_ERRORS, msg.set_by_id(7, "seven"));
  EXPECT_STREQ("seven", msg.get_by_id(7));
  EXPECT_STREQ("", msg.get_by_id(8));

  EXPECT_EQ(Error::NO_ERRORS, msg.set_text("k", "v"));
  EXPECT_STREQ("v", msg.get_text("k"));

  EXPECT_EQ(Error::NO_ERRORS, msg.set_colours(3U, map_fields::Colour::BLUE));
  EXPECT_EQ(map_fields::Colour::BLUE, msg.get_colours(3U));
  EXPECT_EQ(map_fields::Colour::RED, msg.get_colours(4U));

  map_fields::Point point;
  point.set_x(10);
  point.set_y(20);
  EXPECT_EQ(Error::NO_ERRORS, msg.set_points(99, point));
  map_fields::Point read_back;
  EXPECT_EQ(Error::NO_ERRORS, msg.get_points(99, read_back));
  EXPECT_EQ(10, read_back.get_x());
  EXPECT_EQ(20, read_back.get_y());
  EXPECT_EQ(Error::INDEX_OUT_OF_BOUND, msg.get_points(100, read_back));

  ::EmbeddedProto::FieldBytes<4> blob;
  const std::array<uint8_t, 3> raw = {1U, 2U, 3U};
  blob.set(raw.data(), raw.size());
  EXPECT_EQ(Error::NO_ERRORS, msg.set_flags(true, blob));
  ::EmbeddedProto::FieldBytes<4> blob_read;
  EXPECT_EQ(Error::NO_ERRORS, msg.get_flags(true, blob_read));
  EXPECT_EQ(3U, blob_read.get_length());
  EXPECT_EQ(3U, blob_read.get_const(2));
}

// ---------------------------------------------------------------------------------------------
// The wire format
// ---------------------------------------------------------------------------------------------

TEST(MapFields, serialized_bytes_match_a_protoc_peer)
{
  map_fields::MapTypes msg;
  msg.set_by_name("ann", 1);
  msg.set_by_name("bob", 2);
  msg.set_by_id(7, "seven");
  msg.set_text("k", "v");
  msg.set_colours(3U, map_fields::Colour::BLUE);
  map_fields::Point point;
  point.set_x(10);
  point.set_y(20);
  msg.set_points(99, point);
  ::EmbeddedProto::FieldBytes<4> blob;
  const std::array<uint8_t, 3> raw = {1U, 2U, 3U};
  blob.set(raw.data(), raw.size());
  msg.set_flags(true, blob);
  msg.set_other(5);

  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, msg.serialize(buffer));

  // Produced by the python protobuf runtime for the same content. Each entry is a length delimited
  // block holding key = 1 and value = 2.
  const std::array<uint8_t, 64> expected = {
    0x0a, 0x07, 0x0a, 0x03, 0x61, 0x6e, 0x6e, 0x10, 0x01,
    0x0a, 0x07, 0x0a, 0x03, 0x62, 0x6f, 0x62, 0x10, 0x02,
    0x12, 0x09, 0x08, 0x07, 0x12, 0x05, 0x73, 0x65, 0x76, 0x65, 0x6e,
    0x1a, 0x06, 0x0a, 0x01, 0x6b, 0x12, 0x01, 0x76,
    0x22, 0x04, 0x08, 0x03, 0x10, 0x02,
    0x2a, 0x08, 0x08, 0x63, 0x12, 0x04, 0x08, 0x0a, 0x10, 0x14,
    0x32, 0x07, 0x08, 0x01, 0x12, 0x03, 0x01, 0x02, 0x03,
    0x38, 0x05
  };

  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0U; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "at byte " << i;
  }
}

TEST(MapFields, round_trip_every_value_kind)
{
  map_fields::MapTypes out;
  out.set_by_name("ann", 1);
  out.set_by_id(7, "seven");
  out.set_text("k", "v");
  out.set_colours(3U, map_fields::Colour::BLUE);
  map_fields::Point point;
  point.set_x(10);
  point.set_y(20);
  out.set_points(99, point);
  out.set_other(5);

  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));

  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  transfer(buffer, read_buffer);

  map_fields::MapTypes in;
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));

  EXPECT_EQ(1, in.get_by_name("ann"));
  EXPECT_STREQ("seven", in.get_by_id(7));
  EXPECT_STREQ("v", in.get_text("k"));
  EXPECT_EQ(map_fields::Colour::BLUE, in.get_colours(3U));
  map_fields::Point read_back;
  EXPECT_EQ(Error::NO_ERRORS, in.get_points(99, read_back));
  EXPECT_EQ(10, read_back.get_x());
  EXPECT_EQ(20, read_back.get_y());
  // A field following the maps proves the parent kept parsing after them.
  EXPECT_EQ(5, in.get_other());
}

TEST(MapFields, an_empty_map_emits_nothing)
{
  map_fields::SmallMap empty;
  ::EmbeddedProto::WriteBufferFixedSize<256> empty_buffer;
  ASSERT_EQ(Error::NO_ERRORS, empty.serialize(empty_buffer));
  EXPECT_EQ(0U, empty_buffer.get_size());

  // With another field set the message is not empty, so the round trip shows the maps themselves
  // contributed nothing and come back empty.
  map_fields::MapTypes out;
  out.set_other(5);
  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));
  EXPECT_EQ(2U, buffer.get_size());

  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  transfer(buffer, read_buffer);
  map_fields::MapTypes in;
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));
  EXPECT_EQ(0U, in.by_name_size());
  EXPECT_EQ(0U, in.points_size());
  EXPECT_EQ(5, in.get_other());
}

TEST(MapFields, duplicate_key_on_the_wire_takes_the_last_value)
{
  // Two entries with the key "one", values 1 and 2. Protobuf prescribes that the last one wins.
  const std::array<uint8_t, 18> bytes = {
    0x0a, 0x07, 0x0a, 0x03, 0x6f, 0x6e, 0x65, 0x10, 0x01,
    0x0a, 0x07, 0x0a, 0x03, 0x6f, 0x6e, 0x65, 0x10, 0x02
  };
  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  push(bytes, read_buffer);

  map_fields::SmallMap msg;
  ASSERT_EQ(Error::NO_ERRORS, msg.deserialize(read_buffer));
  EXPECT_EQ(2, msg.get_entries("one"));

  // Removing the key drops every entry carrying it.
  EXPECT_EQ(Error::NO_ERRORS, msg.remove_entries("one"));
  EXPECT_EQ(0U, msg.entries_size());
}

TEST(MapFields, an_entry_without_key_or_value_reads_as_the_defaults)
{
  // One entry holding only a value, and one holding only a key.
  const std::array<uint8_t, 11> bytes = {
    0x0a, 0x02, 0x10, 0x07,
    0x0a, 0x05, 0x0a, 0x03, 0x6f, 0x6e, 0x65
  };
  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  push(bytes, read_buffer);

  map_fields::SmallMap msg;
  ASSERT_EQ(Error::NO_ERRORS, msg.deserialize(read_buffer));
  ASSERT_EQ(2U, msg.entries_size());
  // The entry without a key carries the empty string as its key.
  EXPECT_EQ(7, msg.get_entries(""));
  // The entry without a value carries the default value.
  EXPECT_EQ(0, msg.get_entries("one"));
}

TEST(MapFields, an_entry_with_an_unknown_field_is_still_parsed)
{
  // key = "one", value = 5 and an unknown field 3 holding a varint.
  const std::array<uint8_t, 13> bytes = {
    0x0a, 0x0b, 0x0a, 0x03, 0x6f, 0x6e, 0x65, 0x10, 0x05, 0x18, 0x63, 0x00, 0x00
  };
  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  for(uint32_t i = 0U; i < 11U; ++i)
  {
    read_buffer.push(bytes[i]);
  }

  map_fields::SmallMap msg;
  ASSERT_EQ(Error::NO_ERRORS, msg.deserialize(read_buffer));
  EXPECT_EQ(5, msg.get_entries("one"));
}

TEST(MapFields, deserializing_more_entries_than_fit_reports_array_full)
{
  // Three entries into a map sized for two.
  const std::array<uint8_t, 27> bytes = {
    0x0a, 0x07, 0x0a, 0x03, 0x6f, 0x6e, 0x65, 0x10, 0x01,
    0x0a, 0x07, 0x0a, 0x03, 0x74, 0x77, 0x6f, 0x10, 0x02,
    0x0a, 0x07, 0x0a, 0x03, 0x73, 0x69, 0x78, 0x10, 0x03
  };
  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  push(bytes, read_buffer);

  map_fields::SmallMap msg;
  EXPECT_EQ(Error::ARRAY_FULL, msg.deserialize(read_buffer));
}

// ---------------------------------------------------------------------------------------------
// Streaming (callbackStorage) maps
// ---------------------------------------------------------------------------------------------

using StreamEntry = map_fields::StreamMap::EntriesEntry;
using StreamField = ::EmbeddedProto::MessageCallback<StreamEntry>;

// Hands out a fixed set of entries, one per call, the way a device would stream a table it holds
// somewhere else.
class EntryProducer
{
  public:
    uint32_t index = 0U;
    uint32_t size = 0U;
    std::array<const char*, 4> keys = {};
    std::array<int32_t, 4> values = {};

    bool operator()(StreamEntry& entry)
    {
      const bool more = index < size;
      if(more)
      {
        entry.mutable_key().set(keys[index]);
        entry.set_value(values[index]);
        ++index;
      }
      return more;
    }
};

// Collects every entry pushed out of a deserialize.
class EntryCollector
{
  public:
    uint32_t count = 0U;
    std::array<::EmbeddedProto::FieldString<8>, 4> keys = {};
    std::array<int32_t, 4> values = {};

    Error operator()(const StreamEntry& entry)
    {
      Error result = Error::ARRAY_FULL;
      if(count < keys.size())
      {
        keys[count] = entry.get_key();
        values[count] = entry.get_value();
        ++count;
        result = Error::NO_ERRORS;
      }
      return result;
    }
};

TEST(MapFields, a_streaming_map_writes_the_same_bytes_as_a_resident_one)
{
  EntryProducer producer;
  producer.keys = {"ann", "bob", nullptr, nullptr};
  producer.values = {1, 2, 0, 0};
  producer.size = 2U;
  StreamField::SourceCallback source;
  source.set(producer);

  map_fields::StreamMap msg;
  msg.mutable_entries().set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, msg.serialize(buffer));

  // Identical to what the resident map above produced for the same two entries.
  const std::array<uint8_t, 18> expected = {
    0x0a, 0x07, 0x0a, 0x03, 0x61, 0x6e, 0x6e, 0x10, 0x01,
    0x0a, 0x07, 0x0a, 0x03, 0x62, 0x6f, 0x62, 0x10, 0x02
  };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0U; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]) << "at byte " << i;
  }
}

TEST(MapFields, a_streaming_map_round_trips_source_to_sink)
{
  EntryProducer producer;
  producer.keys = {"ann", "bob", "cy", nullptr};
  producer.values = {1, 2, 3, 0};
  producer.size = 3U;
  StreamField::SourceCallback source;
  source.set(producer);

  map_fields::StreamMap out;
  out.mutable_entries().set_source(source);

  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));

  EntryCollector collector;
  StreamField::SinkCallback sink;
  sink.set(collector);

  map_fields::StreamMap in;
  in.mutable_entries().set_sink(sink);

  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  transfer(buffer, read_buffer);
  ASSERT_EQ(Error::NO_ERRORS, in.deserialize(read_buffer));

  ASSERT_EQ(3U, collector.count);
  EXPECT_TRUE(collector.keys[0] == "ann");
  EXPECT_TRUE(collector.keys[1] == "bob");
  EXPECT_TRUE(collector.keys[2] == "cy");
  EXPECT_EQ(1, collector.values[0]);
  EXPECT_EQ(2, collector.values[1]);
  EXPECT_EQ(3, collector.values[2]);
}

TEST(MapFields, a_streaming_map_without_a_source_emits_nothing)
{
  map_fields::StreamMap msg;
  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, msg.serialize(buffer));
  EXPECT_EQ(0U, buffer.get_size());
}

TEST(MapFields, a_streaming_map_reads_what_a_protoc_peer_sent)
{
  // The literal bytes of a resident map, proving a stream consumes a standard peer's output.
  const std::array<uint8_t, 18> bytes = {
    0x0a, 0x07, 0x0a, 0x03, 0x61, 0x6e, 0x6e, 0x10, 0x01,
    0x0a, 0x07, 0x0a, 0x03, 0x62, 0x6f, 0x62, 0x10, 0x02
  };
  ::EmbeddedProto::ReadBufferFixedSize<256> read_buffer;
  push(bytes, read_buffer);

  EntryCollector collector;
  StreamField::SinkCallback sink;
  sink.set(collector);

  map_fields::StreamMap msg;
  msg.mutable_entries().set_sink(sink);
  ASSERT_EQ(Error::NO_ERRORS, msg.deserialize(read_buffer));

  ASSERT_EQ(2U, collector.count);
  EXPECT_TRUE(collector.keys[0] == "ann");
  EXPECT_EQ(2, collector.values[1]);
}

// ---------------------------------------------------------------------------------------------
// Partial (resumable) (de)serialization
// ---------------------------------------------------------------------------------------------

#ifdef PARTIAL_SERIALIZATION_ENABLED

// Serialize a message in chunks of CHUNK_SIZE bytes, so every buffer boundary falls somewhere else
// in the stream, and hand back everything written.
template<uint32_t CHUNK_SIZE, class MSG, class STATE>
static std::size_t serialize_in_chunks(MSG& msg, STATE& state, std::array<uint8_t, 256>& out)
{
  ::EmbeddedProto::WriteBufferFixedSize<CHUNK_SIZE> chunk;
  std::size_t total = 0U;
  Error result = Error::BUFFER_FULL;
  uint32_t guard = 0U;
  while((Error::BUFFER_FULL == result) && (guard < 400U))
  {
    result = msg.serialize_partial(chunk, state.root());
    EXPECT_TRUE((Error::NO_ERRORS == result) || (Error::BUFFER_FULL == result));
    EXPECT_GE(out.size() - total, chunk.get_size());
    std::memcpy(out.data() + total, chunk.get_data(), chunk.get_size());
    total += chunk.get_size();
    chunk.clear();
    ++guard;
  }
  EXPECT_EQ(Error::NO_ERRORS, result);
  return total;
}

// A three byte write buffer splits inside an entry; a nine byte one splits exactly between the
// entries of by_name. Both must produce the very same bytes as a single pass serialize.
TEST(MapFields, partial_serialize_across_a_split_matches_the_full_stream)
{
  map_fields::MapTypes reference;
  reference.set_by_name("ann", 1);
  reference.set_by_name("bob", 2);
  reference.set_other(5);
  ::EmbeddedProto::WriteBufferFixedSize<256> expected;
  ASSERT_EQ(Error::NO_ERRORS, reference.serialize(expected));

  {
    // Split mid entry.
    map_fields::MapTypes out;
    out.set_by_name("ann", 1);
    out.set_by_name("bob", 2);
    out.set_other(5);
    map_fields::MapTypes::StateStack state;
    std::array<uint8_t, 256> accumulated{};
    const std::size_t total = serialize_in_chunks<3U>(out, state, accumulated);
    ASSERT_EQ(expected.get_size(), total);
    EXPECT_EQ(0, std::memcmp(expected.get_data(), accumulated.data(), total));
  }

  {
    // Split between entries, an entry of by_name is exactly nine bytes.
    map_fields::MapTypes out;
    out.set_by_name("ann", 1);
    out.set_by_name("bob", 2);
    out.set_other(5);
    map_fields::MapTypes::StateStack state;
    std::array<uint8_t, 256> accumulated{};
    const std::size_t total = serialize_in_chunks<9U>(out, state, accumulated);
    ASSERT_EQ(expected.get_size(), total);
    EXPECT_EQ(0, std::memcmp(expected.get_data(), accumulated.data(), total));
  }
}

// Feed the wire one byte at a time, so the deserializer resumes inside an entry, between the key
// and the value and between entries.
TEST(MapFields, partial_deserialize_byte_by_byte)
{
  map_fields::MapTypes out;
  out.set_by_name("ann", 1);
  out.set_by_name("bob", 2);
  map_fields::Point point;
  point.set_x(10);
  point.set_y(20);
  out.set_points(99, point);
  out.set_other(5);

  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, out.serialize(buffer));

  map_fields::MapTypes in;
  map_fields::MapTypes::StateStack state;
  ::EmbeddedProto::ReadBufferFixedSize<256> wire;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    wire.push(buffer.get_data()[i]);
    // A root message carries no terminator, so the engine keeps reporting that it ran out of bytes.
    // Being back at the TAG phase after the last byte is what says every field was fully parsed.
    ASSERT_EQ(Error::END_OF_BUFFER, in.deserialize_partial(wire, state.root()));
  }
  ASSERT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_EQ(2U, in.by_name_size());
  EXPECT_EQ(1, in.get_by_name("ann"));
  EXPECT_EQ(2, in.get_by_name("bob"));
  map_fields::Point read_back;
  EXPECT_EQ(Error::NO_ERRORS, in.get_points(99, read_back));
  EXPECT_EQ(10, read_back.get_x());
  EXPECT_EQ(20, read_back.get_y());
  EXPECT_EQ(5, in.get_other());
}

// A streaming map must pull every entry exactly once across a resume and write the same bytes a
// resident map writes.
TEST(MapFields, partial_serialize_of_a_streaming_map_pulls_each_entry_once)
{
  map_fields::SmallMap reference;
  reference.set_entries("ann", 1);
  reference.set_entries("bob", 2);
  ::EmbeddedProto::WriteBufferFixedSize<256> expected;
  ASSERT_EQ(Error::NO_ERRORS, reference.serialize(expected));

  EntryProducer producer;
  producer.keys = {"ann", "bob", nullptr, nullptr};
  producer.values = {1, 2, 0, 0};
  producer.size = 2U;
  StreamField::SourceCallback source;
  source.set(producer);

  map_fields::StreamMap out;
  out.mutable_entries().set_source(source);
  map_fields::StreamMap::StateStack state;

  std::array<uint8_t, 256> accumulated{};
  const std::size_t total = serialize_in_chunks<3U>(out, state, accumulated);

  ASSERT_EQ(expected.get_size(), total);
  EXPECT_EQ(0, std::memcmp(expected.get_data(), accumulated.data(), total));
  // Exactly two pulls, no entry was produced twice to satisfy a size pass.
  EXPECT_EQ(2U, producer.index);
}

// A streaming map consuming a wire that arrives one byte at a time pushes every entry once.
TEST(MapFields, partial_deserialize_of_a_streaming_map_across_a_split)
{
  map_fields::SmallMap reference;
  reference.set_entries("ann", 1);
  reference.set_entries("bob", 2);
  ::EmbeddedProto::WriteBufferFixedSize<256> buffer;
  ASSERT_EQ(Error::NO_ERRORS, reference.serialize(buffer));

  EntryCollector collector;
  StreamField::SinkCallback sink;
  sink.set(collector);

  map_fields::StreamMap in;
  in.mutable_entries().set_sink(sink);
  map_fields::StreamMap::StateStack state;

  ::EmbeddedProto::ReadBufferFixedSize<256> wire;
  for(uint32_t i = 0U; i < buffer.get_size(); ++i)
  {
    wire.push(buffer.get_data()[i]);
    ASSERT_EQ(Error::END_OF_BUFFER, in.deserialize_partial(wire, state.root()));
  }
  ASSERT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  ASSERT_EQ(2U, collector.count);
  EXPECT_TRUE(collector.keys[0] == "ann");
  EXPECT_TRUE(collector.keys[1] == "bob");
  EXPECT_EQ(1, collector.values[0]);
  EXPECT_EQ(2, collector.values[1]);
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // End of namespace test_EmbeddedAMS_map_fields
