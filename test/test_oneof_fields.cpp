
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
  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_xyz());
  msg.set_x(1);
  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(message_oneof::id::X, msg.get_which_xyz());
  msg.clear_x();

  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_xyz());
  msg.set_y(1);
  EXPECT_EQ(1, msg.get_y());
  EXPECT_EQ(message_oneof::id::Y, msg.get_which_xyz());
  msg.clear_y();

  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_xyz());
  msg.set_z(1);
  EXPECT_EQ(1, msg.get_z());
  EXPECT_EQ(message_oneof::id::Z, msg.get_which_xyz());
  msg.clear_z();

  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_xyz());

  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_message());
  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(22);
  msg.mutable_msg_ABC().set_varC(333);
  EXPECT_EQ(message_oneof::id::MSG_ABC, msg.get_which_message());
  EXPECT_EQ(1, msg.msg_ABC().varA());
  EXPECT_EQ(22, msg.msg_ABC().varB());
  EXPECT_EQ(333, msg.msg_ABC().varC());
  msg.clear_msg_ABC();
  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_message());

  msg.set_x(1);
  msg.mutable_msg_ABC().set_varA(1);
  EXPECT_EQ(message_oneof::id::X, msg.get_which_xyz());
  EXPECT_EQ(message_oneof::id::MSG_ABC, msg.get_which_message());
  msg.clear();
  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_xyz());
  EXPECT_EQ(message_oneof::id::NOT_SET, msg.get_which_message());
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

TEST(OneofField, serialize_second_oneof)
{
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  // X and V
  msg.set_a(1);
  msg.set_b(1);
  msg.set_x(1);
  msg.set_v(1);

  uint8_t expected_z[] = {0x08, 0x01,  // a
                          0x50, 0x01,  // b
                          0x28, 0x01,  // x
                          0x85, 0x01, 0x00, 0x00, 0x80, 0x3f}; // v

  for(auto e : expected_z) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}

TEST(OneofField, deserialize) 
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0x08, 0x01,  // a
                       0x50, 0x01,  // b
                       0x30, 0x01}; // y

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(1, msg.get_b());
  EXPECT_EQ(message_oneof::id::Y, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_y());
}

TEST(OneofField, deserialize_second_oneof) 
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0x08, 0x01,  // a
                       0x50, 0x01,  // b
                       0x28, 0x01,  // x
                       0x85, 0x01, 0x00, 0x00, 0x80, 0x3f}; // v

  for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

  EXPECT_EQ(1, msg.get_a());
  EXPECT_EQ(1, msg.get_b());
  EXPECT_EQ(message_oneof::id::X, msg.get_which_xyz());
  EXPECT_EQ(1, msg.get_x());
  EXPECT_EQ(message_oneof::id::V, msg.get_which_uvw());
  EXPECT_EQ(1.0, msg.get_y());
}

TEST(OneofField, serialize_oneof_msg) 
{
  InSequence s;
  message_oneof msg;
  Mocks::WriteBufferMock buffer;

  msg.mutable_msg_ABC().set_varA(1);
  msg.mutable_msg_ABC().set_varB(1);
  msg.mutable_msg_ABC().set_varC(1);

  // When called the buffer will have enough space for the message
  EXPECT_CALL(buffer, get_available_size()).Times(1).WillOnce(Return(99));

  uint8_t expected_ABC[] = {0xa2,       // field ID.
                            0x01, 0x06, // Nested message size.
                            0x08, 0x01, // varA
                            0x10, 0x01, // varB
                            0x18, 0x01};// varC

  for(auto e : expected_ABC) {
    EXPECT_CALL(buffer, push(e)).Times(1).WillOnce(Return(true));
  }

  EXPECT_TRUE(msg.serialize(buffer));
}

TEST(OneofField, deserialize_oneof_msg) 
{
  InSequence s;

  message_oneof msg;
  Mocks::ReadBufferMock buffer;

  uint8_t referee[] = {0xaa, 0x01, 0x07, 0x08, 0x01, 0x10, 0x16, 0x18, 0xcd, 0x02};
    for(auto r: referee) {
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(r), Return(true)));
  }
  EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(Return(false));

  EXPECT_TRUE(msg.deserialize(buffer));

  EXPECT_EQ(message_oneof::id::MSG_DEF, msg.get_which_message());
  EXPECT_EQ(1, msg.get_msg_DEF().get_varD());
  EXPECT_EQ(22, msg.get_msg_DEF().get_varE());
  EXPECT_EQ(333, msg.get_msg_DEF().get_varF());
}