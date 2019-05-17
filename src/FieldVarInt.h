
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

        }

        //! \see Field::deserialize()
        Result deserialize(MessageBufferInterface& buffer) final
        {

        }

        //! \see Field::clear()
        void clear() final
        {
          _data = DEFAULT_VALUE;
        }

        //! \see Field::serialized_size()
        uint32_t serialized_size() const final
        {
          return tag_size() + serialized_data_size();
        }

        //! \see Field::serialized_data_size()
        uint32_t serialized_data_size() const final
        {
          return 
        }

      protected:

        //! The actual data.
        DATA_TYPE _data;

    }; 


  } // End of namespace Detail


} // End of namespace EmbeddedProto


#endif // _FIELD_VARINT_H_
