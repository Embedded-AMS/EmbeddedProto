
#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <repeated_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;

namespace test_EmbeddedAMS_RepeatedFieldMessage
{

static constexpr uint32_t Y_SIZE = 3;

TEST(RepeatedFieldMessage, construction) 
{
  repeated_fields<Y_SIZE> msg;
}

TEST(RepeatedFieldMessage, serialize_array_one)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.add_y(1);
  msg.add_y(1);
  msg.add_y(1);

  uint8_t expected_uv[] = {0x12, 0x03, 0x01, 0x01, 0x01}; // y

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(5));

  for(auto e : expected_uv) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}



TEST(RepeatedFieldMessage, serialize_array_max)
{
  InSequence s;
  Mocks::WriteBufferMock buffer;

  repeated_fields<Y_SIZE> msg;
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());
  msg.add_y(std::numeric_limits<uint32_t>::max());

  uint8_t expected_uv[] = {0x12, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f, 0xff, 0xff, 0xff, 0xff, 0x0f}; // y

  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(17));

  for(auto e : expected_uv) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}

} // End of namespace test_EmbeddedAMS_RepeatedFieldMessage
