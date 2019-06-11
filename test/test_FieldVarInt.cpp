
#include <gtest/gtest.h>

#include <FieldVarInt.h>
#include <Field.h>
#include <MessageBufferMock.h>


using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_FieldVarInt
{

  //! The test fixture class for the following tests.
  class FieldVarIntTest : public ::testing::Test 
  {
    public:
      static constexpr uint8_t N_FIELDS = 4;

      FieldVarIntTest() :
        a(1),
        b(2),
        c(3),
        d(4),
        fields{&a, &b, &c, &d}
      { 

      }

    protected:
      EmbeddedProto::int32 a;
      EmbeddedProto::int64 b;
      EmbeddedProto::uint32 c;
      EmbeddedProto::uint64 d;

      EmbeddedProto::Field* fields[N_FIELDS];
  };

  TEST_F(FieldVarIntTest, set_get)
  {

    // Default value should be zero.
    EXPECT_EQ(0, a.get());

    // Using set
    a.set(-1);
    EXPECT_EQ(-1, a.get());
    
    // Test the other types
    EXPECT_EQ(0, b.get());
    b.set(-1);
    EXPECT_EQ(-1, b.get());

    EXPECT_EQ(0, c.get());
    c.set(1U);
    EXPECT_EQ(1U, c.get());

    EXPECT_EQ(0, d.get());
    d.set(1U);
    EXPECT_EQ(1U, d.get());

  }

  TEST_F(FieldVarIntTest, buffer_to_small)
  {
    a = 100;
    Mocks::MessageBufferMock buffer; 
    // The default mock buffer.get_max_size() will return zero, iow to small to fit.
    EXPECT_EQ(EmbeddedProto::Field::Result::ERROR_BUFFER_TO_SMALL, a.serialize(buffer));

  }

  TEST_F(FieldVarIntTest, serialize_zero)
  {
    // In this test we will test if default values are indeed not serialized.

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    for(auto* field : fields) 
    {
        EXPECT_EQ(EmbeddedProto::Field::Result::OK, field->serialize(buffer));
    }

  }


  TEST_F(FieldVarIntTest, serialize_one)
  {
    // In this test we will test if the value one is serialize correctly for the fixed fields.
    InSequence s;

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    a = 1;
    b = 1;
    c = 1U;
    d = 1U;

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(2));
    EXPECT_CALL(buffer, push(0x08)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(2));
    EXPECT_CALL(buffer, push(0x10)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(2));
    EXPECT_CALL(buffer, push(0x18)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(2));
    EXPECT_CALL(buffer, push(0x20)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.serialize(buffer));


  }

  TEST_F(FieldVarIntTest, serialize_max)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    a = std::numeric_limits<int32_t>::max();
    b = std::numeric_limits<int64_t>::max();
    c = std::numeric_limits<uint32_t>::max();
    d = std::numeric_limits<uint64_t>::max();

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(6));
    EXPECT_CALL(buffer, push(0x08)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x07)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(10));
    EXPECT_CALL(buffer, push(0x10)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(8).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x7F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(6));
    EXPECT_CALL(buffer, push(0x18)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x0F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(11));
    EXPECT_CALL(buffer, push(0x20)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(9).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.serialize(buffer));
  }

  TEST_F(FieldVarIntTest, serialize_min)
  {
    // In this test we will test if the value minimum or lowest is serialize correctly for the fixed 
    // fields.
    InSequence s;

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    a = std::numeric_limits<int32_t>::min();
    b = std::numeric_limits<int64_t>::min();
    c = std::numeric_limits<uint32_t>::min();
    d = std::numeric_limits<uint64_t>::min();

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(11));
    EXPECT_CALL(buffer, push(0x08)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x80)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0xF8)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(4).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(11));
    EXPECT_CALL(buffer, push(0x10)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x80)).Times(9).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));

    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.serialize(buffer));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.serialize(buffer));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.serialize(buffer));
  }


  TEST_F(FieldVarIntTest, deserialize_one)
  {
    InSequence s;

    Mocks::MessageBufferMock buffer;

    ON_CALL(buffer, get_size()).WillByDefault(Return(9));

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));

    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.deserialize(buffer));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.deserialize(buffer));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.deserialize(buffer));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.deserialize(buffer));

    EXPECT_EQ(1, a.get());
    EXPECT_EQ(1, b.get());
    EXPECT_EQ(1U, c.get());
    EXPECT_EQ(1U, d.get());    
  }

  TEST_F(FieldVarIntTest, deserialize_max)
  {
    InSequence s;

    Mocks::MessageBufferMock buffer;

    EXPECT_CALL(buffer, pop(_)).Times(4).WillRepeatedly(DoAll(SetArgReferee<0>(0xFF), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x07), Return(true)));

    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.deserialize(buffer));

    EXPECT_CALL(buffer, pop(_)).Times(8).WillRepeatedly(DoAll(SetArgReferee<0>(0xFF), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x7F), Return(true)));
    
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.deserialize(buffer));

    EXPECT_CALL(buffer, pop(_)).Times(4).WillRepeatedly(DoAll(SetArgReferee<0>(0xFF), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x0F), Return(true)));
    
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.deserialize(buffer));

    EXPECT_CALL(buffer, pop(_)).Times(9).WillRepeatedly(DoAll(SetArgReferee<0>(0xFF), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));

    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.deserialize(buffer));


    EXPECT_EQ(std::numeric_limits<int32_t>::max(), a.get());
    EXPECT_EQ(std::numeric_limits<int64_t>::max(), b.get());
    EXPECT_EQ(std::numeric_limits<uint32_t>::max(), c.get());
    EXPECT_EQ(std::numeric_limits<uint64_t>::max(), d.get());    

/*
    0xFF, 0xFF, 0xFF, 0xFF, 0x07, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F, 
    0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01
*/
  }

} // End of namespace test_EmbeddedAMS_Fields
