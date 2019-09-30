
#ifndef _DYNAMIC_BUFFER_H_
#define _DYNAMIC_BUFFER_H_

#include <cstring>

namespace EmbeddedProto
{

  //! Class template that specifies the interface of an arry with the data type.
  template<class DATA_TYPE>
  class DynamicArray
  {
    public:

      DynamicArray() = default;
      virtual ~DynamicArray() = default;

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

      //! Get the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The value at the given index. If index is out of bound zero is returned.
      */
      virtual DATA_TYPE operator[](uint32_t index) = 0;

      //! Given a different array of known length copy that data into this object.
      /*!
        \param[in] data A pointer the array to copy from.
        \param[in] length The number of value of DATA_TYPE in the array.
        \return True when copying the data succedded. (Could be false if it does not fit.)
      */
      virtual bool set_data(const DATA_TYPE* data, const uint32_t length) = 0;

  };

  //! A template class that actually holds some data.
  /*!
    This is a seperate class to make it possible to not have the size defined in every function or 
    class using this type of object.
  */
  template<class DATA_TYPE, uint32_t MAX_SIZE>
  class DynamicArraySize : DynamicArray<DATA_TYPE>
  { 
    public:

      DynamicArraySize()
        : current_size_(0)
      {

      }  

      uint32_t get_size() const override { return current_size_; }

      uint32_t get_max_size() const override { return MAX_SIZE; }

      DATA_TYPE* get_data() { return data_; }

      bool set_data(const DATA_TYPE* data, const uint32_t length) override 
      {
        uint32_t size = length * sizeof(DATA_TYPE);
        bool result = MAX_SIZE >= size;
        if(result) 
        {
          current_size_ = size;
          memcpy(data_, data, size);
        }
        return result;
      }

    private:

      //! Number of item in the data array.
      uint32_t current_size_;

      //! The actual data 
      DATA_TYPE data_[MAX_SIZE];
  };

} // End of namespace EmbeddedProto

#endif // End of _DYNAMIC_BUFFER_H_
