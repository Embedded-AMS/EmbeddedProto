

#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <simple_types.h>

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

TEST(WireFormatter, SimpleTypes_zero) 
{
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;

  uint8_t buffer[10];
  msg.serialize(buffer, 10);
}

TEST(WireFormatter, SimpleTypes_one) 
{
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);

  uint8_t buffer[100] = {0};
  msg.serialize(buffer, 100);

  uint8_t expected[] = {0x08, 0x01, 0x10, 0x01, 0x18, 0x01, 0x20, 0x01, 0x28, 0x02, 0x30, 0x02, 0x38, 0x01, 0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x65, 0x01, 0x00, 0x00, 0x00, 0x6D, 0x01, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x80, 0x3F};
  uint8_t l = sizeof(expected);

  // Check the serialized data
  for(uint8_t i = 0; i < l; ++i) {
    EXPECT_EQ(expected[i], buffer[i]);
  }

  // Check no additional data is generated.
  for(uint8_t i = l; i < 100; ++i) {
    EXPECT_EQ(0, buffer[i]);
  }
}

TEST(WireFormatter, SimpleTypes_max) 
{
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;

  msg.set_a_int32(std::numeric_limits<int32_t>::max());   
  msg.set_a_int64(std::numeric_limits<int64_t>::max());     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::max());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sint32(std::numeric_limits<int32_t>::max());
  msg.set_a_sint64(std::numeric_limits<int64_t>::max());
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::max());
  msg.set_a_double(std::numeric_limits<double>::max());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::max());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::max()); 
  msg.set_a_float(std::numeric_limits<float>::max());

  uint8_t buffer[100] = {0};
  msg.serialize(buffer, 100);

  uint8_t expected[] = {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x28, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 0x30, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, 0x65, 0xFF, 0xFF, 0xFF, 0xFF, 0x6D, 0xFF, 0xFF, 0xFF, 0x7F, 0x75, 0xFF, 0xFF, 0x7F, 0x7F};
  uint8_t l = sizeof(expected);

  // Check the serialized data
  for(uint8_t i = 0; i < l; ++i) {
    EXPECT_EQ(expected[i], buffer[i]);
  }

  // Check no additional data is generated.
  for(uint8_t i = l; i < 100; ++i) {
    EXPECT_EQ(0, buffer[i]);
  }
}

} // End of namespace test_EmbeddedAMS_WireFormatter
