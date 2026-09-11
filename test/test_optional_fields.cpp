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

#include "gtest/gtest.h"

#include <EmbeddedProto/WireFormatter.h>
#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

// EAMS message definitions
#include <optional_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_OptionalFields
{

TEST(OptionalFields, zero) 
{
  ::optional_fields<5,10> msg;

  // Test an int set to zero.
  EXPECT_FALSE(msg.has_b());
  msg.set_b(0);
  EXPECT_TRUE(msg.has_b());
  msg.clear_b();
  EXPECT_FALSE(msg.has_b());

  // Test a floating point to zero.
  EXPECT_FALSE(msg.has_y());
  msg.set_y(0.0F);
  EXPECT_TRUE(msg.has_y());
  msg.clear_y();
  EXPECT_FALSE(msg.has_y());

  // Address a nested message but do not set anything in it.
  EXPECT_FALSE(msg.has_pos());
  msg.mutable_pos().clear();
  EXPECT_TRUE(msg.has_pos());
  msg.clear_pos();
  EXPECT_FALSE(msg.has_pos());

  // Test an enum set to zero.
  EXPECT_FALSE(msg.has_state());
  msg.set_state(states::A);
  EXPECT_TRUE(msg.has_state());
  msg.clear_state();
  EXPECT_FALSE(msg.has_state());

  // Assign through the mutable reference, this must set the presence bit as well.
  msg.mutable_state() = states::C;
  EXPECT_TRUE(msg.has_state());
  EXPECT_EQ(states::C, msg.get_state());
  msg.clear_state();
  EXPECT_FALSE(msg.has_state());

  // Address a bytes array but do not set anything in it.
  EXPECT_FALSE(msg.has_bytes_array());
  msg.mutable_bytes_array().clear();
  EXPECT_TRUE(msg.has_bytes_array());
  msg.clear_bytes_array();
  EXPECT_FALSE(msg.has_bytes_array());

  // Address a string but do not set anything in it.
  EXPECT_FALSE(msg.has_str());
  msg.mutable_str().clear();
  EXPECT_TRUE(msg.has_str());
  msg.clear_str();
  EXPECT_FALSE(msg.has_str());

}

TEST(OptionalFields, construction_assignment) 
{
  ::optional_fields<5,10> msgA;
  msgA.set_b(1);
  msgA.mutable_pos().set_xpos(1.0F);
  msgA.set_state(states::B);
  msgA.mutable_bytes_array()[0] = 1U;
  msgA.mutable_str().set("ABC", 3);

  optional_fields<5,10> msgB(msgA);
  EXPECT_TRUE(msgB.has_b());
  EXPECT_FALSE(msgB.has_y());
  EXPECT_TRUE(msgB.has_pos());
  EXPECT_TRUE(msgB.has_state());
  EXPECT_TRUE(msgB.has_bytes_array());
  EXPECT_TRUE(msgB.has_str());

  optional_fields<5,10> msgC = msgA;
  EXPECT_TRUE(msgC.has_b());
  EXPECT_FALSE(msgC.has_y());
  EXPECT_TRUE(msgC.has_pos());
  EXPECT_TRUE(msgC.has_state());
  EXPECT_TRUE(msgC.has_bytes_array());
  EXPECT_TRUE(msgC.has_str());

  optional_fields<5,10> msgD;
  msgD = msgA;
  EXPECT_TRUE(msgD.has_b());
  EXPECT_FALSE(msgD.has_y());
  EXPECT_TRUE(msgD.has_pos());
  EXPECT_TRUE(msgD.has_state());
  EXPECT_TRUE(msgD.has_bytes_array());
  EXPECT_TRUE(msgD.has_str());
}

TEST(OptionalFields, clear)
{
  // Clearing one field should not influence the presence of another.
  ::optional_fields<5,10> msg;
  msg.set_b(1);
  msg.set_y(1.0F);
  msg.mutable_pos().set_xpos(1.0F);
  msg.set_state(states::B);
  msg.mutable_bytes_array()[0] = 1U;
  msg.mutable_str().set("ABC", 3);

  EXPECT_TRUE(msg.has_b());
  EXPECT_TRUE(msg.has_y());
  EXPECT_TRUE(msg.has_pos());
  EXPECT_TRUE(msg.has_state());
  EXPECT_TRUE(msg.has_bytes_array());
  EXPECT_TRUE(msg.has_str());

  msg.clear_y();

  EXPECT_TRUE(msg.has_b());
  EXPECT_FALSE(msg.has_y());
  EXPECT_TRUE(msg.has_pos());
  EXPECT_TRUE(msg.has_state());
  EXPECT_TRUE(msg.has_bytes_array());
  EXPECT_TRUE(msg.has_str());

  msg.clear_state();

  EXPECT_TRUE(msg.has_b());
  EXPECT_FALSE(msg.has_y());
  EXPECT_TRUE(msg.has_pos());
  EXPECT_FALSE(msg.has_state());
  EXPECT_TRUE(msg.has_bytes_array());
  EXPECT_TRUE(msg.has_str());

  msg.clear_str();

  EXPECT_TRUE(msg.has_b());
  EXPECT_FALSE(msg.has_y());
  EXPECT_TRUE(msg.has_pos());
  EXPECT_FALSE(msg.has_state());
  EXPECT_TRUE(msg.has_bytes_array());
  EXPECT_FALSE(msg.has_str());
}

TEST(OptionalFields, empty_serialization) 
{
  ::optional_fields<5,10> msg;

  // The actual values are set to their default values or clear such that the presense flag will remain active.
  msg.set_b(0);
  msg.set_y(0.0F);
  msg.mutable_pos().clear();
  msg.set_state(states::A);
  msg.mutable_bytes_array().clear();
  msg.mutable_str().clear();

  InSequence s;
  Mocks::WriteBufferMock buffer;

  // Fields are serialized in field number order
  // b is field 2 (varint)
  std::array<uint8_t, 2> expected_b = { 0x10, 0x00 };
  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // y is field 4 (fixed32)
  std::array<uint8_t, 5> expected_y = { 0x25, 0x00, 0x00, 0x00, 0x00 };
  for(auto e : expected_y) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // pos is field 5 - nested message (LENGTH_DELIMITED)
  EXPECT_CALL(buffer, push(0x2a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  // get_available_size() is called after writing tag and size for empty pos message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  // state is field 6 (varint)
  std::array<uint8_t, 2> expected_state = { 0x30, 0x00 };
  for(auto e : expected_state) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // bytes_array is field 7 (LENGTH_DELIMITED)
  EXPECT_CALL(buffer, push(0x3a)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  // get_available_size() is called after writing tag and size for empty bytes_array
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  // str is field 8 (LENGTH_DELIMITED)
  EXPECT_CALL(buffer, push(0x42)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(0x00)).Times(1).WillOnce(Return(true));

  // get_available_size() is called after writing tag and size for empty str
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(10));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
} 

TEST(OptionalFields, cleared_serialization) 
{
  ::optional_fields<5,10> msg;
  // Clear the presense flags no data is to be expected.
  msg.clear_b();
  msg.clear_y();
  msg.clear_pos();
  msg.clear_state();
  msg.clear_bytes_array();
  msg.clear_str();

  InSequence s;
  Mocks::WriteBufferMock buffer;

  // No data is expected to be pushed into the buffer.
  EXPECT_CALL(buffer, push(_)).Times(0).WillOnce(Return(true));

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

TEST(OptionalFields, empty_deserialization)
{
  // Explicitly present fields with default/empty values must still set the
  // presence flags, including the empty nested message (pos) and empty
  // string/bytes fields.
  ::optional_fields<5,10> msg;
  ::optional_fields<5,10>::StateStack state;

  static constexpr uint32_t SIZE = 15;

  ::EmbeddedProto::ReadBufferFixedSize<SIZE> buffer({ 0x10, 0x00, // b
                                                      0x25, 0x00, 0x00, 0x00, 0x00, // y
                                                      0x2a, 0x00,  // pos
                                                      0x30, 0x00,  // state
                                                      0x3a, 0x00,  // bytes_array
                                                      0x42, 0x00}); // str

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);

  EXPECT_TRUE(msg.has_b());
  EXPECT_TRUE(msg.has_y());
  EXPECT_TRUE(msg.has_pos());
  EXPECT_TRUE(msg.has_state());
  EXPECT_TRUE(msg.has_bytes_array());
  EXPECT_TRUE(msg.has_str());
}

TEST(ManyOptFields, deserialize_partial_split_in_multibyte_tag)
{
  // Field a16 (number 16) has a two-byte tag: (16 << 3) | 0 = 128 -> 0x80 0x01.
  // Split the buffer inside that tag varint to verify the tag resumes correctly.
  ::many_opt_fields msg;
  ::many_opt_fields::StateStack state;

  ::EmbeddedProto::ReadBufferFixedSize<3> buffer({0x80}); // first byte of the tag only

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));

  buffer.push(0x01); // rest of the tag
  buffer.push(0x01); // a16 = 1

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize_partial(buffer, state.root()));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::TAG, state.root().phase);
  EXPECT_TRUE(msg.has_a16());
  EXPECT_EQ(1, msg.get_a16());
}

#endif // PARTIAL_SERIALIZATION_ENABLED

TEST(OptionalFields, cleared_deserialization) 
{
  ::optional_fields<5,10> msg;

  InSequence s;

  Mocks::ReadBufferMock buffer;

  static constexpr uint32_t SIZE = 0;

  ON_CALL(buffer, get_size()).WillByDefault(Return(SIZE));

  EXPECT_CALL(buffer, peek(_, _)).Times(1).WillOnce(Return(false));

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.deserialize(buffer));

  EXPECT_FALSE(msg.has_b());
  EXPECT_FALSE(msg.has_y());
  EXPECT_FALSE(msg.has_pos());
  EXPECT_FALSE(msg.has_state());
  EXPECT_FALSE(msg.has_bytes_array());
  EXPECT_FALSE(msg.has_str());
}

TEST(OptionalFields, presence_size)
{
  many_opt_fields msg;
  msg.set_a33(1);
  EXPECT_TRUE(msg.has_a33());
  EXPECT_FALSE(msg.has_a32());

  msg.set_a32(1);
  EXPECT_TRUE(msg.has_a32());
} 


} // End of namespace test_EmbeddedAMS_OptionalFields
