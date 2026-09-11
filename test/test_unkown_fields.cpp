/*
 *  Copyright (C) 2021 Embedded AMS B.V. - All Rights Reserved
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

// In this file we test what happens when we send a newer vesion of a message and deserialize 
// that data in an older object.

#include "gtest/gtest.h"

#include <EmbeddedProto/WireFormatter.h>
#include <EmbeddedProto/ReadBufferFixedSize.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <simple_types.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_UnknownFields
{

TEST(UnknownFields, varint) 
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<19> buffer( 
                                        { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                                          0x90, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // Additional uint32
                                          0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F,  // a_uint32
                                        } );

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

TEST(UnknownFields, fixed32) 
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<18> buffer(
                                        { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                                          0x9D, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, // Additional fixed32
                                          0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // a_uint32
                                        } );

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

TEST(UnknownFields, fixed64) 
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<22> buffer(
                                  { 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                                    0xA1, 0x03, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Additional fixed64
                                    0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, // a_uint32
                                  } );

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

TEST(UnknownFields, length_delimited) 
{
  ::Test_Simple_Types msg;

  ::EmbeddedProto::ReadBufferFixedSize<20> buffer( {
                            0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // a_int32
                            0xaa, 0x03, 0x05,  // Id and size of the unkown field.
                            0x00, 0x00, 0x00, 0x00, 0x00, // Actuall unkown field
                            0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F // a_uint32
                        } );

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_a_int32()); 
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msg.get_a_uint32());  
}

#ifdef PARTIAL_SERIALIZATION_ENABLED

class TestUnknownFieldPartialMessage final : public ::EmbeddedProto::MessageInterface
{
  public:
    ::EmbeddedProto::Error serialize(::EmbeddedProto::WriteBufferInterface& buffer) const override
    {
      (void)buffer;
      return ::EmbeddedProto::Error::STATE_MISMATCH;
    }

    ::EmbeddedProto::Error deserialize(::EmbeddedProto::ReadBufferInterface& buffer) override
    {
      (void)buffer;
      return ::EmbeddedProto::Error::STATE_MISMATCH;
    }

    void clear() override
    {
      // Do nothing.
    }

    ::EmbeddedProto::Error deserialize_partial(::EmbeddedProto::ReadBufferInterface& buffer,
                                               ::EmbeddedProto::MessageState& state) override
    {
      (void)buffer;
      (void)state;
      return ::EmbeddedProto::Error::STATE_MISMATCH;
    }

    ::EmbeddedProto::Error serialize_partial(::EmbeddedProto::WriteBufferInterface& buffer,
                                             ::EmbeddedProto::MessageState& state) const override
    {
      (void)buffer;
      (void)state;
      return ::EmbeddedProto::Error::STATE_MISMATCH;
    }
};

TEST(UnknownFieldsPartialSkip, varint_split)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::VARINT;

  ::EmbeddedProto::ReadBufferFixedSize<32> buffer(
    {
      0xAC // First byte of varint payload.
    });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);

  buffer.push(0x02);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
}

TEST(UnknownFieldsPartialSkip, fixed32_split)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::FIXED32;

  ::EmbeddedProto::ReadBufferFixedSize<32> buffer(
    {
      0x01, 0x02
    });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(2U, state.bytes_remaining);

  buffer.push(0x03);
  buffer.push(0x04);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
}

TEST(UnknownFieldsPartialSkip, fixed64_split)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::FIXED64;

  ::EmbeddedProto::ReadBufferFixedSize<40> buffer(
    {
      0x01, 0x02, 0x03
    });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(5U, state.bytes_remaining);

  buffer.push(0x04);
  buffer.push(0x05);
  buffer.push(0x06);
  buffer.push(0x07);
  buffer.push(0x08);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
}

TEST(UnknownFieldsPartialSkip, length_delimited_split_in_size)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED;

  ::EmbeddedProto::ReadBufferFixedSize<200> buffer(
    {
      0x8C
    });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::SIZE, state.phase);

  buffer.push(0x01);
  for(uint32_t i = 0; i < 140U; ++i)
  {
    buffer.push(0x00);
  }

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
}

TEST(UnknownFieldsPartialSkip, length_delimited_split_in_data)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED;

  ::EmbeddedProto::ReadBufferFixedSize<40> buffer(
    {
      0x05, 0x00, 0x00
    });

  EXPECT_EQ(::EmbeddedProto::Error::END_OF_BUFFER, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
  EXPECT_EQ(3U, state.bytes_remaining);

  buffer.push(0x00);
  buffer.push(0x00);
  buffer.push(0x00);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::COMPLETE, state.phase);
  EXPECT_EQ(0U, state.bytes_remaining);
}

TEST(UnknownFieldsPartialSkip, varint_overlong_is_fatal)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::DATA;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::VARINT;

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer(
    {
      0x80, 0x80, 0x80, 0x80, 0x80,
      0x80, 0x80, 0x80, 0x80, 0x80
    });

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::DATA, state.phase);
}

TEST(UnknownFieldsPartialSkip, length_delimited_size_overlong_is_fatal)
{
  TestUnknownFieldPartialMessage msg;
  ::EmbeddedProto::MessageState state;
  state.phase = ::EmbeddedProto::FieldProcessingPhase::SIZE;
  state.wire_type = ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED;

  ::EmbeddedProto::ReadBufferFixedSize<16> buffer(
    {
      0x80, 0x80, 0x80, 0x80, 0x80
    });

  EXPECT_EQ(::EmbeddedProto::Error::OVERLONG_VARINT, msg.skip_unknown_field_partial(buffer, state));
  EXPECT_EQ(::EmbeddedProto::FieldProcessingPhase::SIZE, state.phase);
}

#endif // PARTIAL_SERIALIZATION_ENABLED

} // End of namespace test_EmbeddedAMS_UnknownFields
