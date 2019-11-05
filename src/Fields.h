#ifndef _FIELDS_H_
#define _FIELDS_H_

#include <WireFormatter.h>
#include <WriteBufferInterface.h>
#include <ReadBufferInterface.h>

namespace EmbeddedProto 
{


  class Field 
  {
    public:
      Field() = default;
      virtual ~Field() = default;
  };

  template<class TYPE>
  class FieldTemplate : public Field
  {
    public:
      typedef TYPE FIELD_TYPE;

      FieldTemplate() = default;
      FieldTemplate(const TYPE& v) : value_(v) { };
      FieldTemplate(const TYPE&& v) : value_(v) { };
      ~FieldTemplate() override = default;

      void set(const TYPE& v) { value_ = v; }      
      void set(const TYPE&& v) { value_ = v; }
      void operator=(const TYPE& v) { value_ = v; }
      void operator=(const TYPE&& v) { value_ = v; }

      const TYPE& get() const { return value_; }
      TYPE& get() { return value_; }

      operator TYPE() const { return value_; }

      bool operator==(const TYPE& rhs) { return value_ == rhs; }
      bool operator!=(const TYPE& rhs) { return value_ != rhs; }
      bool operator>(const TYPE& rhs) { return value_ > rhs; }
      bool operator<(const TYPE& rhs) { return value_ < rhs; }
      bool operator>=(const TYPE& rhs) { return value_ >= rhs; }
      bool operator<=(const TYPE& rhs) { return value_ <= rhs; }

      template<class TYPE_RHS>
      bool operator==(const FieldTemplate<TYPE_RHS>& rhs) { return value_ == rhs.get(); }
      template<class TYPE_RHS>
      bool operator!=(const FieldTemplate<TYPE_RHS>& rhs) { return value_ != rhs.get(); }
      template<class TYPE_RHS>
      bool operator>(const FieldTemplate<TYPE_RHS>& rhs) { return value_ > rhs.get(); }
      template<class TYPE_RHS>
      bool operator<(const FieldTemplate<TYPE_RHS>& rhs) { return value_ < rhs.get(); }
      template<class TYPE_RHS>
      bool operator>=(const FieldTemplate<TYPE_RHS>& rhs) { return value_ >= rhs.get(); }
      template<class TYPE_RHS>
      bool operator<=(const FieldTemplate<TYPE_RHS>& rhs) { return value_ <= rhs.get(); }

    private:

      TYPE value_;
  };


  class int32 : public FieldTemplate<int32_t> 
  { 
    public: 
      int32() : FieldTemplate<int32_t>(0) {};
      int32(const int32_t& v) : FieldTemplate<int32_t>(v) {};
      int32(const int32_t&& v) : FieldTemplate<int32_t>(v) {};

      ~int32() override = default;
  };

  class int64 : public FieldTemplate<int64_t> 
  { 
    public: 
      int64() : FieldTemplate<int64_t>(0) {};
      int64(const int64_t& v) : FieldTemplate<int64_t>(v) {};
      int64(const int64_t&& v) : FieldTemplate<int64_t>(v) {};

      ~int64() override = default;
  };

  class uint32 : public FieldTemplate<uint32_t> 
  { 
    public: 
      uint32() : FieldTemplate<uint32_t>(0) {};
      uint32(const uint32_t& v) : FieldTemplate<uint32_t>(v) {};
      uint32(const uint32_t&& v) : FieldTemplate<uint32_t>(v) {};

      ~uint32() override = default;
  };

  class uint64 : public FieldTemplate<uint64_t> 
  { 
    public: 
      uint64() : FieldTemplate<uint64_t>(0) {};
      uint64(const uint64_t& v) : FieldTemplate<uint64_t>(v) {};
      uint64(const uint64_t&& v) : FieldTemplate<uint64_t>(v) {};

      ~uint64() override = default;
  };

  class sint32 : public FieldTemplate<int32_t> 
  { 
    public: 
      sint32() : FieldTemplate<int32_t>(0) {};
      sint32(const int32_t& v) : FieldTemplate<int32_t>(v) {};
      sint32(const int32_t&& v) : FieldTemplate<int32_t>(v) {};

      ~sint32() override = default;
  };

  class sint64 : public FieldTemplate<int64_t> 
  { 
    public: 
      sint64() : FieldTemplate<int64_t>(0) {};
      sint64(const int64_t& v) : FieldTemplate<int64_t>(v) {};
      sint64(const int64_t&& v) : FieldTemplate<int64_t>(v) {};

      ~sint64() override = default;
  };

  class boolean : public FieldTemplate<bool> 
  { 
    public: 
      boolean() : FieldTemplate<bool>(false) {};
      boolean(const bool& v) : FieldTemplate<bool>(v) {};
      boolean(const bool&& v) : FieldTemplate<bool>(v) {};

      ~boolean() override = default;
  };

  class fixed32 : public FieldTemplate<uint32_t> 
  { 
    public: 
      fixed32() : FieldTemplate<uint32_t>(0) {};
      fixed32(const uint32_t& v) : FieldTemplate<uint32_t>(v) {};
      fixed32(const uint32_t&& v) : FieldTemplate<uint32_t>(v) {};

      ~fixed32() override = default;
  };

  class fixed64 : public FieldTemplate<uint64_t> 
  { 
    public: 
      fixed64() : FieldTemplate<uint64_t>(0) {};
      fixed64(const uint64_t& v) : FieldTemplate<uint64_t>(v) {};
      fixed64(const uint64_t&& v) : FieldTemplate<uint64_t>(v) {};

      ~fixed64() override = default;
  };

  class sfixed32 : public FieldTemplate<int32_t> 
  { 
    public: 
      sfixed32() : FieldTemplate<int32_t>(0) {};
      sfixed32(const int32_t& v) : FieldTemplate<int32_t>(v) {};
      sfixed32(const int32_t&& v) : FieldTemplate<int32_t>(v) {};

      ~sfixed32() override = default;
  };

  class sfixed64 : public FieldTemplate<int64_t> 
  { 
    public: 
      sfixed64() : FieldTemplate<int64_t>(0) {};
      sfixed64(const int64_t& v) : FieldTemplate<int64_t>(v) {};
      sfixed64(const int64_t&& v) : FieldTemplate<int64_t>(v) {};

      ~sfixed64() override = default;
  };

  class floatfixed : public FieldTemplate<float> 
  { 
    public: 
      floatfixed() : FieldTemplate<float>(0.0F) {};
      floatfixed(const float& v) : FieldTemplate<float>(v) {};
      floatfixed(const float&& v) : FieldTemplate<float>(v) {};

      ~floatfixed() override = default;
  };

  class doublefixed : public FieldTemplate<double> 
  { 
    public: 
      doublefixed() : FieldTemplate<double>(0.0) {};
      doublefixed(const double& v) : FieldTemplate<double>(v) {};
      doublefixed(const double&& v) : FieldTemplate<double>(v) {};

      ~doublefixed() override = default;
  };

  bool serialize(uint32_t field_number, const int32& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const int64& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const uint32& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const uint64& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const sint32& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const sint64& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const boolean& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const fixed32& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const fixed64& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const sfixed32& x, WriteBufferInterface& buffer); 
  bool serialize(uint32_t field_number, const sfixed64& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const floatfixed& x, WriteBufferInterface& buffer);
  bool serialize(uint32_t field_number, const doublefixed& x, WriteBufferInterface& buffer);

  bool serialize(const int32& x, WriteBufferInterface& buffer);
  bool serialize(const int64& x, WriteBufferInterface& buffer);
  bool serialize(const uint32& x, WriteBufferInterface& buffer);
  bool serialize(const uint64& x, WriteBufferInterface& buffer);
  bool serialize(const sint32& x, WriteBufferInterface& buffer);
  bool serialize(const sint64& x, WriteBufferInterface& buffer);
  bool serialize(const boolean& x, WriteBufferInterface& buffer);
  bool serialize(const fixed32& x, WriteBufferInterface& buffer);
  bool serialize(const fixed64& x, WriteBufferInterface& buffer);
  bool serialize(const sfixed32& x, WriteBufferInterface& buffer); 
  bool serialize(const sfixed64& x, WriteBufferInterface& buffer);
  bool serialize(const floatfixed& x, WriteBufferInterface& buffer);
  bool serialize(const doublefixed& x, WriteBufferInterface& buffer);

  bool deserialize(ReadBufferInterface& buffer, int32& x); 
  bool deserialize(ReadBufferInterface& buffer, int64& x); 
  bool deserialize(ReadBufferInterface& buffer, uint32& x);
  bool deserialize(ReadBufferInterface& buffer, uint64& x);
  bool deserialize(ReadBufferInterface& buffer, sint32& x);
  bool deserialize(ReadBufferInterface& buffer, sint64& x);
  bool deserialize(ReadBufferInterface& buffer, boolean& x);
  bool deserialize(ReadBufferInterface& buffer, fixed32& x);
  bool deserialize(ReadBufferInterface& buffer, fixed64& x);
  bool deserialize(ReadBufferInterface& buffer, sfixed32& x);
  bool deserialize(ReadBufferInterface& buffer, sfixed64& x);
  bool deserialize(ReadBufferInterface& buffer, floatfixed& x);
  bool deserialize(ReadBufferInterface& buffer, doublefixed& x);

} // End of namespace EmbeddedProto.
#endif