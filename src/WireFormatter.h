
#ifndef _WIRE_FORMATTER_H_
#define _WIRE_FORMATTER_H_

#include <cstdint>
#include <math.h> 
#include <type_traits>

#include "MessageBufferInterface.h"

namespace EmbeddedProto 
{

  class WireFormatter 
  {

      //! Definitation of the number of bits it takes to serialize a byte of a varint.
      static constexpr uint8_t VARINT_SHIFT_N_BITS = 7;

      //! Definition of a mask inidicating the most significat bit used in varint encoding.
      static constexpr uint8_t VARINT_MSB_BYTE = 0x80;

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

        while (value >= VARINT_MSB_BYTE) {
          buffer.push(static_cast<uint8_t>(value | VARINT_MSB_BYTE));
          value >>= VARINT_SHIFT_N_BITS;
        }
        return buffer.push(static_cast<uint8_t>(value));
      }

      static bool WriteVarint64ToArray(uint64_t value, MessageBufferInterface& buffer) {
        
        while (value >= VARINT_MSB_BYTE) {
          buffer.push(static_cast<uint8_t>(value | VARINT_MSB_BYTE));
          value >>= VARINT_SHIFT_N_BITS;
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

      template<class RET_TYPE>
      static constexpr RET_TYPE ZigZagDecode(const uint64_t n) {
        return static_cast<RET_TYPE>((n >> 1) | ((n & 0x01) << 63));
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


      template<class UINT_TYPE>
      static bool ReadUInt(MessageBufferInterface& buffer, UINT_TYPE& value) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to ReadUInt.");
        
        // Calculate how many bytes there are in a varint 128 base encoded number. This should 
        // yeild 5 for a 32bit number and 10 for a 64bit number.
        static constexpr uint8_t N_BYTES_IN_VARINT = static_cast<uint8_t>(std::ceil(
                                                          std::numeric_limits<UINT_TYPE>::digits 
                                                        / static_cast<float>(VARINT_SHIFT_N_BITS)));
        
        UINT_TYPE temp_value = 0;
        uint8_t byte = 0;
        bool result = buffer.pop(byte);
        // Loop until the end of the encoded varint or until there is nomore data in the buffer.
        for(uint8_t i = 0; (i < N_BYTES_IN_VARINT) && result; ++i) 
        {
          temp_value |= static_cast<UINT_TYPE>(byte & (~VARINT_MSB_BYTE)) << (i * VARINT_SHIFT_N_BITS);
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

      static bool ReadTag(MessageBufferInterface& buffer, WireType& type, uint32_t& id) 
      {
        uint32_t temp_value;
        bool result = ReadUInt(buffer, temp_value);
        if(result) {
          type = static_cast<WireType>(temp_value &  0x07);
          id = (temp_value >> 3);
        }
        return result;
      }

      template<class INT_TYPE>
      static bool ReadInt(MessageBufferInterface& buffer, INT_TYPE& value) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to ReadInt.");
        
        uint64_t uint_value;
        bool result = ReadUInt(buffer, uint_value);
        if(result) 
        {
          value = static_cast<INT_TYPE>(uint_value);
        }
        return result;
      }

      template<class INT_TYPE>
      static bool ReadSInt(MessageBufferInterface& buffer, INT_TYPE& value) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to ReadSInt.");
        
        uint64_t uint_value;
        bool result = ReadUInt(buffer, uint_value);
        if(result) 
        {
          value = ZigZagDecode<INT_TYPE>(uint_value);
        }
        return result;
      }

      template<class TYPE>
      static bool ReadFixed(MessageBufferInterface& buffer, TYPE& value) 
      {
        static_assert(std::is_same<TYPE, uint32_t>::value || 
                      std::is_same<TYPE, uint64_t>::value, "Wrong type passed to ReadFixed.");

          // Read the data little endian to the buffer.
          // TODO Define a little endian flag to support memcpy the data from the buffer.

          TYPE temp_value = 0;
          bool result(true);
          uint8_t byte = 0;
          for(uint8_t i = 0; (i < std::numeric_limits<TYPE>::digits) && result; 
              i += std::numeric_limits<uint8_t>::digits)  
          {
            result = buffer.pop(byte);
            if(result)
            {
              temp_value |= (static_cast<TYPE>(byte) << i);
            }
          }

          if(result)
          {
            value = temp_value;
          }

          return result;
      }

      template<class STYPE>
      static bool ReadSFixed(MessageBufferInterface& buffer, STYPE& value) 
      {
        static_assert(std::is_same<STYPE, int32_t>::value || 
                      std::is_same<STYPE, int64_t>::value, "Wrong type passed to ReadSFixed.");

        typedef typename std::make_unsigned<STYPE>::type USTYPE;
        USTYPE temp_unsigned_value = 0;
        bool result = ReadFixed(buffer, temp_unsigned_value);
        if(result) {
          value = static_cast<STYPE>(temp_unsigned_value);
        }

        return result;
      }

      static bool ReadFloat(MessageBufferInterface& buffer, float& value) 
      {
        uint32_t temp_value = 0;
        bool result = ReadFixed(buffer, temp_value);
        if(result) {
          // Cast from unsigned int to a float.
          const void* pVoid = static_cast<const void*>(&temp_value);
          const float* pFloat = static_cast<const float*>(pVoid);
          value = *pFloat;
        }
        return result;
      }

      static bool ReadDouble(MessageBufferInterface& buffer, double& value) 
      {
        uint64_t temp_value = 0;
        bool result = ReadFixed(buffer, temp_value);
        if(result) {
          // Cast from unsigned int to a double.
          const void* pVoid = static_cast<const void*>(&temp_value);
          const double* pDouble = static_cast<const double*>(pVoid);
          value = *pDouble;
        }
        return result;
      }

      static bool ReadBool(MessageBufferInterface& buffer, bool& value) 
      {
        uint8_t byte;
        bool result = buffer.pop(byte);
        if(result) {
          value = static_cast<bool>(byte);
        }
        return result;
      }

      template<class ENUM_TYPE>
      static bool ReadEnum(MessageBufferInterface& buffer, ENUM_TYPE& value) 
      {
        static_assert(std::is_enum<ENUM_TYPE>::value, "No enum given to ReadEnum parameter value.");
        uint64_t temp_value;
        bool result = ReadUInt(buffer, temp_value);
        if(result) {
          value = static_cast<ENUM_TYPE>(temp_value);
        }
        return result;
      }

      template<class UINT_TYPE>
      static constexpr uint32_t serialized_size_VARINT(const UINT_TYPE& value)
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, 
                      "Wrong type passed to serialized_size_VARINT.");

        UINT_TYPE temp_value = value;
        uint8_t size = 0;
        while (temp_value >= VARINT_MSB_BYTE) {
          temp_value >>= VARINT_SHIFT_N_BITS;
          ++size;
        }

        return size;
      }

      static constexpr uint32_t serialized_size_BOOL(const bool& value)
      {
        return value ? 1 : 0;
      }

      template<class ENUM_TYPE>
      static constexpr uint32_t serialized_size_ENUM(const ENUM_TYPE& value)
      {
        static_assert(std::is_enum<ENUM_TYPE>::value, "Wrong type passed to serialized_size_ENUM");
        typedef typename std::underlying_type<ENUM_TYPE>::type INT_TYPE;
        INT_TYPE temp = static_cast<INT_TYPE>(value);
        return serialized_size_VARINT(temp);
      }

      template<class INT_VALUE>
      static constexpr uint32_t serialized_size_FIXED32(const INT_VALUE& value)
      {
        return 4;
      }

      template<class INT_VALUE>
      static constexpr uint32_t serialized_size_FIXED64(const INT_VALUE& value)
      {
        return 8;
      }

  };

} // End of namespace EmbeddedProto.
#endif
