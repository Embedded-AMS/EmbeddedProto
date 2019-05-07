
#ifndef _FIELD_FIXED_H_
#define _FIELD_FIXED_H_

#include "Field.h"

namespace EmbeddedProto
{

  namespace detail 
  {
    //! The field type with fixed length.
    template<WireType WIRE_TYPE, class DATA_TYPE>
    class FieldFixed : public Field
    {
      public:

        //! Check if for a fixed field a valid wire type has been supplied.
        static_assert((WIRE_TYPE == WireType::FIXED64) || (WIRE_TYPE == WireType::FIXED32), 
                "Invalid wire type supplied in template, only FIXEDXX types are allowed.");

        //! This typedef will return a unsigned 32 or 64 value depending on the field type.
        typedef typename std::conditional<WIRE_TYPE == WireType::FIXED32, uint32_t, uint64_t>::type 
                                                                                  VAR_UINT_TYPE;

        //! This typedef will return a signed 32 or 64 value depending on the field type.
        typedef typename std::conditional<WIRE_TYPE == WireType::FIXED32, int32_t, int64_t>::type 
                                                                                  VAR_INT_TYPE;
        
        //! The Protobuf default value for this type. Zero in this case.
        static constexpr DATA_TYPE DEFAULT_VALUE = static_cast<DATA_TYPE>(0);

        FieldFixed(const uint32_t number) :
            Field(WIRE_TYPE, number)
        {

        }

        ~FieldFixed() final = default;

        //! \see Field::serialize()
        Result serialize(MessageBufferInterface& buffer) const final
        {
          Result result(Result::OK);
          // Only serialize if the dat does not equal the default and when the size in the buffer 
          // is sufficient.
          if(std::numeric_limits<DATA_TYPE>::epsilon() < (_data - DEFAULT_VALUE))
          {
            if(serialized_size() <= buffer.get_max_size()) 
            {
              _serialize_varint(tag(), buffer);
              _serialize_fixed(static_cast<VAR_UINT_TYPE>(_data), buffer);
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
          return Result::OK;
        }

        //! \see Field::clear()
        void clear() final
        {
          _data = DEFAULT_VALUE;
        }

        //! \see Field::serialized_size()
        uint32_t serialized_size() const final
        {
          return tag_size() + std::numeric_limits<VAR_UINT_TYPE>::digits;
        }

      protected:

        //! Serialize a single unsigned integer into the buffer using the fixed size method.
        /*!
            The fixed size method just pushes every byte into the buffer.

            \param[in] value The variable to serialize.
            \param[in,out] buffer The data buffer to which the variable is serialized.
            \return True when serialization succedded.
        */
        bool _serialize_fixed(VAR_UINT_TYPE value, MessageBufferInterface& buffer) const
        {
          // Write the data little endian to the array.
          // TODO Define a little endian flag to support memcpy the data to the array.

          bool result(true);
          for(uint8_t i = 0; i < std::numeric_limits<VAR_UINT_TYPE>::digits; 
              i += std::numeric_limits<uint8_t>::digits)  
          {
            result = buffer.push(static_cast<uint8_t>((value >> i) & 0x00FF));
            if(!result)
            {
              // No more space in the buffer.
              break;
            }
          }
          return result;
        }


      private:

        DATA_TYPE _data;

    };

  } // End of namespace detail


  /*!
    Actually define the types to beused in messages. These specify the template for different field 
    types.
    \{
  */
  typedef detail::FieldFixed<WireType::FIXED64, uint64_t> fixed64;
  typedef detail::FieldFixed<WireType::FIXED64, int64_t>  sfixed64;
  typedef detail::FieldFixed<WireType::FIXED64, double>   double64;

  typedef detail::FieldFixed<WireType::FIXED32, uint32_t> fixed32;
  typedef detail::FieldFixed<WireType::FIXED32, int32_t>  sfixed32;
  typedef detail::FieldFixed<WireType::FIXED32, float>    float32;
  /*! \} */

} // End of namespace EmbeddedProto

#endif
