

#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <MessageBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <simple_types.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;

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
  InSequence s;

  Mocks::MessageBufferMock buffer;
  EXPECT_CALL(buffer, push(_,_)).Times(0);


  EXPECT_CALL(buffer, push(1)).Times(1).WillOnce(Return(true));
  EXPECT_TRUE(::EmbeddedProto::WireFormatter::WriteVarint32ToArray(1, buffer));

  // Edge case of the first byte.
  EXPECT_CALL(buffer, push(127)).Times(1).WillOnce(Return(true));
  EXPECT_TRUE(::EmbeddedProto::WireFormatter::WriteVarint32ToArray(127, buffer));

  // Just over the first byte.
  EXPECT_CALL(buffer, push(128)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(1)).Times(1).WillOnce(Return(true));
  EXPECT_TRUE(::EmbeddedProto::WireFormatter::WriteVarint32ToArray(128, buffer));

  // Full first byte.
  EXPECT_CALL(buffer, push(255)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(1)).Times(1).WillOnce(Return(true));
  EXPECT_TRUE(::EmbeddedProto::WireFormatter::WriteVarint32ToArray(255, buffer));

  // Fast forward to the largest possible number.
  EXPECT_CALL(buffer, push(255)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(255)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(255)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(255)).Times(1).WillOnce(Return(true));
  EXPECT_CALL(buffer, push(15)).Times(1).WillOnce(Return(true));
  EXPECT_TRUE(::EmbeddedProto::WireFormatter::WriteVarint32ToArray(std::numeric_limits<uint32_t>::max(), buffer));

}

TEST(WireFormatter, SimpleTypes_zero) 
{
  InSequence s;

  // See if an empty message results in no data been pushed.
  ::Test_Simple_Types msg;
  Mocks::MessageBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);

  EXPECT_TRUE(msg.serialize(buffer));
}


TEST(WireFormatter, SimpleTypes_serialize_one) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::MessageBufferMock buffer;

  msg.set_a_int32(1);   
  msg.set_a_int64(1);     
  msg.set_a_uint32(1);    
  msg.set_a_uint64(1);
  msg.set_a_sint32(1);
  msg.set_a_sint64(1);
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::ONE);
  msg.set_a_fixed64(1);
  msg.set_a_sfixed64(1);
  msg.set_a_double(1.0);
  msg.set_a_fixed32(1);
  msg.set_a_sfixed32(1); 
  msg.set_a_float(1.0F);

  uint8_t expected[] = {0x08, 0x01, 0x10, 0x01, 0x18, 0x01, 0x20, 0x01, 0x28, 0x02, 0x30, 0x02, 0x38, 0x01, 0x40, 0x01, 0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x65, 0x01, 0x00, 0x00, 0x00, 0x6d, 0x01, 0x00, 0x00, 0x00, 0x75, 0x00, 0x00, 0x80, 0x3f, 
};
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}

TEST(WireFormatter, SimpleTypes_serialize_max) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::MessageBufferMock buffer;

  msg.set_a_int32(std::numeric_limits<int32_t>::max());   
  msg.set_a_int64(std::numeric_limits<int64_t>::max());     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::max());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sint32(std::numeric_limits<int32_t>::max());
  msg.set_a_sint64(std::numeric_limits<int64_t>::max());
  msg.set_a_bool(true);
  msg.set_a_enum(Test_Enum::TWOBILLION);
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::max());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::max());
  msg.set_a_double(std::numeric_limits<double>::max());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::max());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::max()); 
  msg.set_a_float(std::numeric_limits<float>::max());


  uint8_t expected[] = {0x08, 0xFF, 0xFF, 0xFF, 0xFF, 
                        0x07, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
                        0x7F, 0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                        0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                        0x28, 0xFE, 0xFF, 0xFF, 0xFF, 0x0F, 
                        0x30, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 
                        0x38, 0x01, 
                        0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07,
                        0x49, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
                        0x51, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
                        0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, 
                        0x65, 0xFF, 0xFF, 0xFF, 0xFF, 
                        0x6D, 0xFF, 0xFF, 0xFF, 0x7F, 
                        0x75, 0xFF, 0xFF, 0x7F, 0x7F};
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));

}

TEST(WireFormatter, SimpleTypes_serialize_min) 
{
  InSequence s;
  
  // Using a protobuf message and the google protobuf implementation test is serialization is 
  // correct.
  ::Test_Simple_Types msg;
  Mocks::MessageBufferMock buffer;

  msg.set_a_int32(std::numeric_limits<int32_t>::min());   
  msg.set_a_int64(std::numeric_limits<int64_t>::min());     
  msg.set_a_uint32(std::numeric_limits<uint32_t>::min());    
  msg.set_a_uint64(std::numeric_limits<uint64_t>::min());
  msg.set_a_sint32(std::numeric_limits<int32_t>::min());
  msg.set_a_sint64(std::numeric_limits<int64_t>::min());
  msg.set_a_bool(false);
  msg.set_a_enum(Test_Enum::ZERO);
  msg.set_a_fixed64(std::numeric_limits<uint64_t>::min());
  msg.set_a_sfixed64(std::numeric_limits<int64_t>::min());
  msg.set_a_double(std::numeric_limits<double>::lowest());
  msg.set_a_fixed32(std::numeric_limits<uint32_t>::min());
  msg.set_a_sfixed32(std::numeric_limits<int32_t>::min()); 
  msg.set_a_float(std::numeric_limits<float>::lowest());

  uint8_t expected[] = {0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                        0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                        0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
                        0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
                        0x51, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
                        0x59, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0xFF, 
                        0x6D, 0x00, 0x00, 0x00, 0x80, 
                        0x75, 0xFF, 0xFF, 0x7F, 0xFF};
  
  for(auto e : expected) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }
  
  EXPECT_TRUE(msg.serialize(buffer));

}

} // End of namespace test_EmbeddedAMS_WireFormatter
