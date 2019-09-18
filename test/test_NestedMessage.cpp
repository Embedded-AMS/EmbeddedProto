

#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <MessageBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <nested_message.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace test_EmbeddedAMS_NestedMessage
{

TEST(NestedMessage, serialize_zero) 
{
  // Test if a unset message results in zero bytes in the buffer.

  ::message_b msg;
  Mocks::MessageBufferMock buffer;
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(99));

  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(0, msg.serialized_size());
}

TEST(NestedMessage, serialize_one) 
{
  InSequence s;

  ::message_b msg;
  Mocks::MessageBufferMock buffer;

  // Test if a nested message can be serialized with values set to one.
  msg.set_u(1.0F);
  msg.set_v(1.0F);
  msg.mutable_nested_a().set_x(1U);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);

  uint8_t expected_uv[] = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                           0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F};//, // v

  for(auto e : expected_uv) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(9));

  uint8_t expected_a[] = {0x1a, 0x09, // tag and size of nested a
                          0x08, 0x01, // x
                          0x15, 0x00, 0x00, 0x80, 0x3f, // y
                          0x18, 0x02}; // z

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }


  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(29, msg.serialized_size());
}

TEST(NestedMessage, serialize_max) 
{
  InSequence s;

  ::message_b msg;
  Mocks::MessageBufferMock buffer;

  // Test if a nested message can be serialized with values set to one.
  msg.set_u(std::numeric_limits<double>::max());
  msg.set_v(std::numeric_limits<double>::max());
  msg.mutable_nested_a().set_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());

  uint8_t expected_b[] = {0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, // u
                          0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F};//, // v

  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(22));

  uint8_t expected_a[] = {0x1A, 0x16, // tag and size of nested a
                          0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                          0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                          0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}; // z

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(42, msg.serialized_size());
}

TEST(NestedMessage, serialize_nested_in_nested_max) 
{
  InSequence s;

  ::message_c msg;
  Mocks::MessageBufferMock buffer;

  // Test if a nested message in a nested message with some data works.
  msg.mutable_nested_b().set_u(std::numeric_limits<double>::max());
  msg.mutable_nested_b().set_v(std::numeric_limits<double>::max());
  msg.mutable_nested_b().mutable_nested_a().set_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_b().mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_b().mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(42));

  uint8_t expected_b[] = {0x0A, 0x2A, // tag and size of nested b
                          0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, // u
                          0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F};//, // v

  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(22));

  uint8_t expected_a[] = {0x1A, 0x16, // tag and size of nested a
                          0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                          0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                          0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}; // z

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}

} // End of namespace test_EmbeddedAMS_NestedMessage
