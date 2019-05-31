
#ifndef _FIELD_BOOL_H_
#define _FIELD_BOOL_H_

#include "Field.h"

namespace EmbeddedProto
{

  namespace Detail 
  {

    //! The field type for booleans.
    class FieldBool : public Field 
    {
      private:        
          //! The Protobuf default value for this type. Zero in this case.
          static constexpr bool DEFAULT_VALUE = false;

          //! The number of bytes required to serialize one boolean.
          static constexpr uint32_t N_BYTES_IN_BOOL = 1;

          //! The byte as serialized for a bool equal to true.
          static constexpr uint8_t BOOL_BYTE_TRUE = 0x01;

      public:

          //! Constructor which also sets the field id number of this field.
          /*!
              \param[in] The field id number as specified in the *.proto file.
          */
          FieldBool(const uint32_t number) :
              Field(WireType::VARINT, number),
              _data(DEFAULT_VALUE)
          {

          }

          //! The constructor is default as we do not have members with non standart memory allocation.
          ~FieldBool() final = default;

          //! Set the value of this field
          void set(const bool& value)
          {
            _data = value;
          }

          //! Assignment opertor to set the data.
          FieldBool& operator=(const bool& value)
          {
            set(value);
            return *this;
          }

          //! Obtain the value of this field.
          const bool& get() const
          {
            return _data; 
          }

          //! \see Field::serialize()
          Result serialize(MessageBufferInterface& buffer) const final
          {
            Result result(Result::OK);
            // Only serialize if the data does not equal the default and when the size in the buffer 
            // is sufficient.
            if(DEFAULT_VALUE != _data)
            {
              if(serialized_size() <= buffer.get_max_size()) 
              {
                _serialize_varint(tag(), buffer);
                buffer.push(BOOL_BYTE_TRUE);
              }
              else 
              {
                result = Result::ERROR_BUFFER_TO_SMALL;
              }
            }
            return result;
          }

          //! \see Field::deserialize()
          Result deserialize(MessageBufferInterface& buffer) final
          {
            // Check if there is enough data in the buffer for a fixed value.
            bool result = N_BYTES_IN_BOOL <= buffer.get_size();
            if(result) {
              uint8_t byte = 0;
              buffer.pop(byte);
              _data = BOOL_BYTE_TRUE == byte;
            }

            return result ? Result::OK : Result::ERROR_BUFFER_TO_SMALL;
          }

          //! \see Field::clear()
          void clear() final
          {
            _data = DEFAULT_VALUE;
          }

          //! \see Field::serialized_data_size()
          uint32_t serialized_data_size() const final
          {
            return N_BYTES_IN_BOOL;
          }


      private:

          //! The actual data.
          bool _data;
    };

  } // End of namespace detail

  //! A short hand definition of the boolean type.
  typedef Detail::FieldBool boolean;

} // End of namespace EmbeddedProto

#endif // End of _FIELD_BOOL_H_
