
#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <oneof_fields.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;


TEST(OneofField, construction) 
{
  message_oneof msg;
}

TEST(OneofField, serialize_zero) 
{
  message_oneof msg;
  Mocks::WriteBufferMock buffer;
  
  EXPECT_CALL(buffer, push(_)).Times(0);
  EXPECT_CALL(buffer, push(_,_)).Times(0);
  EXPECT_CALL(buffer, get_available_size()).Times(0);

  EXPECT_TRUE(msg.serialize(buffer));
}

TEST(OneofField, set_get_clear)
{
  message_oneof msg;
  EXPECT_EQ(0, msg.get_which_xyz());
  msg.set_x(1);
  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(5, msg.get_which_xyz());
  msg.clear_x();

  EXPECT_EQ(0, msg.get_which_xyz());
  msg.set_y(1);
  EXPECT_EQ(1, msg.get_y());
  EXPECT_EQ(6, msg.get_which_xyz());
  msg.clear_y();

  EXPECT_EQ(0, msg.get_which_xyz());
  msg.set_z(1);
  EXPECT_EQ(1, msg.get_z());
  EXPECT_EQ(7, msg.get_which_xyz());
  msg.clear_z();
}

TEST(OneofField, serialize_ones) 
{
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  // X
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  
  uint8_t expected_x[] = {0x08, 0x01,  // a
                          0x50, 0x01,  // b
                          0x28, 0x01}; // x

  for(auto e : expected_x) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));

  // Y
  msg.set_y(1);
  uint8_t expected_y[] = {0x08, 0x01,  // a
                          0x50, 0x01,  // b
                          0x30, 0x01}; // y

  for(auto e : expected_y) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));

  // z
  msg.set_z(1);
  uint8_t expected_z[] = {0x08, 0x01,  // a
                          0x50, 0x01,  // b
                          0x38, 0x01}; // z

  for(auto e : expected_z) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}
