

#ifndef _MESSAGE_BUFFER_MOCK_H_
#define _MESSAGE_BUFFER_MOCK_H_

#include <gmock/gmock.h>

#include <MessageBufferInterface.h>

namespace Mocks
{

  class MessageBufferMock : public EmbeddedProto::MessageBufferInterface
  {
    public:

      MOCK_METHOD0(clear, void());

      MOCK_CONST_METHOD0(get_size, uint32_t());
      MOCK_CONST_METHOD0(get_max_size, uint32_t());
      
      MOCK_METHOD1(push, bool(uint8_t));
      MOCK_METHOD2(push, bool(uint8_t*, uint32_t));
      
      MOCK_CONST_METHOD1(peak, bool(uint8_t&));
      MOCK_CONST_METHOD0(peak, uint8_t());

      MOCK_METHOD0(advance, void());
      MOCK_METHOD1(advance, void(uint32_t));
      
      MOCK_METHOD1(pop, bool(uint8_t&));
      MOCK_METHOD0(pop, uint8_t());
  };

} // End of namespace Mocks

#endif // End of _MESSAGE_BUFFER_MOCK_H_
