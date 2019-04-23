
#ifndef _WIRE_FORMATTER_H_
#define _WIRE_FORMATTER_H_

#include <cstdint>

namespace EmbeddedProto 
{
namespace WireFormatter 
{
  
  enum class WireType 
  {
    VARINT            = 0,  //!< int32, int64, uint32, uint64, sint32, sint64, bool, enum.
    FIXED64           = 1,  //!< fixed64, sfixed64, double
    LENGTH_DELIMITED  = 2,  //!< string, bytes, embedded messages, packed repeated fields
    START_GROUP       = 3,  //!< Depricated
    END_GROUP         = 4,  //!< Depricated
    FIXED32           = 5,  //!< fixed32, sfixed32, float
  };

  //! Create the tag of a field. This is the combination of field number by three and wire type.
  /*!
    The field number is shifted to the left by three bits. This creates space to or the wire type
    of the designated field.
  */
  static constexpr uint32_t MakeTag(const uint32_t field_number, const WireType type)
  {
    return ((static_cast<uint32_t>(field_number) << 3) | static_cast<uint32_t>(type));
  }

  //! This function converts a given value int a varint formated data array.
  /*!
    \param[in] value  The data to be serialized.
    \param[in] target Pointer to the first element of an array in which the data is to be serialized.
    \warning There should be sufficient space in the array to store a varint32.
    \return A pointer to the first byte after the data just serialized.
    This code is copied and modified from google protobuf sources.
  */
  static constexpr uint8_t* WriteVarint32ToArray(uint32_t value, uint8_t* target) {
    constexpr uint32_t MSB_BYTE = 0x00000080;
    constexpr uint8_t SHIFT_N_BITS = 7;

    while (value >= MSB_BYTE) {
      *target = static_cast<uint8_t>(value | MSB_BYTE);
      value >>= SHIFT_N_BITS;
      ++target;
    }
    *target = static_cast<uint8_t>(value);
    return target + 1;
  }


} // End of namespace WireFormatter.
} // End of namespace EmbeddedProto.
#endif
