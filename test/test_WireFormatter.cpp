

#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <cstdint>    
#include <limits> 

namespace test_EmbeddedAMS_WireFormatter 
{

TEST(WireFormatter, MakeTag)
{
  // Varints
  EXPECT_EQ(0x0000, ::EmbeddedProto::WireFormatter::MakeTag(0, ::EmbeddedProto::WireFormatter::WireType::VARINT));
  EXPECT_EQ(0x0008, ::EmbeddedProto::WireFormatter::MakeTag(1, ::EmbeddedProto::WireFormatter::WireType::VARINT));
  EXPECT_EQ(0x07F8, ::EmbeddedProto::WireFormatter::MakeTag(255, ::EmbeddedProto::WireFormatter::WireType::VARINT));

  // Doubles
  EXPECT_EQ(0x0001, ::EmbeddedProto::WireFormatter::MakeTag(0, ::EmbeddedProto::WireFormatter::WireType::FIXED64));
  EXPECT_EQ(0x0009, ::EmbeddedProto::WireFormatter::MakeTag(1, ::EmbeddedProto::WireFormatter::WireType::FIXED64));
  EXPECT_EQ(0x07F9, ::EmbeddedProto::WireFormatter::MakeTag(255, ::EmbeddedProto::WireFormatter::WireType::FIXED64));

  // Repeated fields
  EXPECT_EQ(0x0002, ::EmbeddedProto::WireFormatter::MakeTag(0, ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED));
  EXPECT_EQ(0x000A, ::EmbeddedProto::WireFormatter::MakeTag(1, ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED));
  EXPECT_EQ(0x07FA, ::EmbeddedProto::WireFormatter::MakeTag(255, ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED));

  // Skip the depricated group type.

  // Floats
  EXPECT_EQ(0x0005, ::EmbeddedProto::WireFormatter::MakeTag(0, ::EmbeddedProto::WireFormatter::WireType::FIXED32));
  EXPECT_EQ(0x000D, ::EmbeddedProto::WireFormatter::MakeTag(1, ::EmbeddedProto::WireFormatter::WireType::FIXED32));
  EXPECT_EQ(0x07FD, ::EmbeddedProto::WireFormatter::MakeTag(255, ::EmbeddedProto::WireFormatter::WireType::FIXED32));
}

TEST(WireFormatter, WriteVarint32ToArray) 
{

  // There are five bytes required to encode a 32bit value, take one extra for checking the return 
  // values.
  uint8_t target[6]; 

  EXPECT_EQ(&target[1], ::EmbeddedProto::WireFormatter::WriteVarint32ToArray(0, target));
  EXPECT_EQ(0, target[0]);

  EXPECT_EQ(&target[1], ::EmbeddedProto::WireFormatter::WriteVarint32ToArray(1, target));
  EXPECT_EQ(1, target[0]);

  // Edge case of the first byte.
  EXPECT_EQ(&target[1], ::EmbeddedProto::WireFormatter::WriteVarint32ToArray(127, target));
  EXPECT_EQ(127, target[0]);

  // Just over the first byte.
  EXPECT_EQ(&target[2], ::EmbeddedProto::WireFormatter::WriteVarint32ToArray(128, target));
  EXPECT_EQ(128, target[0]);
  EXPECT_EQ(1, target[1]);

  // Full first byte.
  EXPECT_EQ(&target[2], ::EmbeddedProto::WireFormatter::WriteVarint32ToArray(255, target));
  EXPECT_EQ(255, target[0]);
  EXPECT_EQ(1, target[1]);

  // Fast forward to the largest possible number.
  EXPECT_EQ(&target[5], ::EmbeddedProto::WireFormatter::WriteVarint32ToArray(std::numeric_limits<uint32_t>::max(), target));
  EXPECT_EQ(255, target[0]);
  EXPECT_EQ(255, target[1]);
  EXPECT_EQ(255, target[2]);
  EXPECT_EQ(255, target[3]);
  EXPECT_EQ(15, target[4]);

}

} // End of namespace test_EmbeddedAMS_WireFormatter
