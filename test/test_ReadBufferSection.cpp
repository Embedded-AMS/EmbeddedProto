/*
 *  Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
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
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#include <gtest/gtest.h>

#include <ReadBufferSection.h>

#include "mock/ReadBufferMock.h"

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;


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

} // End of namespace ReadBufferSection
