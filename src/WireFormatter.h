/*
 *  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
 *
 *  This file is part of Embedded Proto.
 *
 *  Embedded Proto is open source software: you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License as published 
 *  by the Free Software Foundation, version 3 of the license.
 *
 *  Embedded Proto  is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
 *
 *  For commercial and closed source application please visit:
 *  <https://embeddedproto.com/pricing/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Atoomweg 2
 *    1627 LE, Hoorn
 *    the Netherlands
 */

#ifndef _WIRE_FORMATTER_H_
#define _WIRE_FORMATTER_H_

#include "Defines.h"
#include "WriteBufferInterface.h"
#include "ReadBufferInterface.h"
#include "Errors.h"

#include <cstdint>
#include <cstring>
#include <math.h>
#include <type_traits>
#include <limits>
#include <type_traits>

//! Determine whether the target stores multi-byte scalars little-endian.
/*!
    Protobuf fixed-width fields (fixed32/fixed64/sfixed/float/double) are
    little-endian on the wire, which matches the in-memory representation on a
    little-endian target. When that holds a packed payload can be copied to the
    buffer in a single block; otherwise every value is emitted byte-by-byte so
    the on-wire order stays little-endian. Define EMBEDDED_PROTO_LITTLE_ENDIAN
    yourself (to 0 or 1) to override the automatic detection.
*/
#if !defined(EMBEDDED_PROTO_LITTLE_ENDIAN)
  #if (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) \
       && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)) \
      || defined(_WIN32) || defined(_M_IX86) || defined(_M_X64) \
      || defined(_M_ARM) || defined(_M_ARM64)
    #define EMBEDDED_PROTO_LITTLE_ENDIAN 1
  #else
    #define EMBEDDED_PROTO_LITTLE_ENDIAN 0
  #endif
#endif

namespace EmbeddedProto
{

  //! This class combines functions to serialize and deserialize messages.
  class WireFormatter 
  {

      //! Definition of the number of bits it takes to serialize a byte of a varint.
      static constexpr uint8_t VARINT_SHIFT_N_BITS = 7;

      //! Definition of a mask indicating the most significant bit used in varint encoding.
      static constexpr uint8_t VARINT_MSB_BYTE = 0x80;

      //! Convert the floating point number to the next highes integer.
      /*!
        The ceil function in std is not a constexpr. Some compilers doe not accept this.

        \param num The value you would like ot convert.
        \return The resulting integer. 
      */
      static constexpr int32_t constexpr_ceil(float num)
      {
          return (static_cast<float>(static_cast<int32_t>(num)) == num)
              ? static_cast<int32_t>(num)
              : static_cast<int32_t>(num) + ((num > 0) ? 1 : 0);
      }

    public:
      //! Definitions of the different encoding types used in protobuf.
      enum class WireType 
      {
        VARINT            = 0,  //!< int32, int64, uint32, uint64, sint32, sint64, bool, enum.
        FIXED64           = 1,  //!< fixed64, sfixed64, double
        LENGTH_DELIMITED  = 2,  //!< string, bytes, embedded messages, packed repeated fields
        START_GROUP       = 3,  //!< Deprecated
        END_GROUP         = 4,  //!< Deprecated
        FIXED32           = 5,  //!< fixed32, sfixed32, float
      };

      //! Calculate the number of bytes a varint value will take. 
      /*!
        \param[in] value The value of which to calculate the size.
        \return The number of bytes required for serializing the varint.
      */
      static constexpr uint32_t VarintSize(const uint64_t value)
      {
        return (value < (1ULL << 7)) ? 1 :
           (value < (1ULL << 14)) ? 2 :
           (value < (1ULL << 21)) ? 3 :
           (value < (1ULL << 28)) ? 4 :
           (value < (1ULL << 35)) ? 5 :
           (value < (1ULL << 42)) ? 6 :
           (value < (1ULL << 49)) ? 7 :
           (value < (1ULL << 56)) ? 8 :
           (value < (1ULL << 63)) ? 9 : 10;
      }

      //! Encode a signed integer using the zig zag method
      /*!
        As specified the right-shift must be arithmetic, hence the cast is after the shift. The 
        left shift must be unsigned because of overflow.

        This function is suitable for 32 and 64 bit.

        \param[in] n The signed value to be encoded.
        \return The zig zag transformed value ready for serialization into the array.
      */
      template<class INT_TYPE>
      static constexpr typename std::make_unsigned<INT_TYPE>::type ZigZagEncode(const INT_TYPE n) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to ZigZagEncode.");

        using UINT_TYPE = typename std::make_unsigned<INT_TYPE>::type;
        // Prior to reverting back to C++11 we had defined:
        // constexpr uint8_t N_BITS_TO_ZIGZAG = std::numeric_limits<UINT_TYPE>::digits - 1;
        return (static_cast<UINT_TYPE>(n) << 1) ^ static_cast<UINT_TYPE>(n >> (std::numeric_limits<UINT_TYPE>::digits - 1));
      }

      //! Decode a signed integer using the zig zag method
      /*!
          \param[in] n The value encoded in zig zag to be decoded.
          \return The decoded signed value.

          This function is suitable for 32 and 64 bit.
      */
      template<class UINT_TYPE>
      static constexpr typename std::make_signed<UINT_TYPE>::type ZigZagDecode(const UINT_TYPE n) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to ZigZagDecode.");

        using INT_TYPE = typename std::make_signed<UINT_TYPE>::type;

        return static_cast<INT_TYPE>((n >> 1) ^ (~(n & 1) + 1));
      }

      //! Create the tag of a field. 
      /*!
        This is the combination of the field number and wire type of the field. The field number is 
        shifted to the left by three bits. This creates space to or the wire type of the designated 
        field.
      */
      static constexpr uint32_t MakeTag(const uint32_t field_number, const WireType type)
      {
        return ((field_number << 3) | static_cast<uint32_t>(type));
      }

      /**
         @brief Serialize fields, without tags the given buffer.
         @{
      **/

      //! Serialize an unsigned fixed length field without the tag.
      template<class UINT_TYPE>
      static Error SerializeFixedNoTag(const UINT_TYPE value, WriteBufferInterface& buffer)
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to SerializeFixedNoTag.");

        // Push the data little endian to the buffer.
        // TODO Define a little endian flag to support memcpy the data to the buffer.

        bool result = true;

        // Loop over all bytes in the integer.
        for(uint8_t i = 0; (i < std::numeric_limits<UINT_TYPE>::digits) && result; i += 8) {
          // Shift the value using the current value of i.
          result = buffer.push(static_cast<uint8_t>((value >> i) & 0x00FF));
        }
        return result ? Error::NO_ERRORS : Error::BUFFER_FULL;
      }

      //! Serialize a signed fixed length field without the tag.
      template<class INT_TYPE>
      static Error SerialzieSFixedNoTag(const INT_TYPE value, WriteBufferInterface& buffer)
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to SerialzieSFixedNoTag.");

        using UINT_TYPE = typename std::make_unsigned<INT_TYPE>::type;

        return SerializeFixedNoTag(static_cast<UINT_TYPE>(value), buffer);
      }

      //! Serialize a 32bit real value without tag.
      static Error SerialzieFloatNoTag(const float value, WriteBufferInterface& buffer)
      {
        // Cast the type to void and to a 32 fixed number
        const auto* pVoid = static_cast<const void*>(&value);
        const auto* fixed = static_cast<const uint32_t*>(pVoid);
        return SerializeFixedNoTag(*fixed, buffer);
      }

      //! Serialize a 64bit real value without tag.
      static Error SerialzieDoubleNoTag(const double value, WriteBufferInterface& buffer)
      {
        // Cast the type to void and to a 64 fixed number
        const auto* pVoid = static_cast<const void*>(&value);
        const auto* fixed = static_cast<const uint64_t*>(pVoid);
        return SerializeFixedNoTag(*fixed, buffer);
      }

      //! Serialize a contiguous array of fixed-width scalar values without tags.
      /*!
          Writes `count` values, each `sizeof(VAR_TYPE)` bytes wide, little-endian
          on the wire (protobuf packed fixed32/fixed64 layout). On a little-endian
          target the whole block is copied to the buffer with a single
          push(bytes, length) call. On a big-endian target every value is emitted
          byte-by-byte so the on-wire order stays little-endian.

          This is used to batch packed repeated fixed32/sfixed32/float and
          fixed64/sfixed64/double fields into as few buffer writes as possible.

          \param[in] data   Pointer to the first value.
          \param[in] count  The number of values to serialize.
          \param[in] buffer The buffer to write to.
          \return NO_ERRORS on success, BUFFER_FULL when the buffer ran out of space.
      */
      template<class VAR_TYPE>
      static Error SerializeFixedArrayNoTag(const VAR_TYPE* data, const uint32_t count,
                                            WriteBufferInterface& buffer)
      {
        static_assert((4U == sizeof(VAR_TYPE)) || (8U == sizeof(VAR_TYPE)),
                      "SerializeFixedArrayNoTag only supports 32 and 64 bit values.");

#if EMBEDDED_PROTO_LITTLE_ENDIAN
        // The in-memory representation already equals the little-endian wire
        // format, so the whole block can be pushed in one call.
        const auto* const raw = reinterpret_cast<const uint8_t*>(data);
        const uint32_t n_bytes = count * static_cast<uint32_t>(sizeof(VAR_TYPE));
        return buffer.push(raw, n_bytes) ? Error::NO_ERRORS : Error::BUFFER_FULL;
#else
        // Big-endian fallback: emit every value in little-endian byte order.
        using UINT_TYPE = typename std::conditional<4U == sizeof(VAR_TYPE),
                                                    uint32_t, uint64_t>::type;
        Error return_value = Error::NO_ERRORS;
        for(uint32_t i = 0; (i < count) && (Error::NO_ERRORS == return_value); ++i)
        {
          UINT_TYPE bits = 0;
          memcpy(&bits, reinterpret_cast<const uint8_t*>(data) + (i * sizeof(VAR_TYPE)),
                 sizeof(VAR_TYPE));
          return_value = SerializeFixedNoTag(bits, buffer);
        }
        return return_value;
#endif
      }
      /** @} **/


      /**
         @brief Serialize fields, including tags to the given buffer.
         @{
      **/
      template<class INT_TYPE>
      static Error SerializeInt(const uint32_t field_number, const INT_TYPE value, 
                                WriteBufferInterface& buffer)
      {        
        using UINT_TYPE = typename std::make_unsigned<INT_TYPE>::type;
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerializeVarint(static_cast<UINT_TYPE>(value), buffer);
        }
        return return_value;
      }

      template<class UINT_TYPE>
      static Error SerializeUInt(const uint32_t field_number, const UINT_TYPE value, 
                                WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerializeVarint(value, buffer);
        }
        return return_value;
      }

      template<class INT_TYPE>
      static Error SerializeSInt(const uint32_t field_number, const INT_TYPE value, 
                                 WriteBufferInterface& buffer)
      {
         Error return_value = SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer);
         if(Error::NO_ERRORS == return_value)
        {
          return_value = SerializeVarint(ZigZagEncode(value), buffer);
        }
        return return_value;
      }
      
      static Error SerializeFixed(const uint32_t field_number, const uint32_t value, 
                                  WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::FIXED32), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerializeFixedNoTag(value, buffer);
        }
        return return_value;
      }

      static Error SerializeFixed(const uint32_t field_number, const uint64_t value, 
                                  WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::FIXED64), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerializeFixedNoTag(value, buffer);
        }
        return return_value;
      }

      static Error SerializeSFixed(const uint32_t field_number, const int32_t value, 
                                   WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::FIXED32), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerialzieSFixedNoTag(value, buffer);
        }
        return return_value;
      }

      static Error SerializeSFixed(const uint32_t field_number, const int64_t value, 
                                   WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::FIXED64), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerialzieSFixedNoTag(value, buffer);
        }
        return return_value;
      }

      static Error SerializeFloat(const uint32_t field_number, const float value, 
                                  WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::FIXED32), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerialzieFloatNoTag(value, buffer);
        }
        return return_value;
      }

      static Error SerializeDouble(const uint32_t field_number, const double value, 
                                   WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::FIXED64), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerialzieDoubleNoTag(value, buffer);
        }
        return return_value;
      }

      static Error SerializeBool(const uint32_t field_number, const bool value, 
                                 WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          const uint8_t byte = value ? 0x01 : 0x00;
          return_value = buffer.push(byte) ? Error::NO_ERRORS : Error::BUFFER_FULL;
        }
        return return_value;
      }

      static Error SerializeEnum(const uint32_t field_number, const uint32_t value, 
                                 WriteBufferInterface& buffer)
      {
        Error return_value = SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer);
        if(Error::NO_ERRORS == return_value)
        {
          return_value = SerializeVarint(value, buffer);
        }
        return return_value;
      }

      /** @} **/

      /**
        @brief Deserialize fields from the given buffer.
        @{
      **/

      //! Read from the buffer the next wiretype and field id. 
      /*!
          \param[in] buffer The data source from which to read the type and id.
          \param[out] type This parameter returns the wiretype of the next field in the data buffer.
          \param[out] id This parameter returns the next field id.
          \return A value from the EmbeddedProto::Error enum indicating if the process succeeded.
      */
      static Error DeserializeTag(ReadBufferInterface& buffer, WireType& type, uint32_t& id) 
      {
        uint32_t temp_value;
        // Read the next varint considered to be a tag.
        Error return_value = DeserializeVarint(buffer, temp_value);
        
        if(Error::NO_ERRORS == return_value) 
        {
          // Next check the validity of the wire type.
          if((temp_value &  0x07) <= static_cast<uint32_t>(WireType::FIXED32))
          {
            // If reading the tag succeeded and the wire type is a valid one.
            type = static_cast<WireType>(temp_value &  0x07);
            id = (temp_value >> 3);
          }
          else 
          {
            return_value = Error::INVALID_WIRETYPE;
          }
        }
        return return_value;
      }

      template<class UINT_TYPE>
      static Error DeserializeUInt(ReadBufferInterface& buffer, UINT_TYPE& value) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to DeserializeUInt.");
        
        return DeserializeVarint(buffer, value);
      }

      template<class INT_TYPE>
      static Error DeserializeInt(ReadBufferInterface& buffer, INT_TYPE& value) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to DeserializeInt.");
        
        // Use a 64 value even for 32 bit as some implementations serialize 32 bit values with 10 bytes.
        uint64_t uint_value64;
        
        Error result = DeserializeVarint(buffer, uint_value64);
        if(Error::NO_ERRORS == result) 
        {
          value = static_cast<INT_TYPE>(uint_value64);
        }
        return result;
      }

      template<class INT_TYPE>
      static Error DeserializeSInt(ReadBufferInterface& buffer, INT_TYPE& value) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to DeserializeSInt.");
        
        // Use a 64 value even for 32 bit as some implementations serialize 32 bit values with 10 bytes.
        uint64_t uint_value64;

        Error result = DeserializeVarint(buffer, uint_value64);
        if(Error::NO_ERRORS == result) 
        {
          using UINT_TYPE = typename std::make_unsigned<INT_TYPE>::type;
          const auto uint_value = static_cast<UINT_TYPE>(uint_value64);
          value = ZigZagDecode(uint_value);
        }
        return result;
      }

      template<class TYPE>
      static Error DeserializeFixed(ReadBufferInterface& buffer, TYPE& value) 
      {
        static_assert(std::is_same<TYPE, uint32_t>::value || 
                      std::is_same<TYPE, uint64_t>::value, "Wrong type passed to DeserializeFixed.");

        // Deserialize the data little endian to the buffer.
        // TODO Define a little endian flag to support memcpy the data from the buffer.

        TYPE temp_value = 0;
        bool result(true);
        uint8_t byte = 0;
        uint8_t n_bytes_ahead = 0;
        uint8_t i = 0;

        for(i = 0; (i < std::numeric_limits<TYPE>::digits) && result; 
            i += std::numeric_limits<uint8_t>::digits)  
        {
          // Caluclate which byte to peek a head from the read buffer based on the number of bits.
          n_bytes_ahead = i / 8;
          result = buffer.peek(n_bytes_ahead, byte);
          if(result)
          {
            temp_value |= (static_cast<TYPE>(byte) << i);
          }
        }

        Error return_value = Error::NO_ERRORS;
        if(result)
        {
          value = temp_value;
          // Advance the buffer to the next byte to be proccesd
          buffer.advance(n_bytes_ahead+1);
        }
        else 
        {
          return_value = Error::END_OF_BUFFER;
        }

        return return_value;
      }

      template<class STYPE>
      static Error DeserializeSFixed(ReadBufferInterface& buffer, STYPE& value) 
      {
        static_assert(std::is_same<STYPE, int32_t>::value || 
                      std::is_same<STYPE, int64_t>::value, "Wrong type passed to DeserializeSFixed.");

        using USTYPE = typename std::make_unsigned<STYPE>::type;
        USTYPE temp_unsigned_value = 0;
        Error result = DeserializeFixed(buffer, temp_unsigned_value);
        if(Error::NO_ERRORS == result)
        {
          value = static_cast<STYPE>(temp_unsigned_value);
        }

        return result;
      }

      static Error DeserializeFloat(ReadBufferInterface& buffer, float& value) 
      {
        uint32_t temp_value = 0;
        Error result = DeserializeFixed(buffer, temp_value);
        if(Error::NO_ERRORS == result) 
        {
          // Cast from unsigned int to a float.
          const auto* pVoid = static_cast<const void*>(&temp_value);
          const auto* pFloat = static_cast<const float*>(pVoid);
          value = *pFloat;
        }
        return result;
      }

      static Error DeserializeDouble(ReadBufferInterface& buffer, double& value) 
      {
        uint64_t temp_value = 0;
        Error result = DeserializeFixed(buffer, temp_value);
        if(Error::NO_ERRORS == result) 
        {
          // Cast from unsigned int to a double.
          const auto* pVoid = static_cast<const void*>(&temp_value);
          const auto* pDouble = static_cast<const double*>(pVoid);
          value = *pDouble;
        }
        return result;
      }

      //! Deserialize a contiguous array of fixed-width scalar values without tags.
      /*!
          Reads `count` values, each `sizeof(VAR_TYPE)` bytes wide, from the packed
          little-endian on-wire layout (protobuf fixed32/fixed64). On a
          little-endian target the whole block is copied out of the buffer with a
          single pop(bytes, length) call. On a big-endian target every value is
          read individually so the little-endian wire order is honoured.

          The operation is all-or-nothing: when the buffer holds fewer than
          `count * sizeof(VAR_TYPE)` bytes nothing is consumed and END_OF_BUFFER is
          returned, mirroring DeserializeFixed(). This is the receive-side
          counterpart of SerializeFixedArrayNoTag() and is used to batch packed
          repeated fixed-width fields into as few buffer reads as possible.

          \param[out] dest  Pointer to the first value to fill.
          \param[in] count  The number of values to deserialize.
          \param[in] buffer The buffer to read from.
          \return NO_ERRORS on success, END_OF_BUFFER when too few bytes are available.
      */
      template<class VAR_TYPE>
      static Error DeserializeFixedArrayNoTag(VAR_TYPE* dest, const uint32_t count,
                                              ReadBufferInterface& buffer)
      {
        static_assert((4U == sizeof(VAR_TYPE)) || (8U == sizeof(VAR_TYPE)),
                      "DeserializeFixedArrayNoTag only supports 32 and 64 bit values.");

#if EMBEDDED_PROTO_LITTLE_ENDIAN
        // The little-endian wire layout equals the in-memory representation, so
        // the whole block can be popped in one call.
        auto* const raw = reinterpret_cast<uint8_t*>(dest);
        const uint32_t n_bytes = count * static_cast<uint32_t>(sizeof(VAR_TYPE));
        return buffer.pop(raw, n_bytes) ? Error::NO_ERRORS : Error::END_OF_BUFFER;
#else
        // Big-endian fallback: read every value from its little-endian byte order.
        using UINT_TYPE = typename std::conditional<4U == sizeof(VAR_TYPE),
                                                    uint32_t, uint64_t>::type;
        Error return_value = Error::NO_ERRORS;
        for(uint32_t i = 0; (i < count) && (Error::NO_ERRORS == return_value); ++i)
        {
          UINT_TYPE bits = 0;
          return_value = DeserializeFixed(buffer, bits);
          if(Error::NO_ERRORS == return_value)
          {
            memcpy(dest + i, &bits, sizeof(VAR_TYPE));
          }
        }
        return return_value;
#endif
      }

      static Error DeserializeBool(ReadBufferInterface& buffer, bool& value)
      {
        uint8_t byte;
        Error result = Error::NO_ERRORS;
        if(buffer.peek(byte))
        {
          value = static_cast<bool>(byte);
          buffer.advance();
        }
        else 
        {
          result = Error::END_OF_BUFFER;
        }
        return result;
      }

      template<class ENUM_TYPE>
      static Error DeserializeEnum(ReadBufferInterface& buffer, ENUM_TYPE& value) 
      {
        static_assert(std::is_enum<ENUM_TYPE>::value, "No enum given to DeserializeEnum parameter value.");
        uint64_t temp_value;
        Error result = DeserializeVarint(buffer, temp_value);
        if(Error::NO_ERRORS == result)
        {
          value = static_cast<ENUM_TYPE>(temp_value);
        }
        return result;
      }


      /** @} **/


      //! This function converts a given value unsigned integer to a varint formatted data buffer.
      /*!
        \param[in] value  The data to be serialized, uint32_t or uint64_t.
        \param[in] buffer A reference to a message buffer object in which to store the variable.
        \return A value from the Error enum, NO_ERROR in case everything is fine.
      */
      template<class UINT_TYPE>
      static Error SerializeVarint(UINT_TYPE value, WriteBufferInterface& buffer) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, 
                      "Wrong type passed to SerializeVarint.");

        bool memory_free = true;
        while((value >= VARINT_MSB_BYTE) && memory_free) 
        {
          memory_free = buffer.push(static_cast<uint8_t>(value | VARINT_MSB_BYTE));
          value >>= VARINT_SHIFT_N_BITS;
        }
        memory_free = buffer.push(static_cast<uint8_t>(value));

        const Error return_value = memory_free ? Error::NO_ERRORS : Error::BUFFER_FULL;
        return return_value;
      }

      //! This function deserializes the following N bytes into a varint.
      /*!
        \param[in] buffer The data buffer from which bytes are popped.
        \param[out] value The variable in which the varint is returned.
        \return A value from the Error enum, NO_ERROR in case everything is fine.
      */
      template<class UINT_TYPE>
      static Error DeserializeVarint(ReadBufferInterface& buffer, UINT_TYPE& value) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, 
                      "Wrong type passed to DeserializeVarint.");
        
        // Calculate how many bytes there are in a varint 128 base encoded number. This should 
        // yield 5 for a 32bit number and 10 for a 64bit number.
        constexpr auto N_DIGITS = std::numeric_limits<UINT_TYPE>::digits;
        constexpr auto N_BITS_FLOAT = static_cast<float>(VARINT_SHIFT_N_BITS);
        constexpr auto DIV_RESULT = N_DIGITS / N_BITS_FLOAT;
        constexpr auto DIV_CEIL = constexpr_ceil(DIV_RESULT);
        constexpr auto N_BYTES_IN_VARINT = static_cast<uint8_t>(DIV_CEIL);
        
        UINT_TYPE temp_value = 0;
        uint8_t byte = 0;
        uint8_t i = 0;
        bool result = false;
        do 
        {
          result = buffer.peek(i, byte);
          if(result) 
          {
            temp_value |= static_cast<UINT_TYPE>(byte & (~VARINT_MSB_BYTE)) << (i * VARINT_SHIFT_N_BITS);
          }
          ++i;
        } while((byte & VARINT_MSB_BYTE) && (i < N_BYTES_IN_VARINT) && result);

        Error return_value = Error::NO_ERRORS;
        if(result)
        {
          if(byte & VARINT_MSB_BYTE)
          {
            // This varint was not closed properly.
            return_value = Error::OVERLONG_VARINT;
          }
          else
          {
            // All is well.
            value = temp_value;
          }
          // In any case advance the buffer.
          buffer.advance(i);
        }
        else 
        {
          return_value = Error::END_OF_BUFFER;
        }

        return return_value;
      }


  };

} // End of namespace EmbeddedProto.
#endif
