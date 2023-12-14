/*
 *  Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
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
 *  <https://EmbeddedProto.com/license/>.
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

#include <ReadBufferFixedSize.h>

namespace test_EmbeddedAMS_ReadBufferFixedSize
{
  TEST(ReadBufferFixedSize, construction) 
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::ReadBufferFixedSize<BUFFER_SIZE> buffer;

    EXPECT_EQ(0, buffer.get_size());
    EXPECT_EQ(BUFFER_SIZE, buffer.get_max_size());

    uint8_t byte = 255;
    EXPECT_FALSE(buffer.peek(byte));
    EXPECT_EQ(255, byte);
    EXPECT_FALSE(buffer.advance());
    EXPECT_FALSE(buffer.advance(1));
    EXPECT_FALSE(buffer.pop(byte));
  }

  TEST(ReadBufferFixedSize, push)
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::ReadBufferFixedSize<BUFFER_SIZE> buffer;

    EXPECT_TRUE(buffer.push(0));
    EXPECT_EQ(1, buffer.get_size());

    EXPECT_TRUE(buffer.push(1));
    EXPECT_EQ(2, buffer.get_size());
    
    EXPECT_TRUE(buffer.push(2));
    EXPECT_EQ(3, buffer.get_size());
    
    EXPECT_FALSE(buffer.push(3));
    EXPECT_EQ(3, buffer.get_size());
  }

  TEST(ReadBufferFixedSize, get_data)
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::ReadBufferFixedSize<BUFFER_SIZE> buffer;

    constexpr std::array<uint8_t, BUFFER_SIZE> data = { 0, 1, 2 };

    memcpy(buffer.get_data(), data.data(), BUFFER_SIZE);
    buffer.set_bytes_written(BUFFER_SIZE);
    EXPECT_EQ(BUFFER_SIZE, buffer.get_size());
  }

  TEST(ReadBufferFixedSize, pop)
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::ReadBufferFixedSize<BUFFER_SIZE> buffer;

    constexpr std::array<uint8_t, BUFFER_SIZE> data = { 0, 1, 2 };

    memcpy(buffer.get_data(), data.data(), BUFFER_SIZE);
    buffer.set_bytes_written(BUFFER_SIZE);
  
    uint8_t byte = 255;
    EXPECT_TRUE(buffer.pop(byte));
    EXPECT_EQ(0, byte);
    EXPECT_TRUE(buffer.pop(byte));
    EXPECT_EQ(1, byte);
    EXPECT_TRUE(buffer.pop(byte));
    EXPECT_EQ(2, byte);
    EXPECT_FALSE(buffer.pop(byte));
    EXPECT_EQ(2, byte); // byte should not have changed.
  }

  TEST(ReadBufferFixedSize, peak)
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::ReadBufferFixedSize<BUFFER_SIZE> buffer;
    constexpr std::array<uint8_t, BUFFER_SIZE> data = { 0, 1, 2 };

    memcpy(buffer.get_data(), data.data(), BUFFER_SIZE);
    buffer.set_bytes_written(BUFFER_SIZE);


    uint8_t byte = 255;
    EXPECT_TRUE(buffer.peek(byte));
    EXPECT_EQ(0, byte);

    // Peaking again should not have moved the read index.
    EXPECT_TRUE(buffer.peek(byte));
    EXPECT_EQ(0, byte);

    buffer.pop(byte);

    EXPECT_TRUE(buffer.peek(byte));
    EXPECT_EQ(1, byte);

    buffer.pop(byte);

    EXPECT_TRUE(buffer.peek(byte));
    EXPECT_EQ(2, byte);

    buffer.pop(byte);
    EXPECT_FALSE(buffer.peek(byte));
  }

  TEST(ReadBufferFixedSize, advance)
  {
    constexpr uint32_t BUFFER_SIZE = 3;
    EmbeddedProto::ReadBufferFixedSize<BUFFER_SIZE> buffer;

    EXPECT_TRUE(buffer.push(0));
    EXPECT_TRUE(buffer.advance());
    EXPECT_FALSE(buffer.advance());

    EXPECT_TRUE(buffer.push(1));
    EXPECT_TRUE(buffer.push(2));
    EXPECT_TRUE(buffer.advance());
    EXPECT_TRUE(buffer.advance());

    buffer.clear();
    EXPECT_FALSE(buffer.advance());

    EXPECT_TRUE(buffer.push(0));
    EXPECT_TRUE(buffer.push(1));

    EXPECT_TRUE(buffer.advance(2));
    EXPECT_FALSE(buffer.advance(2));

  }
} // End of namespace test_EmbeddedAMS_ReadBufferFixedSize
