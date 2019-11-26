
#ifndef _READ_BUFFER_SECTION_H_
#define _READ_BUFFER_SECTION_H_

#include <cstdint>

#include "ReadBufferInterface.h"

namespace EmbeddedProto 
{
  //! This is a wrapper around a ReadBufferInterface only exposing a given number of bytes.
  /*!
      This class is used when decoding a length delimited fields. It is constructed given a message
      buffer and a size. This class will return bytes from the buffer for the given number of bytes
      givne in the size parameter.

      \see ReadBufferInterface
  */
  class ReadBufferSection : public ReadBufferInterface
  {
    public:

      //! Explicitly delete the default constructor in favour of the one with parameters.
      ReadBufferSection() = delete;

      //! The constructor of the class with the required parameters
      /*!
        \param buffer The actual data buffer from which the bytes are obtained.
        \param size The maximum number of bytes to return from buffer.
      */
      ReadBufferSection(ReadBufferInterface& buffer, const uint32_t size);

      virtual ~ReadBufferSection() = default;


      //! Return the number of bytes remaining.
      uint32_t get_size() const override;

      //! Obtain the total number of bytes which can at most be stored in the buffer.
      /*!
        In the case of this buffer section this will return the size of the section.
      */
      uint32_t get_max_size() const override;

      //! Expose the function of the parent buffer.
      /*!
        This will not do anything if size zero is reached.
      */
      bool peak(uint8_t& byte) const override;

      //! Decrement the size and call advance on the parent buffer.
      /*!
        This will not do anything if size zero is reached.
      */
      void advance() override;

      //! Decrement the size by N bytes and call advance on the parent buffer.
      /*!
        This will not do anything if size zero is reached.
      */
      void advance(const uint32_t N) override;

      //! Decrement the size and pop the next byte from the parent buffer.
      /*!
        This will not do anything if size zero is reached.
      */
      bool pop(uint8_t& byte) override;

    private:

      //! A reference to the buffer containing the actual data.
      ReadBufferInterface& buffer_;

      //! The number of bytes left in this section.
      uint32_t size_;

      //! The total number of bytes masked of by this section.
      const uint32_t max_size_;
  };

} // End of namespace EmbeddedProto

#endif // _READ_BUFFER_SECTION_H_
