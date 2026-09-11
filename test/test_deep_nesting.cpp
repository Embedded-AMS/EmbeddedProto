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

#include <EmbeddedProto/MessageSizeCalculator.h>
#include <EmbeddedProto/WriteBufferFixedSize.h>
#include <EmbeddedProto/Errors.h>

#include <array>
#include <cstdint>

// EAMS message definitions
#include <deep_nesting.h>

namespace test_EmbeddedAMS_DeepNesting
{

//! A size calculator which also counts how many bytes were actually pushed into it.
/*!
    Obtaining the size of a nested message should not require serializing its
    content, only accounting for the number of bytes it occupies. Every byte pushed
    here is a byte which was generated purely to be counted. \see PROTO-302
*/
class CountingSizeCalculator : public ::EmbeddedProto::MessageSizeCalculator
{
  public:
    uint32_t push_count = 0;

    bool push(const uint8_t byte) override
    {
      ++push_count;
      return ::EmbeddedProto::MessageSizeCalculator::push(byte);
    }

    bool push(const uint8_t* bytes, const uint32_t length) override
    {
      push_count += length;
      return ::EmbeddedProto::MessageSizeCalculator::push(bytes, length);
    }
};

//! The eight level chain holding one int32 occupies 18 bytes.
constexpr uint32_t SIZE_LEVEL_8 = 18;

//! Fill the whole chain and give the leaf a one byte value.
void populate(::deep::level_8& msg)
{
  msg.mutable_child().mutable_child().mutable_child().mutable_child()
     .mutable_child().mutable_child().mutable_child().mutable_child().set_v(1);
}

// Calculating the size of the chain may not serialize the nested messages. Only the
// tag and the length prefix of the outermost field are generated, the 16 bytes of the
// subtree are accounted for without being produced. Should the size of a subtree ever
// be calculated more than once again, this count grows with the nesting depth.
TEST(DeepNesting, calculating_the_size_does_not_serialize_the_subtree)
{
  ::deep::level_8 msg;
  populate(msg);

  CountingSizeCalculator calc;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(calc));

  EXPECT_EQ(2U, calc.push_count);
  EXPECT_EQ(SIZE_LEVEL_8, calc.get_size());
}

// The bytes on the wire and the reported size are unchanged.
TEST(DeepNesting, serialized_bytes_are_unchanged)
{
  ::deep::level_8 msg;
  populate(msg);

  ::EmbeddedProto::WriteBufferFixedSize<SIZE_LEVEL_8> buffer;
  ASSERT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg.serialize(buffer));

  const std::array<uint8_t, SIZE_LEVEL_8> expected = { 0x0A, 0x10, 0x0A, 0x0E, 0x0A, 0x0C,
                                                       0x0A, 0x0A, 0x0A, 0x08, 0x0A, 0x06,
                                                       0x0A, 0x04, 0x0A, 0x02, 0x08, 0x01 };
  ASSERT_EQ(expected.size(), buffer.get_size());
  for(uint32_t i = 0; i < expected.size(); ++i)
  {
    EXPECT_EQ(expected[i], buffer.get_data()[i]);
  }

  EXPECT_EQ(buffer.get_size(), msg.serialized_size());
}

} // End of namespace test_EmbeddedAMS_DeepNesting
