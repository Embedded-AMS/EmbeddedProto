#include <gtest/gtest.h>

#include <DynamicArray.h>

namespace test_EmbeddedAMS_DynamicArray
{

TEST(DynamicArray, construction) 
{
  static constexpr uint32_t SIZE = 3;
  EmbeddedProto::DynamicArraySize<uint8_t, SIZE> x;
}

TEST(DynamicArray, size_uint8_t) 
{
  static constexpr uint32_t SIZE = 3;
  EmbeddedProto::DynamicArraySize<uint8_t, SIZE> x;

  EXPECT_EQ(0, x.get_size());
  EXPECT_EQ(SIZE, x.get_max_size());
  EXPECT_EQ(0, x.get_length());
  EXPECT_EQ(SIZE, x.get_max_length());

  x.add(1);
  x.add(2);
  EXPECT_EQ(2, x.get_size());
  EXPECT_EQ(2, x.get_length());

  x.add(3);

  EXPECT_EQ(SIZE, x.get_size());
  EXPECT_EQ(SIZE, x.get_max_size());
  EXPECT_EQ(SIZE, x.get_length());
  EXPECT_EQ(SIZE, x.get_max_length());
}

TEST(DynamicArray, size_uint32_t) 
{  
  static constexpr uint32_t SIZE = 3;
  EmbeddedProto::DynamicArraySize<uint32_t, SIZE> x;

  EXPECT_EQ(0, x.get_size());
  EXPECT_EQ(SIZE*4, x.get_max_size());
  EXPECT_EQ(0, x.get_length());
  EXPECT_EQ(SIZE, x.get_max_length());

  x.add(1);
  x.add(2);
  EXPECT_EQ(2*4, x.get_size());
  EXPECT_EQ(2, x.get_length());

  x.add(3);

  EXPECT_EQ(SIZE*4, x.get_size());
  EXPECT_EQ(SIZE*4, x.get_max_size());
  EXPECT_EQ(SIZE, x.get_length());
  EXPECT_EQ(SIZE, x.get_max_length());
}

TEST(DynamicArray, set) 
{
  static constexpr uint32_t SIZE = 3;
  EmbeddedProto::DynamicArraySize<uint8_t, SIZE> x;

  // First add a value in the middle and see if we have a size of two.
  x.set(1, 2);
  EXPECT_EQ(2, x.get(1));
  EXPECT_EQ(2, x.get_length());

  x.set(0, 1);
  EXPECT_EQ(1, x.get(0));

  x.set(2, 3);
  EXPECT_EQ(3, x.get(2));


}

} // End namespace test_EmbeddedAMS_DynamicArray