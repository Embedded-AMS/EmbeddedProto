

#ifndef _READ_BUFFER_MOCK_H_
#define _READ_BUFFER_MOCK_H_

#include <gmock/gmock.h>

#include <ReadBufferInterface.h>

namespace Mocks
{

  class ReadBufferMock : public EmbeddedProto::ReadBufferInterface
  {
    public:

      MOCK_CONST_METHOD0(get_size, uint32_t());
      MOCK_CONST_METHOD0(get_max_size, uint32_t());
      
      MOCK_CONST_METHOD1(peak, bool(uint8_t&));
      MOCK_CONST_METHOD0(peak, uint8_t());

      MOCK_METHOD0(advance, void());
      MOCK_METHOD1(advance, void(uint32_t));
      
      MOCK_METHOD1(pop, bool(uint8_t&));
      MOCK_METHOD0(pop, uint8_t());
  };

} // End of namespace Mocks

#endif // End of _READ_BUFFER_MOCK_H_
