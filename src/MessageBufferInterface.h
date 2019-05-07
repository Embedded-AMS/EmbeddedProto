
#ifndef _MESSAGE_BUFFER_INTERFACE_H_
#define _MESSAGE_BUFFER_INTERFACE_H_

namespace EmbeddedProto 
{
  //! The pure virtual definition of a message buffer.
  /*!
      This interface is to be used by classes wishing to use the buffer. An actual implementation
      is made specific to how you would like to store data.

      The buffer deals with bytes (uint8_t) only.
  */
  class MessageBufferInterface
  {
    public:

      MessageBufferInterface() = default;
      virtual ~MessageBufferInterface() = default;

      //! Delete all data in the buffer.
      virtual void clear() = 0;

      //! Obtain the total number of bytes currently stored in the buffer.
      virtual uint32_t get_size() const = 0;

      //! Obtain the total number of bytes which can at most be stored in the buffer.
      virtual uint32_t get_max_size() const = 0;

      //! Push a single byte into the buffer.
      /*!
          \param[in] byte The data to append after previously added data in the buffer.
          \return True when there was space to add the byte.
      */
      virtual bool push(const uint8_t byte) = 0;

      //! Push an array of bytes into the buffer.
      /*!
          The given array will be appended after already addded data in the buffer.
          \param[in] bytes Pointer to the array of bytes.
          \param[in] length The number of bytes in the array.
          \return True when there was space to add the bytes.
      */
      virtual bool push(const uint8_t* bytes, const uint32_t length) = 0;

      //! Obtain the value of the oldest byte in the buffer.
      /*!
          This function will not alter the buffer read index.
          
          The parameter byte will not be set if the buffer was empty.

          \param[out] byte When the buffer is not empty this variable will hold the oldest value.
          \return True when the buffer was not empty.
      */
      virtual bool peak(uint8_t& byte) const = 0;

      //! Returns the oldest byte in the buffer.
      /*! 
          This function will not alter the buffer read index.
          \warning When the buffer is empty the value returned is undefined.
      */
      virtual uint8_t peak() const = 0;

      //! Advances the internal read index by one when the buffer is not empty.
      virtual void advance() = 0;

      //! Advances the internal read index by the given value.
      /*!
          The advance is limited to the number of bytes in the buffer.
          \param[in] N The number of bytes to advance the read index.
      */
      virtual void advance(const uint32_t N) = 0;

      //! Obtain the value of the oldest byte in the buffer and remove it from the buffer.
      /*!
          This function will alter the internal read index.
          
          The parameter byte will not be set if the buffer was empty.

          \param[out] byte When the buffer is not empty this variable will hold the oldest value.
          \return True when the buffer was not empty.
      */
      virtual bool pop(uint8_t& byte) const = 0;

      //! Returns the oldest byte in the buffer and removes it from the buffer.
      /*!
          This function will alter the internal read index.
          \warning When the buffer is empty the value returned is undefined.
      */
      virtual uint8_t pop() = 0;

  };

} // End of namespace EmbeddedProto

#endif // End of _MESSAGE_BUFFER_INTERFACE_H_
