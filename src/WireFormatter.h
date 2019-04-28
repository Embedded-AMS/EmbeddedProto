
#ifndef _WIRE_FORMATTER_H_
#define _WIRE_FORMATTER_H_

#include <cstdint>

namespace EmbeddedProto 
{

  class WireFormatter 
  {
    public:
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

      /**
         @brief Write fields, including tags to the given array
         @{
      **/
      static constexpr uint8_t* WriteInt(uint32_t field_number, int32_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return IntNoTag(value, target);
      }

      static constexpr uint8_t* WriteInt(uint32_t field_number, int64_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return IntNoTag(value, target);
      }

      static constexpr uint8_t* WriteUInt(uint32_t field_number, uint32_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return UIntNoTag(value, target);
      }

      static constexpr uint8_t* WriteUInt(uint32_t field_number, uint64_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return UIntNoTag(value, target);
      }

      static constexpr uint8_t* WriteSInt(uint32_t field_number, int32_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return SIntNoTag(value, target);
      }

      static constexpr uint8_t* WriteSInt(uint32_t field_number, int64_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return SIntNoTag(value, target);
      }

      static constexpr uint8_t* WriteFixed(uint32_t field_number, uint32_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED32), target);
        return FixedNoTag(value, target);
      }

      static constexpr uint8_t* WriteFixed(uint32_t field_number, uint64_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED64), target);
        return FixedNoTag(value, target);
      }

      static constexpr uint8_t* WriteSFixed(uint32_t field_number, int32_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED32), target);
        return SFixedNoTag(value, target);
      }

      static constexpr uint8_t* WriteSFixed(uint32_t field_number, int64_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED64), target);
        return SFixedNoTag(value, target);
      }

      static constexpr uint8_t* WriteFloat(uint32_t field_number, float value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED32), target);
        return FloatNoTag(value, target);
      }

      static constexpr uint8_t* WriteDouble(uint32_t field_number, double value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED64), target);
        return DoubleNoTag(value, target);
      }

      static constexpr uint8_t* WriteBool(uint32_t field_number, bool value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return BoolNoTag(value, target);
      }

      static constexpr uint8_t* WriteEnum(uint32_t field_number, uint32_t value, uint8_t* target)
      {
        target = WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), target);
        return EnumNoTag(value, target);
      }


      /** @} **/

//    private:

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

      static constexpr uint8_t* WriteVarint64ToArray(uint64_t value, uint8_t* target) {
        constexpr uint64_t MSB_BYTE = 0x0000000000000080;
        constexpr uint8_t SHIFT_N_BITS = 7;

        while (value >= MSB_BYTE) {
          *target = static_cast<uint8_t>(value | MSB_BYTE);
          value >>= SHIFT_N_BITS;
          ++target;
        }
        *target = static_cast<uint8_t>(value);
        return target + 1;
      }

      //! Encode a signed 32 bit integer using the zig zag method
      /*!
        As specified the right-shift must be arithmetic, hence the cast is after the shift. The 
        left shift must be unsigned because of overflow.

        \param[in] n The signed value to be encoded.
        \return The zig zag transformed value ready for serialization into the array.
      */
      static constexpr uint32_t ZigZagEncode(const int32_t n) {
        return (static_cast<uint32_t>(n) << 1) ^ static_cast<uint32_t>(n >> 31);
      }

      //! Encode a signed 32 bit integer using the zig zag method
      /*!
        As specified the right-shift must be arithmetic, hence the cast is after the shift. The 
        left shift must be unsigned because of overflow.

        \param[in] n The signed value to be encoded.
        \return The zig zag transformed value ready for serialization into the array.
      */
      static constexpr uint64_t ZigZagEncode(const int64_t n) {
        return (static_cast<uint64_t>(n) << 1) ^ static_cast<uint64_t>(n >> 63);
      }

      //! Create the tag of a field. 
      /*!
        This is the combination of the field number and wire type of the field. The field number is 
        shifted to the left by three bits. This creates space to or the wire type of the designated 
        field.
      */
      static constexpr uint32_t MakeTag(const uint32_t field_number, const WireType type)
      {
        return ((static_cast<uint32_t>(field_number) << 3) | static_cast<uint32_t>(type));
      }

      /**
         @brief Write fields, without tags the given array.
         @{
      **/
      static constexpr uint8_t* IntNoTag(int32_t value, uint8_t* target) 
      {
        return WriteVarint32ToArray(static_cast<uint32_t>(value), target);
      }

      static constexpr uint8_t* IntNoTag(int64_t value, uint8_t* target)
      {
        return WriteVarint64ToArray(static_cast<uint64_t>(value), target);
      }

      static constexpr uint8_t* UIntNoTag(uint32_t value, uint8_t* target)
      {
        return WriteVarint32ToArray(value, target);
      }

      static constexpr uint8_t* UIntNoTag(uint64_t value, uint8_t* target)
      {
        return WriteVarint64ToArray(value, target);
      }

      static constexpr uint8_t* SIntNoTag(int32_t value, uint8_t* target)
      {
        return WriteVarint32ToArray(ZigZagEncode(value), target);
      }

      static constexpr uint8_t* SIntNoTag(int64_t value, uint8_t* target)
      {
        return WriteVarint64ToArray(ZigZagEncode(value), target);
      };

      static constexpr uint8_t* FixedNoTag(uint32_t value, uint8_t* target) 
      {
        // Write the data little endian to the array.
        // TODO Define a little endian flag to support memcpy the data to the array.
        target[0] = static_cast<uint8_t>(value & 0x000000FF);
        target[1] = static_cast<uint8_t>((value >> 8) & 0x000000FF);
        target[2] = static_cast<uint8_t>((value >> 16) & 0x000000FF);
        target[3] = static_cast<uint8_t>((value >> 24) & 0x000000FF);
        return target + 4;
      }

      static constexpr uint8_t* FixedNoTag(uint64_t value, uint8_t* target)
      {
        // Write the data little endian to the array.
        // TODO Define a little endian flag to support memcpy the data to the array.
        target[0] = static_cast<uint8_t>(value & 0x00000000000000FF);
        target[1] = static_cast<uint8_t>((value >> 8) & 0x00000000000000FF);
        target[2] = static_cast<uint8_t>((value >> 16) & 0x00000000000000FF);
        target[3] = static_cast<uint8_t>((value >> 24) & 0x00000000000000FF);
        target[4] = static_cast<uint8_t>((value >> 32) & 0x00000000000000FF);
        target[5] = static_cast<uint8_t>((value >> 40) & 0x00000000000000FF);
        target[6] = static_cast<uint8_t>((value >> 48) & 0x00000000000000FF);
        target[7] = static_cast<uint8_t>((value >> 56) & 0x00000000000000FF);
        return target + 8;
      }

      static constexpr uint8_t* SFixedNoTag(int32_t value, uint8_t* target)
      {
        return FixedNoTag(static_cast<uint32_t>(value), target);
      }

      static constexpr uint8_t* SFixedNoTag(int64_t value, uint8_t* target)
      {
        return FixedNoTag(static_cast<uint64_t>(value), target);
      }

      static constexpr uint8_t* FloatNoTag(float value, uint8_t* target)
      {
        // Cast the type to void and to a 32 fixed number
        void* pVoid = static_cast<void*>(&value);
        uint32_t* fixed = static_cast<uint32_t*>(pVoid);
        return FixedNoTag(*fixed, target);
      }

      static constexpr uint8_t* DoubleNoTag(double value, uint8_t* target)
      {
        // Cast the type to void and to a 64 fixed number
        void* pVoid = static_cast<void*>(&value);
        uint64_t* fixed = static_cast<uint64_t*>(pVoid);
        return FixedNoTag(*fixed, target);
      }

      static constexpr uint8_t* BoolNoTag(bool value, uint8_t* target)
      {
        *target = value ? 0x01 : 0x00;
        return target + 1;
      }

      static constexpr uint8_t* EnumNoTag(uint32_t value, uint8_t* target)
      {
        return WriteVarint32ToArray(value, target);
      }
      /** @} **/

  };

} // End of namespace EmbeddedProto.
#endif
