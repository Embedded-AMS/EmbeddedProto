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

#include <Functional.h>

namespace test_EmbeddedAMS_Functional
{

void free_function_increment(int32_t value, int32_t& target)
{
  target += value;
}

void free_function_with_context(void* context, int32_t value)
{
  int32_t* target = static_cast<int32_t*>(context);
  if(nullptr != target)
  {
    *target += value;
  }
}

class Counter
{
  public:
    void increment(int32_t value)
    {
      value_ += value;
    }

    void increment_const(int32_t value) const
    {
      mutable_value_ += value;
    }

    int32_t value_ = 0;
    mutable int32_t mutable_value_ = 0;
};

TEST(Functional, default_not_set)
{
  ::EmbeddedProto::Functional<void(int32_t)> callback;

  EXPECT_FALSE(callback.is_set());
  callback.invoke(7);
  EXPECT_FALSE(callback.is_set());
}

TEST(Functional, bind_free_function)
{
  ::EmbeddedProto::Functional<void(int32_t, int32_t&)> callback;
  int32_t value = 0;

  callback.set(&free_function_increment);
  EXPECT_TRUE(callback.is_set());

  callback(5, value);
  EXPECT_EQ(5, value);
}

TEST(Functional, bind_context_function)
{
  ::EmbeddedProto::Functional<void(int32_t)> callback;
  int32_t value = 1;

  callback.set(&free_function_with_context, &value);
  EXPECT_TRUE(callback.is_set());

  callback.invoke(4);
  EXPECT_EQ(5, value);
}

TEST(Functional, bind_member_function)
{
  ::EmbeddedProto::Functional<void(int32_t)> callback;
  Counter counter;

  callback.set<Counter, &Counter::increment>(&counter);
  EXPECT_TRUE(callback.is_set());

  callback(6);
  EXPECT_EQ(6, counter.value_);
}

TEST(Functional, bind_const_member_function)
{
  ::EmbeddedProto::Functional<void(int32_t)> callback;
  const Counter counter;

  callback.set<Counter, &Counter::increment_const>(&counter);
  EXPECT_TRUE(callback.is_set());

  callback(9);
  EXPECT_EQ(9, counter.mutable_value_);
}

TEST(Functional, bind_lambda_by_reference)
{
  ::EmbeddedProto::Functional<void(int32_t)> callback;
  int32_t value = 10;
  auto lambda = [&value](int32_t input)
  {
    value += input;
  };

  callback.set(lambda);
  EXPECT_TRUE(callback.is_set());

  callback.invoke(8);
  EXPECT_EQ(18, value);
}

TEST(Functional, clear_and_rebind)
{
  ::EmbeddedProto::Functional<void(int32_t)> callback;
  Counter counter;

  callback.set<Counter, &Counter::increment>(&counter);
  callback(2);
  EXPECT_EQ(2, counter.value_);

  callback.clear();
  EXPECT_FALSE(callback.is_set());

  callback(5);
  EXPECT_EQ(2, counter.value_);

  callback.set(&free_function_with_context, &counter.value_);
  callback(3);
  EXPECT_EQ(5, counter.value_);
}

} // namespace test_EmbeddedAMS_Functional
