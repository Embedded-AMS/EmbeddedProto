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

// EAMS message definitions
#include <options_from_file.h>
#include <options_from_file_second.h>

namespace test_EmbeddedAMS_OptionsFromFile
{

// options_from_file.proto carries no Embedded Proto options at all. Every length below comes from
// test/options/embedded_proto_test.options.json, read by the generator through the options_file parameter set in
// CMakeLists.txt. This is the whole feature seen from the outside: a schema you can not edit, sized from a file
// that lives somewhere else entirely.

TEST(OptionsFromFile, message_needs_no_template_parameter)
{
  // Without the options file the lengths would be unknown and these classes would require a template parameter for
  // each of them. That this compiles is in itself the proof that the file was read.
  OptionsFromFile::Frame msg;
  EXPECT_EQ(0, msg.get_samples().get_length());
}

TEST(OptionsFromFile, repeated_field_length)
{
  OptionsFromFile::Frame msg;
  EXPECT_EQ(16, msg.get_samples().get_max_length());
}

TEST(OptionsFromFile, string_and_bytes_length)
{
  OptionsFromFile::Frame msg;
  EXPECT_EQ(32, msg.get_name().get_max_length());
  EXPECT_EQ(64, msg.get_blob().get_max_length());
}

TEST(OptionsFromFile, array_and_element_length)
{
  // maxLength sizes the array, nestedMaxLength the strings in it.
  OptionsFromFile::Frame msg;
  EXPECT_EQ(4, msg.get_tags().get_max_length());
  EXPECT_EQ(12, msg.tags(0).get_max_length());
}

TEST(OptionsFromFile, nested_message_field_length)
{
  // The nested message is addressed at its nested position in the options file.
  OptionsFromFile::Frame msg;
  EXPECT_EQ(8, msg.get_inner().get_label().get_max_length());
}

TEST(OptionsFromFile, the_sized_fields_are_usable)
{
  OptionsFromFile::Frame msg;
  msg.mutable_name() = "sensor one";
  msg.mutable_tags(0) = "degrees";
  msg.mutable_inner().mutable_label() = "unit";

  EXPECT_EQ(0, strncmp("sensor one", msg.get_name().get_const(), 10));
  EXPECT_EQ(0, strncmp("degrees", msg.tags(0).get_const(), 7));
  EXPECT_EQ(0, strncmp("unit", msg.get_inner().get_label().get_const(), 4));
}

// options_from_file_second.proto is a second schema, in its own package, sized from the very same options file.
// While it is generated the entries of the other package match nothing and are skipped, which is what lets one file
// cover several schemas.

TEST(OptionsFromFileSecond, one_file_sizes_a_second_schema)
{
  OptionsFromFileSecond::Reading msg;
  EXPECT_EQ(6, msg.get_unit().get_max_length());
}

TEST(OptionsFromFileSecond, array_and_element_length)
{
  OptionsFromFileSecond::Reading msg;
  EXPECT_EQ(3, msg.get_chunks().get_max_length());
  EXPECT_EQ(20, msg.chunks(0).get_max_length());
}

TEST(OptionsFromFileSecond, nested_message_field_length)
{
  OptionsFromFileSecond::Reading msg;
  EXPECT_EQ(5, msg.get_limits().get_thresholds().get_max_length());
}

} // End of namespace test_EmbeddedAMS_OptionsFromFile
