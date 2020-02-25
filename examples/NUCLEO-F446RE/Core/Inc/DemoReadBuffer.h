
#ifndef _DEMO_READ_BUFFER_H_
#define _DEMO_READ_BUFFER_H_

#include <ReadBufferInterface.h>
#include <stdio.h>

namespace demo
{
  class ReadBuffer : public EmbeddedProto::ReadBufferInterface
  {
    public:
      ReadBuffer()
        : index_(0),
		  size_(0),
          bytes_{0U}
      {

      }

      virtual ~ReadBuffer() = default;

      uint32_t get_size() const override
      {
        return index_;
      }

      uint32_t get_max_size() const override
      {
        return size_;
      }

      void set_demo_data(uint8_t* bytes, const uint32_t size)
      {
        bytes_ = bytes;
    	size_ = size;
      }

      bool peak(uint8_t& byte) const override
      {
        bool result = index_ < size_;
        if(result)
        {
          byte = bytes_[index_];
        }
        return result;
      }

      void advance() override
      {
        if(index_ < size_)
        {
          ++index_;
        }
      }

      void advance(const uint32_t N) override
      {
        index_ += N;
        if(index_ >= size_)
        {
          index_ = size_;
        }
      }

      bool pop(uint8_t& byte) override
      {
        bool result = index_ < size_;
        if(result)
        {
          byte = bytes_[index_];
          ++index_;
        }
        return result;
      }

    private:

      uint32_t index_;
      uint32_t size_;
      uint8_t* bytes_;

  };

} // End of namespace DEMO


#endif // _DEMO_READ_BUFFER_H_
