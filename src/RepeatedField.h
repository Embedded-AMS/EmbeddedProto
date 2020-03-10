/*
 *  Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation, version 3 of the license.
 *
 *  Embedded Proto  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
 *
 *  For commercial and closed source application please vist:
 *  <https://EmbeddedProto.com/license/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal adress:
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#ifndef _DYNAMIC_BUFFER_H_
#define _DYNAMIC_BUFFER_H_

#include <cstring>
#include <algorithm>    // std::min
#include <type_traits>

#include "Fields.h"
#include "MessageInterface.h"
#include "MessageSizeCalculator.h"
#include "ReadBufferSection.h" 

namespace EmbeddedProto
{

  //! Class template that specifies the interface of an arry with the data type.
  template<class DATA_TYPE>
  class RepeatedField : public Field
  {
    static_assert(std::is_base_of<::EmbeddedProto::Field, DATA_TYPE>::value, "A Field can only be used as template paramter.");

    //! Check how this field shoeld be serialized, packed or not.
    static constexpr bool PACKED = !std::is_base_of<MessageInterface, DATA_TYPE>::value;

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

      bool serialize(WriteBufferInterface& buffer) const final
      {
        return false;
      }

      bool serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer) const final
      {
        bool result = true;

        if(PACKED)
        {
          const uint32_t size_x = this->serialized_size_packed(field_number);
          result = (size_x <= buffer.get_available_size());

          // Use the packed way of serialization for base fields.
          if(result && (0 < size_x))
          {          
            uint32_t tag = ::EmbeddedProto::WireFormatter::MakeTag(field_number, 
                                        ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED);
            result = ::EmbeddedProto::WireFormatter::SerializeVarint(tag, buffer);
            result = result && ::EmbeddedProto::WireFormatter::SerializeVarint(size_x, buffer);
            result = result && serialize_packed(buffer);
          }
        }
        else 
        {
          const uint32_t size_x = this->serialized_size_unpacked(field_number);
          result = (size_x <= buffer.get_available_size());
          result = result && serialize_unpacked(field_number, buffer);
        }

        return result;
      }

      //! Function to deserialize this array.
      /*!
          From a buffer of data fill this array with data.
          \param buffer [in]  The memory from which the message is obtained.
          \return True when every was successfull. 
      */
      bool deserialize(::EmbeddedProto::ReadBufferInterface& buffer) final
      {
        bool result = true;
        if(PACKED)
        {              
          uint32_t size;
          result = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, size);
          ::EmbeddedProto::ReadBufferSection bufferSection(buffer, size);
          DATA_TYPE x;
          while(result && x.deserialize(bufferSection)) 
          {
            result = this->add(x);
          }
        }
        else 
        {
          uint32_t size;
          result = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, size);
          ::EmbeddedProto::ReadBufferSection bufferSection(buffer, size);
          DATA_TYPE x;
          if(result && x.deserialize(bufferSection))
          {
            result = this->add(x);
          }
        }
        return result;
      }

      //! Calculate the size of this message when serialized.
      /*!
          \return The number of bytes this message will require once serialized.
      */
      uint32_t serialized_size_packed(int32_t field_number) const 
      {
        ::EmbeddedProto::MessageSizeCalculator calcBuffer;
        serialize_packed(calcBuffer);
        return calcBuffer.get_size();
      }

      //! Calculate the size of this message when serialized.
      /*!
          \return The number of bytes this message will require once serialized.
      */
      uint32_t serialized_size_unpacked(int32_t field_number) const 
      {
        ::EmbeddedProto::MessageSizeCalculator calcBuffer;
        serialize_unpacked(field_number, calcBuffer);
        return calcBuffer.get_size();
      }

    private:

      bool serialize_packed(WriteBufferInterface& buffer) const
      {
        bool result = true;
        for(uint32_t i = 0; (i < this->get_length()) && result; ++i)
        {
          result = this->get(i).serialize(buffer);
        }
        return result;
      }

      bool serialize_unpacked(uint32_t field_number, WriteBufferInterface& buffer) const
      {
        bool result = true;
        for(uint32_t i = 0; (i < this->get_length()) && result; ++i)
        {
          const uint32_t size_x = this->get(i).serialized_size();
          uint32_t tag = ::EmbeddedProto::WireFormatter::MakeTag(field_number, 
                                    ::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED);
          result = ::EmbeddedProto::WireFormatter::SerializeVarint(tag, buffer);
          result = result && ::EmbeddedProto::WireFormatter::SerializeVarint(size_x, buffer);
          if(result && (0 < size_x)) 
          {
            result = this->get(i).serialize(buffer);
          }
        }
        return result;
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

/*
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
*/
} // End of namespace EmbeddedProto

#endif // End of _DYNAMIC_BUFFER_H_
