
#include <gtest/gtest.h>

#include <FieldFixed.h>


namespace test_EmbeddedAMS_Fields
{

  TEST(FieldFixed, set_get)
  {
    constexpr double  RANDOM_DOUBLE = 1682.78324;
    constexpr float   RANDOM_FLOAT = 2472.25241;

    EmbeddedProto::fixed64 a(1);

    // Default value should be zero.
    EXPECT_EQ(0, a.get());

    // Using set
    a.set(1U);
    EXPECT_EQ(1U, a.get());
    
    // Test the other types
    EmbeddedProto::sfixed64 b(2);
    EXPECT_EQ(0, b.get());
    b.set(-1);
    EXPECT_EQ(-1, b.get());

    EmbeddedProto::double64 c(3);
    EXPECT_EQ(0.0, c.get());
    c.set(RANDOM_DOUBLE);
    EXPECT_EQ(RANDOM_DOUBLE, c.get());


    EmbeddedProto::fixed32 d(4);
    EXPECT_EQ(0, d.get());
    d.set(1U);
    EXPECT_EQ(1U, d.get());

    EmbeddedProto::sfixed32 e(5);
    EXPECT_EQ(0, e.get());
    e.set(-1);
    EXPECT_EQ(-1, e.get());

    EmbeddedProto::float32 f(6);
    EXPECT_EQ(0.0F, f.get());
    f.set(RANDOM_FLOAT);
    EXPECT_EQ(RANDOM_FLOAT, f.get());

  }

} // End of namespace test_EmbeddedAMS_Fields
