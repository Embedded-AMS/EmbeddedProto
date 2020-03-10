/*
 *  Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
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
 *  For commercial and closed source application please vist:
 *  <https://EmbeddedProto.com/license/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal adress:
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#ifndef _WIRE_FORMATTER_H_
#define _WIRE_FORMATTER_H_

#include <cstdint>
#include <math.h> 
#include <type_traits>
#include <limits>

#include "WriteBufferInterface.h"
#include "ReadBufferInterface.h"

namespace EmbeddedProto 
{

  //! This class combines functions to serialize and deserialize messages.
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
         @brief Serialize fields, including tags to the given buffer.
         @{
      **/
      template<class INT_TYPE>
      static bool SerializeInt(uint32_t field_number, INT_TYPE value, WriteBufferInterface& buffer)
      {        
        typedef typename std::make_unsigned<INT_TYPE>::type UINT_TYPE;
        return SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer) 
               && SerializeVarint(static_cast<UINT_TYPE>(value), buffer);
      }

      template<class UINT_TYPE>
      static bool SerializeUInt(uint32_t field_number, UINT_TYPE value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer) 
               && SerializeVarint(value, buffer);
      }

      template<class INT_TYPE>
      static bool SerializeSInt(uint32_t field_number, INT_TYPE value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer) 
               && SerializeVarint(ZigZagEncode(value), buffer);
      }
      
      static bool SerializeFixed(uint32_t field_number, uint32_t value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::FIXED32), buffer) 
               && SerialzieFixedNoTag(value, buffer);
      }

      static bool SerializeFixed(uint32_t field_number, uint64_t value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::FIXED64), buffer) 
               && SerialzieFixedNoTag(value, buffer);
      }

      static bool SerializeSFixed(uint32_t field_number, int32_t value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::FIXED32), buffer) 
               && SerialzieSFixedNoTag(value, buffer);
      }

      static bool SerializeSFixed(uint32_t field_number, int64_t value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::FIXED64), buffer) 
               && SerialzieSFixedNoTag(value, buffer);
      }

      static bool SerializeFloat(uint32_t field_number, float value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::FIXED32), buffer) 
               && SerialzieFloatNoTag(value, buffer);
      }

      static bool SerializeDouble(uint32_t field_number, double value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::FIXED64), buffer) 
               && SerialzieDoubleNoTag(value, buffer);
      }

      static bool SerializeBool(uint32_t field_number, bool value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer)
               && buffer.push(value ? 0x01 : 0x00);
      }

      static bool SerializeEnum(uint32_t field_number, uint32_t value, WriteBufferInterface& buffer)
      {
        return SerializeVarint(MakeTag(field_number, WireType::VARINT), buffer) 
               && SerializeVarint(value, buffer);
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
          \return True when deserializing the type and id succeded.  
      */
      static bool DeserializeTag(ReadBufferInterface& buffer, WireType& type, uint32_t& id) 
      {
        uint32_t temp_value;
        
        // Read the next varint considerted to be a tag. Next check the validity of the wire type.
        bool result = DeserializeVarint(buffer, temp_value) && 
                      ((temp_value &  0x07) <= static_cast<uint32_t>(WireType::FIXED32));
        
        // If reading the tag succedded and the wire type is a valid one
        if(result) 
        {
          type = static_cast<WireType>(temp_value &  0x07);
          id = (temp_value >> 3);
        }
        return result;
      }

      template<class UINT_TYPE>
      static bool DeserializeUInt(ReadBufferInterface& buffer, UINT_TYPE& value) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to DeserializeUInt.");
        
        return DeserializeVarint(buffer, value);
      }

      template<class INT_TYPE>
      static bool DeserializeInt(ReadBufferInterface& buffer, INT_TYPE& value) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to DeserializeInt.");
        
        uint64_t uint_value;
        bool result = DeserializeVarint(buffer, uint_value);
        if(result) 
        {
          value = static_cast<INT_TYPE>(uint_value);
        }
        return result;
      }

      template<class INT_TYPE>
      static bool DeserializeSInt(ReadBufferInterface& buffer, INT_TYPE& value) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to DeserializeSInt.");
        
        uint64_t uint_value;
        bool result = DeserializeVarint(buffer, uint_value);
        if(result) 
        {
          value = ZigZagDecode(uint_value);
        }
        return result;
      }

      template<class TYPE>
      static bool DeserializeFixed(ReadBufferInterface& buffer, TYPE& value) 
      {
        static_assert(std::is_same<TYPE, uint32_t>::value || 
                      std::is_same<TYPE, uint64_t>::value, "Wrong type passed to DeserializeFixed.");

          // Deserialize the data little endian to the buffer.
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
      static bool DeserializeSFixed(ReadBufferInterface& buffer, STYPE& value) 
      {
        static_assert(std::is_same<STYPE, int32_t>::value || 
                      std::is_same<STYPE, int64_t>::value, "Wrong type passed to DeserializeSFixed.");

        typedef typename std::make_unsigned<STYPE>::type USTYPE;
        USTYPE temp_unsigned_value = 0;
        bool result = DeserializeFixed(buffer, temp_unsigned_value);
        if(result) {
          value = static_cast<STYPE>(temp_unsigned_value);
        }

        return result;
      }

      static bool DeserializeFloat(ReadBufferInterface& buffer, float& value) 
      {
        uint32_t temp_value = 0;
        bool result = DeserializeFixed(buffer, temp_value);
        if(result) {
          // Cast from unsigned int to a float.
          const void* pVoid = static_cast<const void*>(&temp_value);
          const float* pFloat = static_cast<const float*>(pVoid);
          value = *pFloat;
        }
        return result;
      }

      static bool DeserializeDouble(ReadBufferInterface& buffer, double& value) 
      {
        uint64_t temp_value = 0;
        bool result = DeserializeFixed(buffer, temp_value);
        if(result) {
          // Cast from unsigned int to a double.
          const void* pVoid = static_cast<const void*>(&temp_value);
          const double* pDouble = static_cast<const double*>(pVoid);
          value = *pDouble;
        }
        return result;
      }

      static bool DeserializeBool(ReadBufferInterface& buffer, bool& value) 
      {
        uint8_t byte;
        bool result = buffer.pop(byte);
        if(result) {
          value = static_cast<bool>(byte);
        }
        return result;
      }

      template<class ENUM_TYPE>
      static bool DeserializeEnum(ReadBufferInterface& buffer, ENUM_TYPE& value) 
      {
        static_assert(std::is_enum<ENUM_TYPE>::value, "No enum given to DeserializeEnum parameter value.");
        uint64_t temp_value;
        bool result = DeserializeVarint(buffer, temp_value);
        if(result) {
          value = static_cast<ENUM_TYPE>(temp_value);
        }
        return result;
      }


      /** @} **/


      //! This function converts a given value unsigned integer to a varint formated data buffer.
      /*!
        \param[in] value  The data to be serialized, uint32_t or uint64_t.
        \param[in] buffer A reference to a message buffer object in which to store the variable.
        \return True when it was possible to store the variable in the buffer.
      */
      template<class UINT_TYPE>
      static bool SerializeVarint(UINT_TYPE value, WriteBufferInterface& buffer) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, 
                      "Wrong type passed to SerializeVarint.");

        while (value >= VARINT_MSB_BYTE) {
          buffer.push(static_cast<uint8_t>(value | VARINT_MSB_BYTE));
          value >>= VARINT_SHIFT_N_BITS;
        }
        return buffer.push(static_cast<uint8_t>(value));
      }

      //! This function de serializes the following N bytes into a varint.
      /*!
        \param[in] buffer The data buffer from which bytes are popped.
        \param[out] value The varaible in which the varint is returned.
        \return True when it was possible to desrialize a varint from the buffer.
      */
      template<class UINT_TYPE>
      static bool DeserializeVarint(ReadBufferInterface& buffer, UINT_TYPE& value) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, 
                      "Wrong type passed to DeserializeVarint.");
        
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

      //! Encode a signed integer using the zig zag method
      /*!
        As specified the right-shift must be arithmetic, hence the cast is after the shift. The 
        left shift must be unsigned because of overflow.

        This function is suitable for 32 and 64 bit.

        \param[in] n The signed value to be encoded.
        \return The zig zag transformed value ready for serialization into the array.
      */
      template<class INT_TYPE>
      static constexpr auto ZigZagEncode(const INT_TYPE n) 
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to ZigZagEncode.");

        typedef typename std::make_unsigned<INT_TYPE>::type UINT_TYPE;
        constexpr uint8_t N_BITS_TO_ZIGZAG = std::numeric_limits<UINT_TYPE>::digits - 1;

        return (static_cast<UINT_TYPE>(n) << 1) ^ static_cast<UINT_TYPE>(n >> N_BITS_TO_ZIGZAG);
      }

      //! Decode a signed integer using the zig zag method
      /*!
          \param[in] n The value encoded in zig zag to be deencoded.
          \return The decoded signed value.

          This function is suitable for 32 and 64 bit.
      */
      template<class UINT_TYPE>
      static constexpr auto ZigZagDecode(const UINT_TYPE n) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to ZigZagDecode.");

        typedef typename std::make_signed<UINT_TYPE>::type INT_TYPE;

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
        return ((static_cast<uint32_t>(field_number) << 3) | static_cast<uint32_t>(type));
      }

      /**
         @brief Serialize fields, without tags the given buffer.
         @{
      **/

      //! Serialize an unsigned fixed lenth field without the tag.
      template<class UINT_TYPE>
      static bool SerialzieFixedNoTag(UINT_TYPE value, WriteBufferInterface& buffer) 
      {
        static_assert(std::is_same<UINT_TYPE, uint32_t>::value || 
                      std::is_same<UINT_TYPE, uint64_t>::value, "Wrong type passed to SerialzieFixedNoTag.");

        // Push the data little endian to the buffer.
        // TODO Define a little endian flag to support memcpy the data to the buffer.

        bool result = true;

        // Loop over all bytes in the integer.
        for(uint8_t i = 0; (i < std::numeric_limits<UINT_TYPE>::digits) && result; i += 8) {
          // Shift the value using the current value of i.
          result = buffer.push(static_cast<uint8_t>((value >> i) & 0x00FF));
        }
        return result;
      }

      //! Srialzie a signed fixed length field without the tag.
      template<class INT_TYPE>
      static bool SerialzieSFixedNoTag(INT_TYPE value, WriteBufferInterface& buffer)
      {
        static_assert(std::is_same<INT_TYPE, int32_t>::value || 
                      std::is_same<INT_TYPE, int64_t>::value, "Wrong type passed to SerialzieSFixedNoTag.");

        typedef typename std::make_unsigned<INT_TYPE>::type UINT_TYPE;

        return SerialzieFixedNoTag(static_cast<UINT_TYPE>(value), buffer);
      }

      //! Serialize a 32bit real value without tag.
      static bool SerialzieFloatNoTag(float value, WriteBufferInterface& buffer)
      {
        // Cast the type to void and to a 32 fixed number
        void* pVoid = static_cast<void*>(&value);
        uint32_t* fixed = static_cast<uint32_t*>(pVoid);
        return SerialzieFixedNoTag(*fixed, buffer);
      }

      //! SErialize a 64bit real value without tag.
      static bool SerialzieDoubleNoTag(double value, WriteBufferInterface& buffer)
      {
        // Cast the type to void and to a 64 fixed number
        void* pVoid = static_cast<void*>(&value);
        uint64_t* fixed = static_cast<uint64_t*>(pVoid);
        return SerialzieFixedNoTag(*fixed, buffer);
      }
      /** @} **/

  };

} // End of namespace EmbeddedProto.
#endif
