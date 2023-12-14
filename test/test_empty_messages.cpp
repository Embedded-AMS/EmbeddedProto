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
#include "gmock/gmock.h"

#include "empty_message.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_empty_messages
{

TEST(EmptyMessages, construction)
{ 
  // Test if using a message with no fields or enums cause build errors. 
  empty_message empty;
}

TEST(EmptyMessage, clear)
{
  empty_message empty;
  empty.clear();
}

TEST(EmptyMessage, serialize)
{
  Mocks::WriteBufferMock buffer;
  empty_message empty;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, empty.serialize(buffer));
}

TEST(EmptyMessage, deserialize)
{
  Mocks::ReadBufferMock buffer;
  InSequence s;

  // Some actual data we try to use to deserialize an empty message.
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x08), Return(true)));
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));
  
  empty_message empty;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, empty.deserialize(buffer));
}


} // End of namespace test_EmbeddedAMS_empty_messages
