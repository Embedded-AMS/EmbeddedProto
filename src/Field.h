

#ifndef _FIELD_H_
#define _FIELD_H_

#include <cstdint>
#include <limits> 
#include <type_traits>
#include <math.h> 
#include <MessageBufferInterface.h>

namespace EmbeddedProto
{

  //! Definitions of the different encoding types used in protobuf.
  enum class WireType 
  {
    VARINT            = 0,  //!< int32, int64, uint32, uint64, sint32, sint64, bool, enum.
    FIXED64           = 1,  //!< fixed64, sfixed64, double
    LENGTH_DELIMITED  = 2,  //!< string, bytes, embedded messages, packed repeated fields
    START_GROUP       = 3,  //!< Depricated
    END_GROUP         = 4,  //!< Depricated
    FIXED32           = 5,  //!< fixed32, sfixed32, float
  };

  //! The base class for any type of field.
  /*!
      This class defines the basic structor for any type of field, that includes the basic types
      but also nested messages
  */
  class Field 
  {
    public:

      //! Possible return values of the (de)serialization functions of a field.
      enum class Result {
          OK,
          ERROR_BUFFER_TO_SMALL,
      };
      
      //! The default constructor which sets the wire type and field number of this field.
      Field(const WireType type, const uint32_t number) :
          _wire_type(type),
          _field_number(number)
      {

      }

      //! The default destructor.
      virtual ~Field() = default;

      //! Function to serialize this field including the tag.
      /*!
          The data this field holds will be serialized into an byte array.

          \param[in,out] buffer A reference to the buffer which can be used ot serialize the data.

          \return An enum value indicating successful operation of this function or an error.
      */
      virtual Result serialize(MessageBufferInterface& buffer) const = 0;

      //! From the buffer desirialize a tag.
      /*!
          This function returns the tag value, the combination of the field number and wire type.
          \param[out] tag The variable in which the tag value is returned. 
          \param[in,out] buffer The data buffer from which to deseriale the tag.
          \return An enum value indicating successful operation of this function or an error.
      */
      static Result deserialize_tag(uint32_t& tag, MessageBufferInterface& buffer) 
      {
        return _deserialize_varint(tag, buffer) ? Result::OK : Result::ERROR_BUFFER_TO_SMALL;
      }

      //! Function to deserialize this field excluding the tag.
      /*!
          From an array of bytes fill this field object with data.
          
          \param[in,out] buffer A reference to the buffer which is used to deserialize the data.

          \return An enum value indicating successful operation of this function or an error.
      */
      virtual Result deserialize(MessageBufferInterface& buffer) = 0;

      //! Clear the content of this field and set it to it's default state.
      /*!
          The defaults are to be set according to the Protobuf standard.
      */
      virtual void clear() = 0;

      //! Calculate the size of this field as if it was serialized, tag + data.
      /*!
          \return The number of bytes this field will takeup in total when serialized.
      */
      virtual uint32_t serialized_size() const
      { 
        return tag_size() + serialized_data_size();
      }

      //! Calculate the size of the data when serialized.
      /*!
          \return The number of bytes the data of this field will takeup when serialized.
      */
      virtual uint32_t serialized_data_size() const = 0;

    protected:

      //! The maximum number of bits the WireType will takeup.
      /*!
          The maximum value of a wire type is five. Three bits are thus required to endonce it.
      */
      static constexpr uint8_t WIRE_TYPE_BIT_SIZE = 3;

      //! Definition of a mask inidicating the most significat bit used in varint encoding.
      /*!
          \see _serialize_varint() _deserialize_varint()
      */
      static constexpr uint8_t VARINT_MSB_BYTE = 0x80;

      //! Definitation of the number of bits it takes to serialize a byte of a varint.
      /*!
          \see _serialize_varint() _deserialize_varint()
      */      
      static constexpr uint8_t VARINT_SHIFT_N_BITS = 7;

      //! The maximum value which can be stored in a signle byte in base 128 encoding.
      static constexpr uint8_t VARINT_MAX_SINGLE_BYTE = 
                                    static_cast<uint8_t>(~VARINT_MSB_BYTE);

      //! Create the tag of a field. 
      /*!
        This is the combination of the field number and wire type of the field. The field number is 
        shifted to the left by three bits. This creates space to or the wire type of the designated 
        field.
      */
      const uint32_t tag() const 
      {
        return ((static_cast<uint32_t>(_field_number) << WIRE_TYPE_BIT_SIZE)
                | static_cast<uint32_t>(_wire_type));
      }

      //! Calculate the number of bytes required to encode the given varint value.
      /*!
          The tag is encoded as a varint and anything below 127 will only take one byte.
          \warning This is a short cut and large field numbers are not supported at this moment.
          This function only operates on unsigned values.
          \param[in] value The value for which we would like to know the number of bytes.
          \return Then required number of bytes to encode the value. 
      */
      template<class VAR_UINT_TYPE>
      const uint32_t serialized_size_varint(const VAR_UINT_TYPE& value) const
      {
          static_assert(std::is_unsigned<VAR_UINT_TYPE>::value, "Varint size calculation is only "
                                                          " possible for unsigned integer types.");

          uint32_t n_bytes_required = 0;
          VAR_UINT_TYPE v = value;

          while(v > 0) {
            v = v >> VARINT_SHIFT_N_BITS;
            ++n_bytes_required;
          }

          return n_bytes_required;
      }

      //! Return the number of bytes the tag of this field will need.
      const uint8_t tag_size() const
      { 
        return serialized_size_varint(tag());
      }

      //! Obtain the wire type of this field.
      const WireType wireType() const 
      {
        return _wire_type;
      }

      //! Obtain the field number of this field.
      const uint32_t fieldNumber() const
      {
        return _field_number;
      }

      //! This function converts a given value int a varint formated data array.
      /*!
        This function only accepts unsigned integers.

        \param[in] value  The data to be serialized.
        \param[in,out] buffer The data buffer in which the varint is stored.
        \return True when serilization succedded, false otherwise.
      */
      template<class VARINT_TYPE>
      static bool _serialize_varint(VARINT_TYPE value, MessageBufferInterface& buffer)
      {
        static_assert(std::is_unsigned<VARINT_TYPE>::value, "Varint encoding only possible for "
                                                            "unsigned integer types.");
        bool result(true);

        while((value >= VARINT_MSB_BYTE) && result) 
        {
          result = buffer.push(static_cast<uint8_t>(value | VARINT_MSB_BYTE));
          value >>= VARINT_SHIFT_N_BITS;
        }
        result = buffer.push(static_cast<uint8_t>(value));

        return result;
      }

      //! This function converts a given buffer into an integer value.
      /*!
        This function only accepts unsigned integers.

        \param[in] value  Variable in which the deserialized unsigned integer is returned.
        \param[in,out] buffer The data set from which to obtain the value.
        \return True when deserialization succedded.
      */
      template<class VARINT_TYPE>
      static bool _deserialize_varint(VARINT_TYPE& value, MessageBufferInterface& buffer)
      {
        static_assert(std::is_unsigned<VARINT_TYPE>::value, "Varint encoding only possible for "
                                                            "unsigned integer types.");

        // Calculate how many bytes there are in a varint 128 base encoded number. This should 
        // yeild 5 for a 32bit number and 10 for a 64bit number.
        static constexpr uint8_t N_BYTES_IN_VARINT = static_cast<uint8_t>(std::ceil(
                                                          std::numeric_limits<VARINT_TYPE>::digits 
                                                        / static_cast<float>(VARINT_SHIFT_N_BITS)));
        
        VARINT_TYPE temp_value = 0;
        uint8_t byte = 0;
        bool result = buffer.pop(byte);
        // Loop until the end of the encoded varint or until there is nomore data in the buffer.
        for(uint8_t i = 0; (i < N_BYTES_IN_VARINT) && result; ++i) 
        {
          temp_value |= (byte & ~VARINT_MSB_BYTE) << (i * VARINT_SHIFT_N_BITS);
          if(byte & VARINT_MSB_BYTE) 
          {
            // Continue
            result = buffer.pop(byte);
          }
          else
          {
            // The varint is complete
            break;
          }
        }

        if(result)
        {
          value = temp_value;
        }

        return result;
      }

    private:

      //! The wire type of this field.
      const WireType _wire_type;

      //! The field number of this field.
      const uint32_t _field_number;

  };

} // End of namespace EmbeddedProto

#endif 
