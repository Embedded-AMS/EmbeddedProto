

#ifndef _FIELD_H_
#define _FIELD_H_

#include <cstdint>
#include <limits> 

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
/*
enum class FieldDescriptorProto_Type {
  DOUBLE    = 1,
  FLOAT     = 2,
  INT64     = 3,
  UINT64    = 4,
  INT32     = 5,
  FIXED64   = 6,
  FIXED32   = 7,
  BOOL      = 8,
  STRING    = 9,
  GROUP     = 10,
  MESSAGE   = 11,
  BYTES     = 12,
  UINT32    = 13,
  ENUM      = 14,
  SFIXED32  = 15,
  SFIXED64  = 16,
  SINT32    = 17,
  SINT64    = 18,
};
*/
enum class EncodingType {
  VARINT,
  ZIGZAG,
  FIXED,
  STRING, // ???
  BYTES,  // ??? 
  MESSAGE
}

template<class TYPE, EncodingType ENCODING_TYPE, WireType WIRE_TYPE, uint32_t FIELD_NUMBER, 
          bool REPEATED>
class Field
{
  public:

    //! This typedef will return a unsigned 32 or 64 value depending on the field type.
    typedef typename std::conditional<sizeof(TYPE) <= sizeof(uint32_t), uint32_t, 
                                                                uint64_t>::type VAR_UINT_TYPE;

    //! This typedef will return a signed 32 or 64 value depending on the field type.
    typedef typename std::conditional<sizeof(TYPE) <= sizeof(uint32_t), int32_t, 
                                                                int64_t>::type VAR_INT_TYPE;

    //! Create the tag of a field. 
    /*!
      This is the combination of the field number and wire type of the field. The field number is 
      shifted to the left by three bits. This creates space to or the wire type of the designated 
      field.
    */
    static constexpr uint32_t TAG = ((static_cast<uint32_t>(FIELD_NUMBER) << 3) 
                                      | static_cast<uint32_t>(WIRE_TYPE));


    //! The default constructor without any special initialization.
    Field() = default;
    
    //! The constructor which allows setting the data value to an initial value.
    Field(const TYPE& x) :
      _data(x)
    {

    }

    ~Field() = default;

    const TYPE& get() const
    {
      return _data;
    }

    const TYPE& operator()() const
    {
      return _data;
    }


    Field<TYPE, WIRE_TYPE, FIELD_NUMBER>& operator=(const TYPE& x) 
    {
      _data = x;
      return *this;
    }

    uint8_t* write(uint8_t* target) 
    {
      target = _Varint(TAG, target);
      switch(ENCODING_TYPE) 
      {
        case EncodingType::VARINT:
          target = _Varint(static_cast<VAR_UINT_TYPE>(_data), target);
          break;
        case EncodingType::ZIGZAG:
          target = _Varint(_ZigZagEncode(_data), target);
          break;
        case EncodingType::FIXED:
          // TODO
          break;
        case EncodingType::STRING:
          // TODO
          break;          
        case EncodingType::BYTES:
          // TODO
          break;  
        case EncodingType::MESSAGE:
          // TODO
          break;          
        default:
          break;
      }
      return target;
    }


    // ----------------- PRIVATE -----------------

    //! The actaul data of this field.
    TYPE _data;


    //! This function converts a given value int a varint formated data array.
    /*!
      \param[in] value  The data to be serialized.
      \param[in] target Pointer to the first element of an array in which the data is to be serialized.
      \warning There should be sufficient space in the array to store a varint32.
      \return A pointer to the first byte after the data just serialized.
      This code is copied and modified from google protobuf sources.
    */
    static constexpr uint8_t* _Varint(VAR_UINT_TYPE value, uint8_t* target) {

      constexpr VAR_UINT_TYPE MSB_BYTE = 0x0080;
      constexpr uint8_t SHIFT_N_BITS = 7;

      while(value >= MSB_BYTE) {
        *target = static_cast<uint8_t>(value | MSB_BYTE);
        value >>= SHIFT_N_BITS;
        ++target;
      }
      *target = static_cast<uint8_t>(value);
      return target + 1;
    }    

    //! Encode a signed integer using the zig zag method
    /*!
      As specified the right-shift must be arithmetic, hence the cast is after the shift. The 
      left shift must be unsigned because of overflow.

      \param[in] n The signed value to be encoded.
      \return The zig zag transformed value ready for serialization into the array.
    */
    static constexpr VAR_UINT_TYPE _ZigZagEncode(const VAR_INT_TYPE n) {
      return ((static_cast<VAR_INT_TYPE>(n) << 1) ^ static_cast<VAR_INT_TYPE>(n >> 
                                                      (std::numeric_limits<TYPE>::digits - 1)));
    }

    static constexpr uint8_t* _Fixed(VAR_UINT_TYPE value, uint8_t* target) 
    {
      // Write the data little endian to the array.
      // TODO Define a little endian flag to support memcpy the data to the array.

      uint8_t i = 0;
      while((i*8) < std::numeric_limits<VAR_UINT_TYPE>::digits) 
      {
        target[i] = static_cast<uint8_t>((value >> (i*8)) & 0x00FF);
        ++i;
      }
      return target + i;
    }

    static constexpr uint8_t* _Real(TYPE value, uint8_t* target)
    {
      // Cast the type to void and to an unsigned integer.
      void* pVoid = static_cast<void*>(&value);
      VAR_UINT_TYPE* fixed = static_cast<VAR_UINT_TYPE*>(pVoid);
      return _Fixed(*fixed, target);
    }
};

} // End of namespace EmbeddedProto

#endif 
