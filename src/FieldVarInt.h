
#ifndef _FIELD_VARINT_H_
#define _FIELD_VARINT_H_

#include "Field.h"

namespace EmbeddedProto
{

  namespace Detail 
  {
    //! The field type with varint encoding.
    /*!
        The variables encoded with this field are: int32, int64, uint32, uint64, sint32, sint64, 
        bool, enum.
    */
    template<class DATA_TYPE>
    class FieldVarInt : public Field
    {
      protected:

        //! The Protobuf default value for this type. Zero in this case.
        static constexpr DATA_TYPE DEFAULT_VALUE = static_cast<DATA_TYPE>(0);

        //! This typedef will return an unsigned variable depending on the data type.
        typedef typename std::make_unsigned<DATA_TYPE>::type VAR_UINT_TYPE;

      public:

        //! Constructor which also sets the field id number of this field.
        /*!
            \param[in] The field id number as specified in the *.proto file.
        */
        FieldVarInt(const uint32_t number) :
            Field(WireType::VARINT, number),
            _data(DEFAULT_VALUE)
        {

        }

        //! The constructor is default as we do not have members with non standart memory allocation.
        ~FieldVarInt() final = default;

        //! Set the value of this field
        void set(const DATA_TYPE& value)
        {
          _data = value;
        }

        //! Assignment opertor to set the data.
        FieldVarInt& operator=(const DATA_TYPE& value)
        {
          set(value);
          return *this;
        }

        //! Obtain the value of this field.
        const DATA_TYPE& get() const
        {
          return _data; 
        }

        //! \see Field::serialize()
        Result serialize(MessageBufferInterface& buffer) const final
        {
          Result result(Result::OK);
          if(DEFAULT_VALUE != _data)
          {
            if(serialized_size() <= buffer.get_max_size()) 
            {
              _serialize_varint(tag(), buffer);
              VAR_UINT_TYPE uint_data = static_cast<VAR_UINT_TYPE>(_data);
              _serialize_varint(uint_data, buffer);
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
          Result result(Result::OK);
          VAR_UINT_TYPE uint_data;
          if(_deserialize_varint(uint_data, buffer))
          {
            _data = static_cast<DATA_TYPE>(uint_data);
          }
          else
          {
            // TODO create a better error for this case. May be an error from _deserialize_varint().
            result = Result::ERROR_BUFFER_TO_SMALL;
          }
          return result;
        }

        //! \see Field::clear()
        void clear() final
        {
          _data = DEFAULT_VALUE;
        }
        
        //! \see Field::serialized_data_size()
        uint32_t serialized_data_size() const final
        {
          return serialized_size_varint(static_cast<VAR_UINT_TYPE>(_data));
        }

      protected:

        //! The actual data.
        DATA_TYPE _data;

    }; 


  } // End of namespace Detail

  /*!
    Actually define the types to beused in messages. These specify the template for different field 
    types.
    \{
  */
  typedef Detail::FieldVarInt<int32_t>  int32;
  typedef Detail::FieldVarInt<int64_t>  int64;

  typedef Detail::FieldVarInt<uint32_t> uint32;
  typedef Detail::FieldVarInt<uint64_t> uint64;
  /*! \} */


} // End of namespace EmbeddedProto


#endif // _FIELD_VARINT_H_
