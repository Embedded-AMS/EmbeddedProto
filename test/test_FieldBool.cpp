#include <gtest/gtest.h>

#include <FieldBool.h>
#include <Field.h>
#include <MessageBufferMock.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_FieldBool
{
  TEST(FieldBoolTest, set_get)
  {
    EmbeddedProto::boolean a(1);

    // Default value should be false.
    EXPECT_EQ(false, a.get());

    a.set(true);
    EXPECT_EQ(true, a.get());

    a = false;
    EXPECT_EQ(false, a.get());
  }

  TEST(FieldBoolTest, serialize_buffer_to_small)
  {
    EmbeddedProto::boolean a(1);
    a = true;
    Mocks::MessageBufferMock buffer; 
    // The default mock buffer.get_max_size() will return zero, iow to small to fit.
    EXPECT_EQ(EmbeddedProto::Field::Result::ERROR_BUFFER_TO_SMALL, a.serialize(buffer));
  }

  TEST(FieldBoolTest, deserialize_buffer_to_small)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;
        
    EmbeddedProto::boolean a(1);
    Mocks::MessageBufferMock buffer; 
    EXPECT_CALL(buffer, get_size()).WillOnce(Return(0));
    EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_EQ(EmbeddedProto::Field::Result::ERROR_BUFFER_TO_SMALL, a.deserialize(buffer));
  }

  TEST(FieldBoolTest, serialize_false)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::boolean a(1);
    a = false;
    Mocks::MessageBufferMock buffer; 
    ON_CALL(buffer, get_max_size()).WillByDefault(Return(2));
    EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_CALL(buffer, push(_,_)).Times(0);
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));
  }

  TEST(FieldBoolTest, serialize_true)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::boolean a(1);
    a = true;
    Mocks::MessageBufferMock buffer; 
    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(2));
    EXPECT_CALL(buffer, push(0x08)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));
  }

  TEST(FieldBoolTest, deserialize_true)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::boolean a(1);
    Mocks::MessageBufferMock buffer; 
    EXPECT_CALL(buffer, get_size()).WillOnce(Return(1));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.deserialize(buffer));
    EXPECT_EQ(true, a.get());
  }

} // End of namespace test_EmbeddedAMS_FieldBool
