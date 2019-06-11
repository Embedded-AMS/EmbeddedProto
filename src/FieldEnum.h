
#ifndef _FIELD_ENUM_H_
#define _FIELD_ENUM_H_

#include "Field.h"

#include <type_traits>


namespace EmbeddedProto
{

  namespace Detail 
  {    
    //! The field type for enumerations normal and class enums.
    template<class ENUM_TYPE>
    class FieldEnum : public Field 
    {
      private:
        static_assert(std::is_enum<ENUM_TYPE>::value, "This class only supports enum and enum "
                                                      "classes as template parameter.");

        //! The actual type of integer used to store this enum underneeth the bonnit.
        typedef typename std::underlying_type<ENUM_TYPE>::type INT_TYPE;

        //! Initialize the enum always for its default value.
        /*!
            Protobuf dictates all enums should have their first item be equal to zero.
        */
        static constexpr ENUM_TYPE DEFAULT_VALUE = static_cast<ENUM_TYPE>(0);

      public:

        //! This default constructor sets the 
        FieldEnum(const uint32_t number) 
          : Field(WireType::VARINT, number),
            _data(DEFAULT_VALUE)
        {

        }

        ~FieldEnum() final = default;

        //! Obtain the current value.
        const ENUM_TYPE& get() const { return _data; }

        //! Set the enum
        void set(const ENUM_TYPE& e)
        {
          _data = e;
        }

        //! Assignment opertor to set the data.
        FieldEnum<ENUM_TYPE>& operator=(const ENUM_TYPE& e)
        {
          set(e);
          return *this;
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
              _serialize_varint(static_cast<INT_TYPE>(_data), buffer);
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
          uint64_t uint_data;
          if((0 < buffer.get_size()) && _deserialize_varint(uint_data, buffer))
          {
            _data = static_cast<ENUM_TYPE>(uint_data);
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
          return serialized_size_varint(static_cast<INT_TYPE>(_data));;
        }   

      private:

        ENUM_TYPE _data; 
    };

  } // End of namespace detail

  //! Short hand notation to access the Detail namespace for the FieldEnum class.   
  template<class ENUM_TYPE>
  using Enumeration = Detail::FieldEnum<ENUM_TYPE>;

} // End of namespace EmbeddedProto

#endif // End of _FIELD_ENUM_H_
