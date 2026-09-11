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

#include <EmbeddedProto/ReadBufferSection.h>

#include "mock/ReadBufferMock.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;


namespace test_EmbeddedAMS_ReadBufferSection
{

TEST(ReadBufferSection, construction) 
{
  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).WillRepeatedly(Return(99));

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 1);
  EXPECT_EQ(1, read_buffer_section.get_max_size());
  EXPECT_EQ(1, read_buffer_section.get_size());
}

TEST(ReadBufferSection, construction_limited) 
{
  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).WillRepeatedly(Return(1));

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 2);
  EXPECT_EQ(1, read_buffer_section.get_max_size());
  EXPECT_EQ(1, read_buffer_section.get_size());
}

TEST(ReadBufferSection, peek) 
{
  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).WillRepeatedly(Return(1));
  EXPECT_CALL(read_buffer_mock, peek(_)).WillOnce(DoAll(SetArgReferee<0>(1), Return(true)));

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 1);
  
  uint8_t byte = 0;
  EXPECT_TRUE(read_buffer_section.peek(byte));
  EXPECT_EQ(1, byte);
}

TEST(ReadBufferSection, advance) 
{
  InSequence s;

  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).Times(2).WillRepeatedly(Return(3));
  EXPECT_CALL(read_buffer_mock, advance());
  EXPECT_CALL(read_buffer_mock, advance());

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 2);
  EXPECT_EQ(2, read_buffer_section.get_max_size());
  EXPECT_EQ(2, read_buffer_section.get_size());

  read_buffer_section.advance();
  EXPECT_EQ(1, read_buffer_section.get_size());  

  read_buffer_section.advance();
  EXPECT_EQ(0, read_buffer_section.get_size());  

  // One more advance should still be zero.
  read_buffer_section.advance();
  EXPECT_EQ(0, read_buffer_section.get_size());

}

TEST(ReadBufferSection, advance_n) 
{
  InSequence s;

  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).Times(2).WillRepeatedly(Return(3));
  EXPECT_CALL(read_buffer_mock, advance(2));
  EXPECT_CALL(read_buffer_mock, advance(1));

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 3);
  EXPECT_EQ(3, read_buffer_section.get_max_size());
  EXPECT_EQ(3, read_buffer_section.get_size());

  read_buffer_section.advance(2);
  EXPECT_EQ(1, read_buffer_section.get_size());  

  // We should not advance beond the max size.
  read_buffer_section.advance(2);
  EXPECT_EQ(0, read_buffer_section.get_size()); 
}

TEST(ReadBufferSection, pop) 
{
  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).WillRepeatedly(Return(1));
  EXPECT_CALL(read_buffer_mock, pop(_)).WillOnce(DoAll(SetArgReferee<0>(1), Return(true)));

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 1);
  
  uint8_t byte = 0;
  EXPECT_TRUE(read_buffer_section.pop(byte));
  EXPECT_EQ(1, byte);
  EXPECT_EQ(0, read_buffer_section.get_size());

  // When attempting to read or peek at more we should not change byte and get a false.
  byte = 0;
  EXPECT_FALSE(read_buffer_section.pop(byte));
  EXPECT_EQ(0, byte);
  EXPECT_EQ(0, read_buffer_section.get_size());
  EXPECT_FALSE(read_buffer_section.peek(byte));
  EXPECT_EQ(0, byte);
}

TEST(ReadBufferSection, pop_block)
{
  // A block fully inside the section is delegated to the parent buffer in a
  // single call and shrinks the section by that many bytes.
  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).WillRepeatedly(Return(8));
  const uint8_t src[3] = { 1, 2, 3 };
  EXPECT_CALL(read_buffer_mock, pop(_, 3U)).WillOnce(
      [&](uint8_t* dst, uint32_t n){ memcpy(dst, src, n); return true; });

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 5);

  uint8_t dest[3] = { 0, 0, 0 };
  EXPECT_TRUE(read_buffer_section.pop(dest, 3));
  EXPECT_EQ(1, dest[0]);
  EXPECT_EQ(2, dest[1]);
  EXPECT_EQ(3, dest[2]);
  EXPECT_EQ(2, read_buffer_section.get_size());
}

TEST(ReadBufferSection, pop_block_respects_boundary)
{
  // A block larger than what the section exposes is refused without touching the
  // parent buffer, so a fixed-width element straddling the section boundary is
  // never half-consumed.
  Mocks::ReadBufferMock read_buffer_mock;
  EXPECT_CALL(read_buffer_mock, get_size()).WillRepeatedly(Return(8));
  EXPECT_CALL(read_buffer_mock, pop(_, _)).Times(0);

  EmbeddedProto::ReadBufferSection read_buffer_section(read_buffer_mock, 3);

  uint8_t dest[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
  EXPECT_FALSE(read_buffer_section.pop(dest, 4));
  EXPECT_EQ(0xFF, dest[0]); // Untouched on a boundary-exceeding read.
  EXPECT_EQ(3, read_buffer_section.get_size());
}

} // End of namespace ReadBufferSection
