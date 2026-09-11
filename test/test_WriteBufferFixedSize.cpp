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

#include <EmbeddedProto/WriteBufferFixedSize.h>

namespace test_EmbeddedAMS_WriteBufferFixedSize
{
  TEST(WriteBufferFixedSize, construction) 
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::WriteBufferFixedSize<BUFFER_SIZE> buffer;
    
    EXPECT_EQ(0, buffer.get_size());
    EXPECT_EQ(BUFFER_SIZE, buffer.get_max_size());
  }

  TEST(WriteBufferFixedSize, push_byte) 
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::WriteBufferFixedSize<BUFFER_SIZE> buffer;

    EXPECT_EQ(3, buffer.get_available_size());
    EXPECT_TRUE(buffer.push(0));
    EXPECT_EQ(2, buffer.get_available_size());
    EXPECT_TRUE(buffer.push(1));
    EXPECT_EQ(1, buffer.get_available_size());
    EXPECT_TRUE(buffer.push(2));
    EXPECT_EQ(0, buffer.get_available_size());
    EXPECT_FALSE(buffer.push(3));

    for(uint32_t i = 0; i < BUFFER_SIZE; ++i) 
    {
      EXPECT_EQ(i, *(buffer.get_data() + i));
    }
  }

  TEST(WriteBufferFixedSize, push_c_array) 
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::WriteBufferFixedSize<BUFFER_SIZE> buffer;

    constexpr uint32_t DATA_SIZE = 2;
    uint8_t data[DATA_SIZE] = {0, 1};

    EXPECT_EQ(3, buffer.get_available_size());
    EXPECT_TRUE(buffer.push(data, DATA_SIZE));
    EXPECT_EQ(1, buffer.get_available_size());

    for(uint32_t i = 0; i < DATA_SIZE; ++i) 
    {
      EXPECT_EQ(i, *(buffer.get_data() + i));
    }

    // Exceeds the space left.
    EXPECT_FALSE(buffer.push(data, DATA_SIZE));
    EXPECT_EQ(1, buffer.get_available_size());
  }

  // A block push that exactly fills the remaining space must succeed, matching
  // the single-byte push() which allows filling up to BUFFER_SIZE. This keeps
  // batched (whole-block) serialization byte-identical to the per-byte path.
  TEST(WriteBufferFixedSize, push_c_array_exact_fill)
  {
    constexpr uint32_t BUFFER_SIZE = 4;
    EmbeddedProto::WriteBufferFixedSize<BUFFER_SIZE> buffer;

    uint8_t data[BUFFER_SIZE] = {10, 11, 12, 13};

    // Fill the whole buffer in one push.
    EXPECT_TRUE(buffer.push(data, BUFFER_SIZE));
    EXPECT_EQ(0, buffer.get_available_size());
    for(uint32_t i = 0; i < BUFFER_SIZE; ++i)
    {
      EXPECT_EQ(data[i], *(buffer.get_data() + i));
    }

    // A further push of any length must now fail.
    EXPECT_FALSE(buffer.push(data, 1));
    EXPECT_EQ(0, buffer.get_available_size());
  }

  // Exact fill starting from a partially filled buffer.
  TEST(WriteBufferFixedSize, push_c_array_exact_fill_partial)
  {
    constexpr uint32_t BUFFER_SIZE = 5;
    EmbeddedProto::WriteBufferFixedSize<BUFFER_SIZE> buffer;

    EXPECT_TRUE(buffer.push(0xAA));            // one byte used, 4 left
    uint8_t data[4] = {1, 2, 3, 4};
    EXPECT_TRUE(buffer.push(data, 4));         // exactly fills remaining 4
    EXPECT_EQ(0, buffer.get_available_size());
  }

  TEST(WriteBufferFixedSize, clear) 
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::WriteBufferFixedSize<BUFFER_SIZE> buffer;

    EXPECT_TRUE(buffer.push(0));
    EXPECT_EQ(2, buffer.get_available_size());
    buffer.clear();
    EXPECT_EQ(3, buffer.get_available_size());
  }

} // End of namespace test_EmbeddedAMS_WriteBufferFixedSize