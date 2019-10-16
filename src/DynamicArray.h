
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

      //! Get a refernce to the value at the given index. 
      /*!
        \param[in] index The desired index to return.
        \return The reference to the value at the given index.
      */
      virtual DATA_TYPE& get(uint32_t index) = 0;

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
  };

  //! A template class that actually holds some data.
  /*!
    This is a seperate class to make it possible to not have the size defined in every function or 
    class using this type of object.
  */
  template<class DATA_TYPE, uint32_t MAX_SIZE>
  class DynamicArraySize : DynamicArray<DATA_TYPE>
  { 
      static constexpr uint32_t BYTES_PER_ELEMENT = sizeof(DATA_TYPE);

    public:

      DynamicArraySize()
        : current_size_(0)
      {

      }  

      uint32_t get_size() const override { return BYTES_PER_ELEMENT * current_size_; }

      uint32_t get_max_size() const override { return BYTES_PER_ELEMENT * MAX_SIZE; }

      uint32_t get_length() const override { return current_size_; }

      uint32_t get_max_length() const override { return MAX_SIZE; }

      DATA_TYPE* get_data() { return data_; }

      DATA_TYPE& get(uint32_t index) override { return data_[index]; }

      void set(uint32_t index, const DATA_TYPE& value) override { data_[index] = value; }

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
        constexpr uint32_t size = MAX_SIZE * BYTES_PER_ELEMENT;
        current_size_ = 0;
        memset(data_, 0, size);
      }

    private:

      //! Number of item in the data array.
      uint32_t current_size_;

      //! The actual data 
      DATA_TYPE data_[MAX_SIZE];
  };

} // End of namespace EmbeddedProto

#endif // End of _DYNAMIC_BUFFER_H_
