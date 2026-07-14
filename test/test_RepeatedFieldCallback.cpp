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

#include <Fields.h>
#include <RepeatedField.h>
#include <RepeatedFieldCallback.h>
#include <Errors.h>

#include <type_traits>

namespace test_EmbeddedAMS_RepeatedFieldCallback
{

using ::EmbeddedProto::Error;
using ::EmbeddedProto::int32;
using Callback = ::EmbeddedProto::RepeatedFieldCallback<int32>;

// A callback field must be usable anywhere a RepeatedField<int32> is expected
// (this is what the customStorage static_assert relies on).
static_assert(std::is_base_of<::EmbeddedProto::RepeatedField<int32>, Callback>::value,
              "RepeatedFieldCallback must derive from RepeatedField.");

TEST(RepeatedFieldCallback, default_state)
{
  Callback field;

  EXPECT_EQ(0U, field.get_length());
  EXPECT_EQ(0U, field.get_size());
  EXPECT_FALSE(field.is_source_set());
  EXPECT_FALSE(field.is_sink_set());

  // Streaming is unbounded, so the engine's capacity checks must never trip.
  EXPECT_GT(field.get_max_length(), field.get_length());
}

TEST(RepeatedFieldCallback, add_counts_elements)
{
  Callback field;
  int32 value;
  value.set(42);

  EXPECT_EQ(Error::NO_ERRORS, field.add(value));
  EXPECT_EQ(Error::NO_ERRORS, field.add(value));
  EXPECT_EQ(2U, field.get_length());
}

TEST(RepeatedFieldCallback, clear_resets_length)
{
  Callback field;
  int32 value;
  value.set(7);
  (void)field.add(value);
  (void)field.add(value);
  ASSERT_EQ(2U, field.get_length());

  field.clear();
  EXPECT_EQ(0U, field.get_length());
}

TEST(RepeatedFieldCallback, random_access_is_unsupported)
{
  Callback field;
  int32 out;
  EXPECT_EQ(Error::INDEX_OUT_OF_BOUND, field.get_const(0U, out));

  // set_data cannot store into a streaming field.
  int32 data[2];
  EXPECT_EQ(Error::ARRAY_FULL, field.set_data(data, 2U));
}

bool produce_nothing(int32& element)
{
  static_cast<void>(element);
  return false;
}

Error swallow(const int32& element)
{
  static_cast<void>(element);
  return Error::NO_ERRORS;
}

TEST(RepeatedFieldCallback, source_binds_and_clears)
{
  Callback field;
  EXPECT_FALSE(field.is_source_set());

  Callback::SourceCallback source;
  source.set(&produce_nothing);
  field.set_source(source);
  EXPECT_TRUE(field.is_source_set());

  field.clear_source();
  EXPECT_FALSE(field.is_source_set());
}

TEST(RepeatedFieldCallback, sink_binds_and_clears)
{
  Callback field;
  EXPECT_FALSE(field.is_sink_set());

  Callback::SinkCallback sink;
  sink.set(&swallow);
  field.set_sink(sink);
  EXPECT_TRUE(field.is_sink_set());

  field.clear_sink();
  EXPECT_FALSE(field.is_sink_set());
}

TEST(RepeatedFieldCallback, clear_keeps_bindings)
{
  Callback field;
  Callback::SinkCallback sink;
  sink.set(&swallow);
  field.set_sink(sink);

  int32 value;
  value.set(1);
  (void)field.add(value);

  field.clear();
  EXPECT_EQ(0U, field.get_length());
  EXPECT_TRUE(field.is_sink_set());
}

} // namespace test_EmbeddedAMS_RepeatedFieldCallback
