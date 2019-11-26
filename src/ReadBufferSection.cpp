
#include "ReadBufferSection.h"

namespace EmbeddedProto 
{

  ReadBufferSection::ReadBufferSection(ReadBufferInterface& buffer, const uint32_t size)
    : buffer_(buffer),
      size_(size),
      max_size_(size)
  {

  }

  uint32_t ReadBufferSection::get_size() const
  {
    return size_;
  }

  uint32_t ReadBufferSection::get_max_size() const
  {
    return max_size_;
  }

  bool ReadBufferSection::peak(uint8_t& byte) const
  {
    bool result = 0 < size_;
    if(result)
    {
      result = buffer_.peak(byte);
    }
    return result;
  }

  void ReadBufferSection::advance()
  {
    if(0 < size_) 
    {
      buffer_.advance();
    }
  }

  void ReadBufferSection::advance(const uint32_t N)
  {
    if(0 < size_) 
    {
      uint32_t n = (N <= size_) ? N : size_;
      buffer_.advance(n);
      size_ -= n;
    }
  }

  bool ReadBufferSection::pop(uint8_t& byte)
  {
    bool result = 0 < size_;
    if(result)
    {
      result = buffer_.pop(byte);
      --size_;
    }
    return result;
  }

} // End of namespace EmbeddedProto
