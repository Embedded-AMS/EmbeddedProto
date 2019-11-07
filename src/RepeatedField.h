
#ifndef _DYNAMIC_BUFFER_H_
#define _DYNAMIC_BUFFER_H_

#include <cstring>
#include <algorithm>    // std::min

#include <Fields.h>
#include <MessageSizeCalculator.h>

namespace EmbeddedProto
{

  //! Class template that specifies the interface of an arry with the data type.
  template<class DATA_TYPE>
  class RepeatedField
  {
    public:

      RepeatedField() = default;
      virtual ~RepeatedField() = default;

      //! Obtain the total number of bytes currently stored in the array.
      virtual uint32_t get_size() const = 0;

      //! Obtain the maximum number of bytes which can at most be stored in the array.
      virtual uint32_t get_max_size() const = 0;

      //! Obtain the total number of DATA_TYPE items in the array.
      virtual uint32_t get_length() const = 0;

      //! Obtain the maximum number of DATA_TYPE items which can at most be stored in the array.
      virtual uint32_t get_max_length() const = 0;

      //! Get a pointer to the first element in the array.
      virtual DATA_TYPE* get_data() = 0;

      //! Get a refernce to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The reference to the value at the given index.
      */
      virtual DATA_TYPE& get(uint32_t index) = 0;

      //! Get a constatnt refernce to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The constant reference to the value at the given index.
      */
      virtual const DATA_TYPE& get(uint32_t index) const = 0;

      //! Get a refernce to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The reference to the value at the given index.
      */
      DATA_TYPE& operator[](uint32_t index) { return this->get(index); }

      //! Get a refernce to the value at the given index. But constant. 
      /*!
        \param[in] index The desired index to return.
        \return The constant reference to the value at the given index.
      */
      const DATA_TYPE& operator[](uint32_t index) const { return this->get(index); }

      //! Set the value at the given index.
      /*!
        \param[in] index The desired index to change.
        \param[in] value The value we would like to set.
      */
      virtual void set(uint32_t index, const DATA_TYPE& value) = 0;

      //! Given a different array of known length copy that data into this object.
      /*!
        \param[in] data A pointer the array to copy from.
        \param[in] length The number of value of DATA_TYPE in the array.
        \return True when copying the data succedded. (Could be false if it does not fit.)
      */
      virtual bool set_data(const DATA_TYPE* data, const uint32_t length) = 0;

      //! Append a value to the end of the array.
      /*!
        \param[in] value The data to add.
        \return False if there was no space to add the value.
      */
      virtual bool add(const DATA_TYPE& value) = 0;

      //! Remove all data in the array and set it to the default value.
      virtual void clear() = 0;

      //! Function to serialize this array.
      /*!
          The data this array holds will be serialized into the buffer.
          \param buffer [in]  The memory in which the serialized array is stored.
          \return True when every was successfull. 
      */
      bool serialize(::EmbeddedProto::WriteBufferInterface& buffer) const
      {
        bool result = true;
        for(uint32_t i = 0; (i < this->get_length()) && result; ++i)
        {
          result = ::EmbeddedProto::serialize(this->get(i), buffer);
        }
        return result;
      }

      //! Function to deserialize this array.
      /*!
          From a buffer of data fill this array with data.
          \param buffer [in]  The memory from which the message is obtained.
          \return True when every was successfull. 
      */
      bool deserialize(::EmbeddedProto::ReadBufferInterface& buffer)
      {
        this->clear();
        DATA_TYPE x;
        bool result = true;
        while(result && ::EmbeddedProto::deserialize(buffer, x)) 
        {
          result = this->add(x);
        }
        return result;
      }

      //! Calculate the size of this message when serialized.
      /*!
          \return The number of bytes this message will require once serialized.
      */
      uint32_t serialized_size() const 
      {
        ::EmbeddedProto::MessageSizeCalculator calcBuffer;
        this->serialize(calcBuffer);
        return calcBuffer.get_size();
      }

  };

  //! A template class that actually holds some data.
  /*!
    This is a seperate class to make it possible to not have the size defined in every function or 
    class using this type of object.
  */
  template<class DATA_TYPE, uint32_t MAX_SIZE>
  class RepeatedFieldSize : public RepeatedField<DATA_TYPE>
  { 
      static constexpr uint32_t BYTES_PER_ELEMENT = sizeof(DATA_TYPE);

    public:

      RepeatedFieldSize()
        : current_size_(0),
          data_{}
      {

      }  

      ~RepeatedFieldSize() override = default;

      uint32_t get_size() const override { return BYTES_PER_ELEMENT * current_size_; }

      uint32_t get_max_size() const override { return BYTES_PER_ELEMENT * MAX_SIZE; }

      uint32_t get_length() const override { return current_size_; }

      uint32_t get_max_length() const override { return MAX_SIZE; }

      DATA_TYPE* get_data() { return data_; }

      DATA_TYPE& get(uint32_t index) override { return data_[index]; }
      const DATA_TYPE& get(uint32_t index) const override { return data_[index]; }

      void set(uint32_t index, const DATA_TYPE& value) override 
      { 
        data_[index] = value;
        current_size_ = std::max(index+1, current_size_); 
      }

      bool set_data(const DATA_TYPE* data, const uint32_t length) override 
      {
        const uint32_t size = length * BYTES_PER_ELEMENT;
        const bool result = MAX_SIZE >= size;
        if(result) 
        {
          current_size_ = size;
          memcpy(data_, data, size);
        }
        return result;
      }

      bool add(const DATA_TYPE& value) override 
      {
        const bool result = MAX_SIZE >= current_size_;
        if(result) 
        {
          data_[current_size_] = value;
          ++current_size_;
        }
        return result;
      }

      void clear() override 
      {
        current_size_ = 0;
      }

    private:

      //! Number of item in the data array.
      uint32_t current_size_;

      //! The actual data 
      DATA_TYPE data_[MAX_SIZE];
  };

  template<class DATA_TYPE>
  bool serialize(uint32_t field_number, const RepeatedField<DATA_TYPE>& x, WriteBufferInterface& buffer)
  {
    const uint32_t size_x = x.serialized_size();
    bool result = (size_x < buffer.get_available_size());
    if(result && (0 < size_x))
    {
      uint32_t tag = ::EmbeddedProto::WireFormatter::MakeTag(field_number, ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED);
      result = ::EmbeddedProto::WireFormatter::SerializeVarint(tag, buffer);
      result = result && ::EmbeddedProto::WireFormatter::SerializeVarint(size_x, buffer);
      result = result && x.serialize(buffer);
    }
    return result;
  }

  template<class DATA_TYPE>
  bool serialize(const RepeatedField<DATA_TYPE>& x, WriteBufferInterface& buffer)
  {
    const uint32_t size_x = x.serialized_size();
    bool result = (size_x < buffer.get_available_size());
    if(result && (0 < size_x))
    {
      result = ::EmbeddedProto::WireFormatter::SerializeVarint(size_x, buffer);
      result = result && x.serialize(buffer);
    }
    return result;
  }

  template<class DATA_TYPE>
  inline bool deserialize(ReadBufferInterface& buffer, RepeatedField<DATA_TYPE>& x)
  {
      return x.deserialize(buffer);
  }

} // End of namespace EmbeddedProto

#endif // End of _DYNAMIC_BUFFER_H_
