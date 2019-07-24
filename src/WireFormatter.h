
#ifndef _WIRE_FORMATTER_H_
#define _WIRE_FORMATTER_H_

#include <cstdint>
#include "MessageBufferInterface.h"

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
      static bool WriteInt(uint32_t field_number, int32_t value, MessageBufferInterface& buffer)
      {        
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && IntNoTag(value, buffer);
      }

      static bool WriteInt(uint32_t field_number, int64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && IntNoTag(value, buffer);
      }

      static bool WriteUInt(uint32_t field_number, uint32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && UIntNoTag(value, buffer);
      }

      static bool WriteUInt(uint32_t field_number, uint64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && UIntNoTag(value, buffer);
      }

      static bool WriteSInt(uint32_t field_number, int32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && SIntNoTag(value, buffer);
      }

      static bool WriteSInt(uint32_t field_number, int64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && SIntNoTag(value, buffer);
      }

      static bool WriteFixed(uint32_t field_number, uint32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED32), buffer) 
               && FixedNoTag(value, buffer);
      }

      static bool WriteFixed(uint32_t field_number, uint64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED64), buffer) 
               && FixedNoTag(value, buffer);
      }

      static bool WriteSFixed(uint32_t field_number, int32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED32), buffer) 
               && SFixedNoTag(value, buffer);
      }

      static bool WriteSFixed(uint32_t field_number, int64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED64), buffer) 
               && SFixedNoTag(value, buffer);
      }

      static bool WriteFloat(uint32_t field_number, float value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED32), buffer) 
               && FloatNoTag(value, buffer);
      }

      static bool WriteDouble(uint32_t field_number, double value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::FIXED64), buffer) 
               && DoubleNoTag(value, buffer);
      }

      static bool WriteBool(uint32_t field_number, bool value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer)
               && BoolNoTag(value, buffer);
      }

      static bool WriteEnum(uint32_t field_number, uint32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(MakeTag(field_number, WireType::VARINT), buffer) 
               && EnumNoTag(value, buffer);
      }


      /** @} **/

//    private:

      //! This function converts a given value int a varint formated data array.
      /*!
        \param[in] value  The data to be serialized.
        \param[in] buffer Pointer to the first element of an array in which the data is to be serialized.
        \warning There should be sufficient space in the array to store a varint32.
        \return A pointer to the first byte after the data just serialized.
        This code is copied and modified from google protobuf sources.
      */
      static bool WriteVarint32ToArray(uint32_t value, MessageBufferInterface& buffer) {
        constexpr uint32_t MSB_BYTE = 0x00000080;
        constexpr uint8_t SHIFT_N_BITS = 7;

        while (value >= MSB_BYTE) {
          buffer.push(static_cast<uint8_t>(value | MSB_BYTE));
          value >>= SHIFT_N_BITS;
        }
        return buffer.push(static_cast<uint8_t>(value));
      }

      static bool WriteVarint64ToArray(uint64_t value, MessageBufferInterface& buffer) {
        constexpr uint64_t MSB_BYTE = 0x0000000000000080;
        constexpr uint8_t SHIFT_N_BITS = 7;
        
        while (value >= MSB_BYTE) {
          buffer.push(static_cast<uint8_t>(value | MSB_BYTE));
          value >>= SHIFT_N_BITS;
        }
        return buffer.push(static_cast<uint8_t>(value));
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
      static bool IntNoTag(int32_t value, MessageBufferInterface& buffer) 
      {
        return WriteVarint32ToArray(static_cast<uint32_t>(value), buffer);
      }

      static bool IntNoTag(int64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint64ToArray(static_cast<uint64_t>(value), buffer);
      }

      static bool UIntNoTag(uint32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(value, buffer);
      }

      static bool UIntNoTag(uint64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint64ToArray(value, buffer);
      }

      static bool SIntNoTag(int32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(ZigZagEncode(value), buffer);
      }

      static bool SIntNoTag(int64_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint64ToArray(ZigZagEncode(value), buffer);
      };

      static bool FixedNoTag(uint32_t value, MessageBufferInterface& buffer) 
      {
        // Write the data little endian to the array.
        // TODO Define a little endian flag to support memcpy the data to the array.
        buffer.push(static_cast<uint8_t>(value & 0x000000FF));
        buffer.push(static_cast<uint8_t>((value >> 8) & 0x000000FF));
        buffer.push(static_cast<uint8_t>((value >> 16) & 0x000000FF));
        return buffer.push(static_cast<uint8_t>((value >> 24) & 0x000000FF));
      }

      static bool FixedNoTag(uint64_t value, MessageBufferInterface& buffer)
      {
        // Write the data little endian to the array.
        // TODO Define a little endian flag to support memcpy the data to the array.
        buffer.push(static_cast<uint8_t>(value & 0x00000000000000FF));
        buffer.push(static_cast<uint8_t>((value >> 8) & 0x00000000000000FF));
        buffer.push(static_cast<uint8_t>((value >> 16) & 0x00000000000000FF));
        buffer.push(static_cast<uint8_t>((value >> 24) & 0x00000000000000FF));
        buffer.push(static_cast<uint8_t>((value >> 32) & 0x00000000000000FF));
        buffer.push(static_cast<uint8_t>((value >> 40) & 0x00000000000000FF));
        buffer.push(static_cast<uint8_t>((value >> 48) & 0x00000000000000FF));
        return buffer.push(static_cast<uint8_t>((value >> 56) & 0x00000000000000FF));
      }

      static bool SFixedNoTag(int32_t value, MessageBufferInterface& buffer)
      {
        return FixedNoTag(static_cast<uint32_t>(value), buffer);
      }

      static bool SFixedNoTag(int64_t value, MessageBufferInterface& buffer)
      {
        return FixedNoTag(static_cast<uint64_t>(value), buffer);
      }

      static bool FloatNoTag(float value, MessageBufferInterface& buffer)
      {
        // Cast the type to void and to a 32 fixed number
        void* pVoid = static_cast<void*>(&value);
        uint32_t* fixed = static_cast<uint32_t*>(pVoid);
        return FixedNoTag(*fixed, buffer);
      }

      static bool DoubleNoTag(double value, MessageBufferInterface& buffer)
      {
        // Cast the type to void and to a 64 fixed number
        void* pVoid = static_cast<void*>(&value);
        uint64_t* fixed = static_cast<uint64_t*>(pVoid);
        return FixedNoTag(*fixed, buffer);
      }

      static bool BoolNoTag(bool value, MessageBufferInterface& buffer)
      {
        return buffer.push(value ? 0x01 : 0x00);;
      }

      static bool EnumNoTag(uint32_t value, MessageBufferInterface& buffer)
      {
        return WriteVarint32ToArray(value, buffer);
      }
      /** @} **/

  };

} // End of namespace EmbeddedProto.
#endif
