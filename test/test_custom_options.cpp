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

#include "custom_options.h"

#include <WriteBufferFixedSize.h>
#include <ReadBufferFixedSize.h>

namespace test_EmbeddedAMS_custom_options
{

// The proto file of this test declares protoc custom options. This requires importing
// google/protobuf/descriptor.proto. Embedded Proto does not generate code for that file. These tests mainly assure
// the generated code builds and behaves as usual.

TEST(CustomOptions, construction)
{
  CustomOptions<10, 4> msg;
  EXPECT_EQ(0, msg.get_value());
  EXPECT_EQ(0, msg.get_samples().get_length());
  EXPECT_EQ(0, msg.get_name().get_length());
}

TEST(CustomOptions, serialize_deserialize)
{
  ::EmbeddedProto::WriteBufferFixedSize<50> write_buffer;

  CustomOptions<10, 4> msg_out;
  msg_out.mutable_name() = "sensor";
  msg_out.set_value(-7);
  msg_out.add_samples(1);
  msg_out.add_samples(2);

  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg_out.serialize(write_buffer));

  ::EmbeddedProto::ReadBufferFixedSize<50> read_buffer;
  for(uint32_t i = 0; i < write_buffer.get_size(); ++i)
  {
    read_buffer.push(write_buffer.get_data()[i]);
  }

  CustomOptions<10, 4> msg_in;
  EXPECT_EQ(::EmbeddedProto::Error::NO_ERRORS, msg_in.deserialize(read_buffer));

  EXPECT_EQ(0, strncmp("sensor", msg_in.get_name().get_const(), 10));
  EXPECT_EQ(-7, msg_in.get_value());
  ASSERT_EQ(2, msg_in.get_samples().get_length());
  EXPECT_EQ(1, msg_in.samples(0).get());
  EXPECT_EQ(2, msg_in.samples(1).get());
}

} // End of namespace test_EmbeddedAMS_custom_options
