#include "MessageInterface.h"
#include <WireFormatter.h>

namespace EmbeddedProto
{

  bool MessageInterface::serialize(uint32_t field_number, ::EmbeddedProto::WriteBufferInterface& buffer) const
  {
    const uint32_t size_x = this->serialized_size();
    bool result = (size_x < buffer.get_available_size());
    if(result && (0 < size_x))
    {
      uint32_t tag = ::EmbeddedProto::WireFormatter::MakeTag(field_number, ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED);
      result = ::EmbeddedProto::WireFormatter::SerializeVarint(tag, buffer);
      result = result && ::EmbeddedProto::WireFormatter::SerializeVarint(size_x, buffer);
      result = result && this->serialize(buffer);
    }
    return result;
  }

} // End of namespace EmbeddedProto
