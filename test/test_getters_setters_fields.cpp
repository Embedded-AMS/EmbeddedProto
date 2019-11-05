
#include <gtest/gtest.h>

#include <Fields.h>

namespace test_EmbeddedAMS_Getters_Setters_Fields
{


TEST(getters_setters_fields, construction) 
{
  EmbeddedProto::int32 a;
  EmbeddedProto::int32 b(1);
  EmbeddedProto::int32 c = 1;
  int32_t dd = 1;
  EmbeddedProto::int32 d(dd);
  EmbeddedProto::int32 e = dd;

  EXPECT_EQ(0, a);
  EXPECT_EQ(1, b);
  EXPECT_EQ(1, c);
  EXPECT_EQ(1, d);
  EXPECT_EQ(1, e);
}

TEST(getters_setters_fields, comparison) 
{
  EmbeddedProto::int32 a(1);
  EmbeddedProto::uint32 b(1);
  EmbeddedProto::floatfixed c(0.5F);

  EXPECT_TRUE(a == 1);
  EXPECT_TRUE(a != 0);
  EXPECT_TRUE(a > 0);
  EXPECT_TRUE(a < 2);
  EXPECT_TRUE(a >= 0);
  EXPECT_TRUE(a >= 1);
  EXPECT_FALSE(a >= 2);
  EXPECT_TRUE(a <= 1);  
  EXPECT_TRUE(a <= 2);
  EXPECT_FALSE(a <= 0); 


  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  EXPECT_TRUE(a > c);
  EXPECT_TRUE(c < b);

  EXPECT_TRUE(a >= b);
  EXPECT_FALSE(a <= c);
}

} // End of namespace test_EmbeddedAMS_Getters_Setters_Fields