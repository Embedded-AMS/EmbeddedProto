
#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

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
  Mocks::WriteBufferMock buffer;
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
  Mocks::WriteBufferMock buffer;

  // Test if a nested message can be serialized with values set to one.
  msg.set_u(1.0F);
  msg.mutable_nested_a().set_x(1);
  msg.mutable_nested_a().set_y(1.0F);
  msg.mutable_nested_a().set_z(1);
  msg.set_v(1);

  uint8_t expected_uv[] = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F}; // u

  for(auto e : expected_uv) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(9));

  uint8_t expected_a[] = {0x12, 0x09, // tag and size of nested a
                          0x08, 0x01, // x
                          0x15, 0x00, 0x00, 0x80, 0x3f, // y
                          0x18, 0x02, // z
                          0x18, 0x01};// And back to the parent message with field v.

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }


  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(22, msg.serialized_size());
}

TEST(NestedMessage, serialize_max) 
{
  InSequence s;

  ::message_b msg;
  Mocks::WriteBufferMock buffer;

  // Test if a nested message can be serialized with values set to one.
  msg.set_u(std::numeric_limits<double>::max());
  msg.mutable_nested_a().set_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());
  msg.set_v(std::numeric_limits<int32_t>::max());

  uint8_t expected_b[] = {0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F}; // u

  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(22));

  uint8_t expected_a[] = {0x12, 0x16, // tag and size of nested a
                          0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                          0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                          0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, // z
                          // And back to the parent message with field v.
                          0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x07};

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));

  EXPECT_EQ(39, msg.serialized_size());
}

TEST(NestedMessage, serialize_nested_in_nested_max) 
{
  InSequence s;

  ::message_c msg;
  Mocks::WriteBufferMock buffer;

  // Test if a nested message in a nested message with some data works.
  msg.mutable_nested_b().set_u(std::numeric_limits<double>::max());
  msg.mutable_nested_b().mutable_nested_a().set_x(std::numeric_limits<int32_t>::max());
  msg.mutable_nested_b().mutable_nested_a().set_y(std::numeric_limits<float>::max());
  msg.mutable_nested_b().mutable_nested_a().set_z(std::numeric_limits<int64_t>::max());
  msg.mutable_nested_b().set_v(std::numeric_limits<int32_t>::max());

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(42));

  uint8_t expected_b[] = {0x0A, 0x27, // tag and size of nested b
                          0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F}; // u

  for(auto e : expected_b) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(22));

  uint8_t expected_a[] = {0x12, 0x16, // tag and size of nested a
                          0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                          0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                          0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, // z
                          // And back to the parent message with field v.
                          0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x07};

  for(auto e : expected_a) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}

TEST(NestedMessage, deserialize_one) 
{
  InSequence s;

  ::message_b msg;
  Mocks::ReadBufferMock buffer;

  // Test if a nested message can be deserialized with values set to one.

  uint8_t referee[] = {0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
                       0x12, 0x09, // tag and size of nested a
                       0x08, 0x01, // x
                       0x15, 0x00, 0x00, 0x80, 0x3F, // y
                       0x18, 0x02, // z
                       // And back to the parent message with field v.
                       0x18, 0x01};

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

  EXPECT_EQ(1.0F, msg.get_u());
  EXPECT_EQ(1, msg.get_nested_a().get_x());
  EXPECT_EQ(1.0F, msg.get_nested_a().get_y());
  EXPECT_EQ(1, msg.get_nested_a().get_x());
  EXPECT_EQ(1, msg.get_v());
}

TEST(NestedMessage, deserialize_nested_in_nested_max) 
{
  InSequence s;

  ::message_c msg;
  Mocks::ReadBufferMock buffer;

  // Test if a double nested message can be deserialized with values set to maximum.

  uint8_t referee[] = { 0x0A, 0x27, // tag and size of nested b
                        0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEF, 0x7F, // u
                        0x12, 0x16, // tag and size of nested a
                        0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x07, // x
                        0x15, 0xFF, 0xFF, 0x7F, 0x7F, // y
                        0x18, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, // z
                        // And back to the parent message with field v.
                        0x18, 0xFF, 0xFF, 0xFF, 0xFF, 0x07};

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

  EXPECT_EQ(std::numeric_limits<double>::max(), msg.get_nested_b().get_u());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_nested_b().get_nested_a().get_x());
  EXPECT_EQ(std::numeric_limits<float>::max(), msg.get_nested_b().get_nested_a().get_y());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), msg.get_nested_b().get_nested_a().get_z());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), msg.get_nested_b().get_v());
}

} // End of namespace test_EmbeddedAMS_NestedMessage
