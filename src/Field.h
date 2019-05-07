

#ifndef _FIELD_H_
#define _FIELD_H_

#include <cstdint>
#include <limits> 
#include <type_traits>

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

      //! Function to serialize this field.
      /*!
          The data this field holds will be serialized into an byte array.

          \param buffer [out] The array of bytes into which the field will be serialized.
          \param length [in]  The number of bytes in the buffer array.

          \return An enum value indicating successful operation of this function or an error.
      */
      virtual Result serialize(uint8_t* buffer, uint32_t length) const = 0;

      //! Function to deserialize this field.
      /*!
          From an array of date fill this field object with data.

          \param buffer [in]  The array of bytes into which the field will be serialized.
          \param length [in]  The number of bytes in the buffer array.

          \return An enum value indicating successful operation of this function or an error.
      */
      virtual Result deserialize(const uint8_t* buffer, uint32_t length) = 0;

      //! Clear the content of this field and set it to it's default state.
      /*!
          The defaults are to be set according to the Protobuf standard.
      */
      virtual void clear() = 0;

      //! Calculate the size of this field as if it was serialized.
      /*!
          \return The number of bytes this field will takeup when serialized.
      */
      virtual uint32_t serialized_size() const = 0;

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
      static constexpr VARINT_TYPE VARINT_MSB_BYTE = 0x00000080;

      //! Definitation of the number of bits it takes to serialize a byte of a varint.
      /*!
          \see _serialize_varint() _deserialize_varint()
      */      
      static constexpr uint8_t VARINT_SHIFT_N_BITS = 7;

      //! The maximum value which can be stored in a signle byte in base 128 encoding.
      static constexpr uint8_t VARINT_MAX_SINGLE_BYTE = ~VARINT_MSB_BYTE;

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

      //! Return the number of bytes the tag of this field will need.
      /*!
          The tag is encoded as a varint and anything below 127 will only take one byte.
          \warning This is a short cut and large field numbers are not supported at this moment.
      */
      const uint8_t tag_size() const 
      {
        return (VARINT_MAX_SINGLE_BYTE <= tag()) ? 1 : 2;
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
        \param[in] target Pointer to the first element of an array in which the data is to be serialized.
        \warning There should be sufficient space in the array to store a varint32.
        \return A pointer to the first byte after the data just serialized.
      */
      template<class VARINT_TYPE>
      static constexpr uint8_t* _serialize_varint(VARINT_TYPE value, uint8_t* target) {
        static_assert(std::is_unsigned<VARINT_TYPE>::value, "Varint encoding only possible for "
                                                            "unsigned types.");

        while (value >= VARINT_MSB_BYTE) {
          *target = static_cast<uint8_t>(value | MSB_BYTE);
          value >>= VARINT_SHIFT_N_BITS;
          ++target;
        }
        *target = static_cast<uint8_t>(value);
        return target + 1;
      }

      //! This function converts a given buffer into an integer value.
      /*!
        This function only accepts unsigned integers.

        \param[in] value  The data to be serialized.
        \param[in] target Pointer to the first element of an array in which the data is to be serialized.
        \return A pointer to the first byte after the data just deserialized.
      */
      template<class VARINT_TYPE>
      static constexpr uint8_t* _deserialize_varint(VARINT_TYPE value, uint8_t* target) {
        static_assert(std::is_unsigned<VARINT_TYPE>::value, "Varint encoding only possible for "
                                                            "unsigned types.");


      }

    private:

      //! The wire type of this field.
      const WireType _wire_type;

      //! The field number of this field.
      const uint32_t _field_number;

  };

} // End of namespace EmbeddedProto

#endif 
