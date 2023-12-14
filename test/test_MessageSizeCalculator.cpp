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

#include "gtest/gtest.h"

#include <MessageSizeCalculator.h>

#include <cstdint>
#include <limits> 


namespace test_EmbeddedAMS_MessageSizeCalculator
{

TEST(MessageSizeCalculator, push) 
{
  ::EmbeddedProto::MessageSizeCalculator msc;
  uint8_t byte = 0;
  EXPECT_EQ(0, msc.get_size());
  EXPECT_TRUE(msc.push(byte));
  EXPECT_EQ(1, msc.get_size());
  EXPECT_TRUE(msc.push(byte));
  EXPECT_EQ(2, msc.get_size());

  // Clear the content
  msc.clear();
  EXPECT_EQ(0, msc.get_size());
}

TEST(MessageSizeCalculator, push_n) 
{
  ::EmbeddedProto::MessageSizeCalculator msc;

  static constexpr uint8_t SIZE = 3;
  uint8_t bytes[SIZE] = {1, 2, 3};
  EXPECT_TRUE(msc.push(bytes, SIZE));
  EXPECT_EQ(3, msc.get_size());
}

TEST(MessageSizeCalculator, unlimited_size) 
{
  ::EmbeddedProto::MessageSizeCalculator msc;
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msc.get_max_size());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), msc.get_available_size());
}

} // End of namespace test_EmbeddedAMS_MessageSizeCalculator