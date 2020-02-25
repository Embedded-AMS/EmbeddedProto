
#ifndef _DEMO_WRITE_BUFFER_H_
#define _DEMO_WRITE_BUFFER_H_

#include <WriteBufferInterface.h>
#include <stdio.h>

namespace demo
{
	template<uint32_t BUFFER_SIZE>
	class WriteBuffer : public EmbeddedProto::WriteBufferInterface
	{
	  public:
      WriteBuffer()
    		: bytes_used_(0),
      		bytes_{0U}
      {

      }

      virtual ~WriteBuffer() = default;

      void clear() override
      {
    		bytes_used_ = 0;
  	  }

      uint32_t get_size() const override
      {
      	return bytes_used_;
      }

      uint32_t get_max_size() const override
      {
      	return BUFFER_SIZE;
      }

      uint32_t get_available_size() const override
      {
      	return  - bytes_used_;
      }

      bool push(const uint8_t byte) override
      {
      	bool result = BUFFER_SIZE > bytes_used_;
      	if(result)
      	{
      		bytes_[bytes_used_] = byte;
      		++bytes_used_;
      	}
      	return result;
      }

      bool push(const uint8_t* bytes, const uint32_t length) override
      {
      	bool result = BUFFER_SIZE >= (bytes_used_ + length);
      	if(result)
      	{
      		memcpy(bytes_ + bytes_used_, bytes, length);
      		bytes_used_ += length;
      	}
      	return result;
      }

	  private:

	  	uint32_t bytes_used_;
	  	uint8_t bytes_[BUFFER_SIZE];

	};

} // End of namespace DEMO


#endif // _DEMO_WRITE_BUFFER_H_
