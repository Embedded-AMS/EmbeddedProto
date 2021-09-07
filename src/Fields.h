/*
 *  Copyright (C) 2020-2021 Embedded AMS B.V. - All Rights Reserved
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
 *  <https://EmbeddedProto.com/license/>.
 *
 *  Embedded AMS B.V.
 *  Info:
 *    info at EmbeddedProto dot com
 *
 *  Postal address:
 *    Johan Huizingalaan 763a
 *    1066 VH, Amsterdam
 *    the Netherlands
 */

#ifndef _FIELDS_H_
#define _FIELDS_H_

#include "Errors.h"
#include "WireFormatter.h"
#include "WriteBufferInterface.h"
#include "ReadBufferInterface.h"

#include <cstdint>


namespace EmbeddedProto 
{

  class Field 
  {
    public:
      Field() = default;
      virtual ~Field() = default;

      virtual Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const = 0;

      virtual Error serialize(WriteBufferInterface& buffer) const = 0;

      //! Deserialize this field from the bytes in the given buffer.
      /*!
        \param buffer data from which the field should be obtained.
        \return NO_ERROR if everything went ok.
      */      
      virtual Error deserialize(ReadBufferInterface& buffer) = 0;

      //! Deserialize this field but also check if given wire type matches the field type.
      /*!
        \param buffer data from which the field should be obtained.
        \param wire_type The wire type obtained from the tag used to match with this field type.
        \return NO_ERROR if everything went ok.
      */
      virtual Error deserialize_check_type(ReadBufferInterface& buffer, 
                                           const ::EmbeddedProto::WireFormatter::WireType& wire_type) = 0;

      //! Calculate the size of this message when serialized.
      /*!
          \return The number of bytes this message will require once serialized.
      */
      uint32_t serialized_size() const;

      //! Reset the field to it's initial value.
      virtual void clear() = 0;
  };

  template<class TYPE, WireFormatter::WireType WIRETYPE>
  class FieldTemplate : public Field
  {
    public:
      using FIELD_TYPE = TYPE;

      FieldTemplate() = default;
      explicit FieldTemplate(const TYPE& v) : value_(v) { };
      explicit FieldTemplate(const TYPE&& v) : value_(v) { };
      explicit FieldTemplate(const FieldTemplate<TYPE, WIRETYPE>& ft) : value_(ft.value_) { };
      ~FieldTemplate() override = default;

      //! \see Field::deserialize()
      Error deserialize_check_type(ReadBufferInterface& buffer, 
                                   const ::EmbeddedProto::WireFormatter::WireType& wire_type) final
      {
        Error return_value = WIRETYPE == wire_type ? Error::NO_ERRORS : Error::INVALID_WIRETYPE;
        if(Error::NO_ERRORS == return_value) 
        {
          return_value = deserialize(buffer);
        }
        return return_value;
      }

      void set(const TYPE& v) { value_ = v; }      
      void set(const TYPE&& v) { value_ = v; }
      void set(const FieldTemplate<TYPE, WIRETYPE>& ft) { value_ = ft.value_; }
      void set(const FieldTemplate<TYPE, WIRETYPE>&& ft) { value_ = ft.value_; }
      FieldTemplate<TYPE, WIRETYPE>& operator=(const TYPE& v) 
      { 
        value_ = v;
        return *this;
      }
      FieldTemplate<TYPE, WIRETYPE>& operator=(const TYPE&& v) 
      { 
        value_ = v;
        return *this;
      }
      FieldTemplate<TYPE, WIRETYPE>& operator=(const FieldTemplate<TYPE, WIRETYPE>& ft)
      { 
        value_ = ft.value_; 
        return *this; 
      }
      FieldTemplate<TYPE, WIRETYPE>& operator=(const FieldTemplate<TYPE, WIRETYPE>&& ft) noexcept
      { 
        value_ = ft.value_;
        return *this;
      }

      const TYPE& get() const { return value_; }
      TYPE& get() { return value_; }

      //! This is the conversion operator. 
      /*! 
        Sonar would like this to be explicit but this is not practial in normal usage with other 
        integer and floating point types.
      */
      operator TYPE() const { return value_; } //NOSONAR

      bool operator==(const TYPE& rhs) { return value_ == rhs; }
      bool operator!=(const TYPE& rhs) { return value_ != rhs; }
      bool operator>(const TYPE& rhs) { return value_ > rhs; }
      bool operator<(const TYPE& rhs) { return value_ < rhs; }
      bool operator>=(const TYPE& rhs) { return value_ >= rhs; }
      bool operator<=(const TYPE& rhs) { return value_ <= rhs; }

      template<class TYPE_RHS, WireFormatter::WireType WIRETYPE_RHS>
      bool operator==(const FieldTemplate<TYPE_RHS, WIRETYPE_RHS>& rhs) { return value_ == rhs.get(); }
      template<class TYPE_RHS, WireFormatter::WireType WIRETYPE_RHS>
      bool operator!=(const FieldTemplate<TYPE_RHS, WIRETYPE_RHS>& rhs) { return value_ != rhs.get(); }
      template<class TYPE_RHS, WireFormatter::WireType WIRETYPE_RHS>
      bool operator>(const FieldTemplate<TYPE_RHS, WIRETYPE_RHS>& rhs) { return value_ > rhs.get(); }
      template<class TYPE_RHS, WireFormatter::WireType WIRETYPE_RHS>
      bool operator<(const FieldTemplate<TYPE_RHS, WIRETYPE_RHS>& rhs) { return value_ < rhs.get(); }
      template<class TYPE_RHS, WireFormatter::WireType WIRETYPE_RHS>
      bool operator>=(const FieldTemplate<TYPE_RHS, WIRETYPE_RHS>& rhs) { return value_ >= rhs.get(); }
      template<class TYPE_RHS, WireFormatter::WireType WIRETYPE_RHS>
      bool operator<=(const FieldTemplate<TYPE_RHS, WIRETYPE_RHS>& rhs) { return value_ <= rhs.get(); }

      void clear() override { value_ = static_cast<TYPE>(0); }

    private:

      TYPE value_;
  };

  class int32 : public FieldTemplate<int32_t, WireFormatter::WireType::VARINT> 
  { 
    public: 
      int32() : FieldTemplate<int32_t, WireFormatter::WireType::VARINT>(0) {};
      int32(const int32_t& v) : FieldTemplate<int32_t, WireFormatter::WireType::VARINT>(v) {};
      int32(const int32_t&& v) : FieldTemplate<int32_t, WireFormatter::WireType::VARINT>(v) {};

      ~int32() override = default;

      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class int64 : public FieldTemplate<int64_t, WireFormatter::WireType::VARINT> 
  { 
    public: 
      int64() : FieldTemplate<int64_t, WireFormatter::WireType::VARINT>(0) {};
      int64(const int64_t& v) : FieldTemplate<int64_t, WireFormatter::WireType::VARINT>(v) {};
      int64(const int64_t&& v) : FieldTemplate<int64_t, WireFormatter::WireType::VARINT>(v) {};

      ~int64() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class uint32 : public FieldTemplate<uint32_t, WireFormatter::WireType::VARINT> 
  { 
    public: 
      uint32() : FieldTemplate<uint32_t, WireFormatter::WireType::VARINT>(0) {};
      uint32(const uint32_t& v) : FieldTemplate<uint32_t, WireFormatter::WireType::VARINT>(v) {};
      uint32(const uint32_t&& v) : FieldTemplate<uint32_t, WireFormatter::WireType::VARINT>(v) {};

      ~uint32() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class uint64 : public FieldTemplate<uint64_t, WireFormatter::WireType::VARINT> 
  { 
    public: 
      uint64() : FieldTemplate<uint64_t, WireFormatter::WireType::VARINT>(0) {};
      uint64(const uint64_t& v) : FieldTemplate<uint64_t, WireFormatter::WireType::VARINT>(v) {};
      uint64(const uint64_t&& v) : FieldTemplate<uint64_t, WireFormatter::WireType::VARINT>(v) {};

      ~uint64() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffe) final; 
  };

  class sint32 : public FieldTemplate<int32_t, WireFormatter::WireType::VARINT> 
  { 
    public: 
      sint32() : FieldTemplate<int32_t, WireFormatter::WireType::VARINT>(0) {};
      sint32(const int32_t& v) : FieldTemplate<int32_t, WireFormatter::WireType::VARINT>(v) {};
      sint32(const int32_t&& v) : FieldTemplate<int32_t, WireFormatter::WireType::VARINT>(v) {};

      ~sint32() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class sint64 : public FieldTemplate<int64_t, WireFormatter::WireType::VARINT> 
  { 
    public: 
      sint64() : FieldTemplate<int64_t, WireFormatter::WireType::VARINT>(0) {};
      sint64(const int64_t& v) : FieldTemplate<int64_t, WireFormatter::WireType::VARINT>(v) {};
      sint64(const int64_t&& v) : FieldTemplate<int64_t, WireFormatter::WireType::VARINT>(v) {};

      ~sint64() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class boolean : public FieldTemplate<bool, WireFormatter::WireType::VARINT> 
  { 
    public: 
      boolean() : FieldTemplate<bool, WireFormatter::WireType::VARINT>(false) {};
      boolean(const bool& v) : FieldTemplate<bool, WireFormatter::WireType::VARINT>(v) {};
      boolean(const bool&& v) : FieldTemplate<bool, WireFormatter::WireType::VARINT>(v) {};

      ~boolean() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class fixed32 : public FieldTemplate<uint32_t, WireFormatter::WireType::FIXED32> 
  { 
    public: 
      fixed32() : FieldTemplate<uint32_t, WireFormatter::WireType::FIXED32>(0) {};
      fixed32(const uint32_t& v) : FieldTemplate<uint32_t, WireFormatter::WireType::FIXED32>(v) {};
      fixed32(const uint32_t&& v) : FieldTemplate<uint32_t, WireFormatter::WireType::FIXED32>(v) {};

      ~fixed32() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class fixed64 : public FieldTemplate<uint64_t, WireFormatter::WireType::FIXED64> 
  { 
    public: 
      fixed64() : FieldTemplate<uint64_t, WireFormatter::WireType::FIXED64>(0) {};
      fixed64(const uint64_t& v) : FieldTemplate<uint64_t, WireFormatter::WireType::FIXED64>(v) {};
      fixed64(const uint64_t&& v) : FieldTemplate<uint64_t, WireFormatter::WireType::FIXED64>(v) {};

      ~fixed64() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class sfixed32 : public FieldTemplate<int32_t, WireFormatter::WireType::FIXED32> 
  { 
    public: 
      sfixed32() : FieldTemplate<int32_t, WireFormatter::WireType::FIXED32>(0) {};
      sfixed32(const int32_t& v) : FieldTemplate<int32_t, WireFormatter::WireType::FIXED32>(v) {};
      sfixed32(const int32_t&& v) : FieldTemplate<int32_t, WireFormatter::WireType::FIXED32>(v) {};

      ~sfixed32() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class sfixed64 : public FieldTemplate<int64_t, WireFormatter::WireType::FIXED64> 
  { 
    public: 
      sfixed64() : FieldTemplate<int64_t, WireFormatter::WireType::FIXED64>(0) {};
      sfixed64(const int64_t& v) : FieldTemplate<int64_t, WireFormatter::WireType::FIXED64>(v) {};
      sfixed64(const int64_t&& v) : FieldTemplate<int64_t, WireFormatter::WireType::FIXED64>(v) {};

      ~sfixed64() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class floatfixed : public FieldTemplate<float, WireFormatter::WireType::FIXED32> 
  { 
    public: 
      floatfixed() : FieldTemplate<float, WireFormatter::WireType::FIXED32>(0.0F) {};
      floatfixed(const float& v) : FieldTemplate<float, WireFormatter::WireType::FIXED32>(v) {};
      floatfixed(const float&& v) : FieldTemplate<float, WireFormatter::WireType::FIXED32>(v) {};

      ~floatfixed() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

  class doublefixed : public FieldTemplate<double, WireFormatter::WireType::FIXED64> 
  { 
    public: 
      doublefixed() : FieldTemplate<double, WireFormatter::WireType::FIXED64>(0.0) {};
      doublefixed(const double& v) : FieldTemplate<double, WireFormatter::WireType::FIXED64>(v) {};
      doublefixed(const double&& v) : FieldTemplate<double, WireFormatter::WireType::FIXED64>(v) {};

      ~doublefixed() override = default;
      
      Error serialize_with_id(uint32_t field_number, WriteBufferInterface& buffer, const bool optional) const final;
      Error serialize(WriteBufferInterface& buffer) const final;
      Error deserialize(ReadBufferInterface& buffer) final; 
  };

} // End of namespace EmbeddedProto.
#endif
