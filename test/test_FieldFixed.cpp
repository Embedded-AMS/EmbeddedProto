
#include <gtest/gtest.h>

#include <FieldFixed.h>
#include <Field.h>
#include <MessageBufferMock.h>


using ::testing::_;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

namespace test_EmbeddedAMS_FieldFixed
{

  //! The test fixture class for the following tests.
  class FieldFixedTest : public ::testing::Test 
  {
    public:
      static constexpr uint8_t N_FIELDS = 6;

      FieldFixedTest() :
        a(1),
        b(2),
        c(3),
        d(4),
        e(5),
        f(6),
        fields{&a, &b, &c, &d, &e, &f}
      { 

      }

    protected:
      EmbeddedProto::fixed64 a;
      EmbeddedProto::sfixed64 b;
      EmbeddedProto::double64 c;

      EmbeddedProto::fixed32 d;
      EmbeddedProto::sfixed32 e;
      EmbeddedProto::float32 f;

      EmbeddedProto::Field* fields[N_FIELDS];
  };

  TEST_F(FieldFixedTest, set_get)
  {
    constexpr double  RANDOM_DOUBLE = 1682.78324;
    constexpr float   RANDOM_FLOAT = 2472.25241F;

    // Default value should be zero.
    EXPECT_EQ(0, a.get());

    // Using set
    a.set(1U);
    EXPECT_EQ(1U, a.get());
    
    // Test the other types
    EXPECT_EQ(0, b.get());
    b.set(-1);
    EXPECT_EQ(-1, b.get());

    EXPECT_EQ(0.0, c.get());
    c.set(RANDOM_DOUBLE);
    EXPECT_EQ(RANDOM_DOUBLE, c.get());

    EXPECT_EQ(0, d.get());
    d.set(1U);
    EXPECT_EQ(1U, d.get());

    EXPECT_EQ(0, e.get());
    e.set(-1);
    EXPECT_EQ(-1, e.get());

    EXPECT_EQ(0.0F, f.get());
    f.set(RANDOM_FLOAT);
    EXPECT_EQ(RANDOM_FLOAT, f.get());

  }

  TEST_F(FieldFixedTest, buffer_to_small)
  {
    a = 100;
    Mocks::MessageBufferMock buffer; 
    // The default mock buffer.get_max_size() will return zero, iow to small to fit.
    EXPECT_EQ(EmbeddedProto::Field::Result::ERROR_BUFFER_TO_SMALL, a.serialize(buffer));

  }

  TEST_F(FieldFixedTest, serialize_zero)
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


  TEST_F(FieldFixedTest, serialize_one)
  {
    // In this test we will test if the value one is serialize correctly for the fixed fields.
    InSequence s;

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    a = 1U;
    b = 1;
    c = 1.0;

    d = 1U;
    e = 1;
    f = 1.0F;

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x09)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(7).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x11)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(7).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x19)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(6).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0xF0)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x3F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x25)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(3).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x2D)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x01)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(3).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, e.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x35)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x80)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x3F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, f.serialize(buffer));

  }

  TEST_F(FieldFixedTest, serialize_max)
  {
    // In this test we will test if the maximum is serialize correctly for the fixed fields.
    InSequence s;

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    a = std::numeric_limits<uint64_t>::max();
    b = std::numeric_limits<int64_t>::max();
    c = std::numeric_limits<double>::max();

    d = std::numeric_limits<uint32_t>::max();
    e = std::numeric_limits<int32_t>::max();
    f = std::numeric_limits<float>::max();

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x09)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(8).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x11)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(7).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x7F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x19)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(6).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0xEF)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x7F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x25)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(4).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x2D)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x7F)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, e.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x35)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x7F)).Times(2).WillRepeatedly(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, f.serialize(buffer));

  }

  TEST_F(FieldFixedTest, serialize_min)
  {
    // In this test we will test if the value minimum or lowest is serialize correctly for the fixed 
    // fields.
    InSequence s;

    Mocks::MessageBufferMock buffer;
    EXPECT_CALL(buffer, push(_,_)).Times(0);

    a = std::numeric_limits<uint64_t>::min();
    b = std::numeric_limits<int64_t>::min();
    c = std::numeric_limits<double>::lowest();

    d = std::numeric_limits<uint32_t>::min();
    e = std::numeric_limits<int32_t>::min();
    f = std::numeric_limits<float>::lowest();

    //EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x11)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(7).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x80)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(9));
    EXPECT_CALL(buffer, push(0x19)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(6).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0xEF)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.serialize(buffer));

    //EXPECT_CALL(buffer, push(_)).Times(0);
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x2D)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0x00)).Times(3).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x80)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, e.serialize(buffer));

    EXPECT_CALL(buffer, get_max_size()).WillOnce(Return(5));
    EXPECT_CALL(buffer, push(0x35)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(buffer, push(0x7F)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(buffer, push(0xFF)).Times(1).WillOnce(Return(true));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, f.serialize(buffer));

  }


  TEST_F(FieldFixedTest, deserialize_one)
  {
    InSequence s;

    Mocks::MessageBufferMock buffer;

    ON_CALL(buffer, get_size()).WillByDefault(Return(9));

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(7).WillRepeatedly(DoAll(SetArgReferee<0>(0x00), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, a.deserialize(buffer));
    EXPECT_EQ(1U, a.get());

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(7).WillRepeatedly(DoAll(SetArgReferee<0>(0x00), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, b.deserialize(buffer));
    EXPECT_EQ(1, b.get());

    EXPECT_CALL(buffer, pop(_)).Times(6).WillRepeatedly(DoAll(SetArgReferee<0>(0x00), Return(true))); 
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0xF0), Return(true)));   
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x3F), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, c.deserialize(buffer));
    EXPECT_EQ(1.0, c.get());

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(3).WillRepeatedly(DoAll(SetArgReferee<0>(0x00), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, d.deserialize(buffer));
    EXPECT_EQ(1U, d.get());

    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x01), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(3).WillRepeatedly(DoAll(SetArgReferee<0>(0x00), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, e.deserialize(buffer));
    EXPECT_EQ(1, e.get());

    EXPECT_CALL(buffer, pop(_)).Times(2).WillRepeatedly(DoAll(SetArgReferee<0>(0x00), Return(true)));
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x80), Return(true)));    
    EXPECT_CALL(buffer, pop(_)).Times(1).WillOnce(DoAll(SetArgReferee<0>(0x3F), Return(true)));
    EXPECT_EQ(EmbeddedProto::Field::Result::OK, f.deserialize(buffer));
    EXPECT_EQ(1.0F, f.get());

    /* 
      0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
      0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 

      0x25, 0x01, 0x00, 0x00, 0x00, 
      0x2D, 0x01, 0x00, 0x00, 0x00, 
      0x35, 0x00, 0x00, 0x80, 0x3F 
    */
  }


} // End of namespace test_EmbeddedAMS_Fields
