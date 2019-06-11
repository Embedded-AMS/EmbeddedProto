#include <gtest/gtest.h>

#include <FieldEnum.h>
#include <Field.h>
#include <MessageBufferMock.h>

using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_FieldEnum
{
  enum test_enum {
    ZERO  = 0,
    ONE   = 1,
    TWO   = 2,
    ONEHUNDERTTWENTYSEVEN = 127,
    ONEHUNDERTTWENTYEIGHT = 128,
    TWOHUNDERDFIFTYFIVE   = 255,
    TWOBILLION = 2000000000
  };

  enum class test_enum_class : uint32_t {
    I = 0,
    J = 100000, 
    K = 1000000000
  };

  TEST(FieldEnumTest, set_get)
  {
    EmbeddedProto::Enumeration<test_enum> a(1);

    // Default value should be the first item.
    EXPECT_EQ(ZERO, a.get());

    a.set(ONE);
    EXPECT_EQ(ONE, a.get());

    a = TWOHUNDERDFIFTYFIVE;
    EXPECT_EQ(TWOHUNDERDFIFTYFIVE, a.get());


    EmbeddedProto::Enumeration<test_enum_class> b(2);
    EXPECT_EQ(test_enum_class::I, b.get());

    b.set(test_enum_class::J);
    EXPECT_EQ(test_enum_class::J, b.get());

    b = test_enum_class::K;
    EXPECT_EQ(test_enum_class::K, b.get()); 
  }

  TEST(FieldEnumTest, serialize_buffer_to_small)
  {
    EmbeddedProto::Enumeration<test_enum> a(1);
    a = TWO;
    Mocks::MessageBufferMock buffer; 
    // The default mock buffer.get_max_size() will return zero, iow to small to fit.
    EXPECT_EQ(EmbeddedProto::Field::Result::ERROR_BUFFER_TO_SMALL, a.serialize(buffer));  }

  TEST(FieldEnumTest, deserialize_buffer_to_small)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::Enumeration<test_enum> a(1);
    Mocks::MessageBufferMock buffer; 
    EXPECT_CALL(buffer, get_size()).WillOnce(Return(0));
    EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_EQ(EmbeddedProto::Field::Result::ERROR_BUFFER_TO_SMALL, a.deserialize(buffer));
  }

  TEST(FieldEnumTest, serialize_zero) 
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::Enumeration<test_enum> a(1);
    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_CALL(buffer, push(_,_)).Times(0);
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));
  }

  TEST(FieldEnumTest, serialize_one) 
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::Enumeration<test_enum> a(8);
    a = ONE;
    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(2));
    EXPECT_CALL(buffer, push(0x40)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));
  }

  TEST(FieldEnumTest, serialize_two_billion) 
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    EmbeddedProto::Enumeration<test_enum> a(8);
    a = TWOBILLION;
    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(6));
    EXPECT_CALL(buffer, push(0x40)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x80)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xA8)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xD6)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xB9)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x07)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));
  }

  TEST(FieldEnumTest, deserialize_one)
  {
    InSequence s;

    EmbeddedProto::Enumeration<test_enum> a(8); // Use a different id to vary.
    Mocks::MessageBufferMock buffer;

    ON_CALL(buffer, get_size()).WillByDefault(Return(9));

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.deserialize(buffer));

    EXPECT_EQ(ONE, a.get());
  }

  TEST(FieldEnumTest, deserialize_two_billion)
  {
    InSequence s;

    EmbeddedProto::Enumeration<test_enum> a(8); // Use a different id to vary.
    Mocks::MessageBufferMock buffer;

    ON_CALL(buffer, get_size()).WillByDefault(Return(5));

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x80), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0xA8), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0xD6), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0xB9), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x07), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.deserialize(buffer));

    EXPECT_EQ(TWOBILLION, a.get());
  }

} // End of namespace test_EmbeddedAMS_FieldEnum
