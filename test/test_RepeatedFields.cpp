
#include "gtest/gtest.h"

#include <WireFormatter.h>
#include <ReadBufferMock.h>
#include <WriteBufferMock.h>

#include <cstdint>    
#include <limits> 

// EAMS message definitions
#include <repeated_fields.h>

namespace test_EmbeddedAMS_RepeatedFields
{

TEST(NestedMessage, construction) 
{
  static constexpr uint32_t Y_SIZE = 3;
  repeated_fields<Y_SIZE> a;

}

} // End of namespace test_EmbeddedAMS_RepeatedFields
