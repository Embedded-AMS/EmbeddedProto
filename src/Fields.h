#ifndef _FIELDS_H_
#define _FIELDS_H_

#include "WireFormatter.h"
#include "WriteBufferInterface.h"
#include "ReadBufferInterface.h"

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

    private:
      TYPE value_;
  };

  class int32 : public FieldTemplate<int32_t> { };      
  class int64 : public FieldTemplate<int64_t> { };      
  class uint32 : public FieldTemplate<uint32_t> { };    
  class uint64 : public FieldTemplate<uint64_t> { };    
  class sint32 : public FieldTemplate<int32_t> { };    
  class sint64 : public FieldTemplate<int64_t> { };    
  class boolean : public FieldTemplate<bool> { };
  // TODO enum
  class fixed32 : public FieldTemplate<uint32_t> { };   
  class fixed64 : public FieldTemplate<uint64_t> { };   
  class sfixed32 : public FieldTemplate<int32_t> { };   
  class sfixed64 : public FieldTemplate<int64_t> { };   
  class floatfixed : public FieldTemplate<float> { };   
  class doublefixed : public FieldTemplate<double> { }; 

  bool serialize(uint32_t field_number, const int32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const int64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const uint32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeUInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const uint64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeUInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const sint32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const sint64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const boolean x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeBool(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const fixed32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const fixed64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const sfixed32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const sfixed64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const floatfixed& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeFloat(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, const doublefixed& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeDouble(field_number, x.get(), buffer); }



  bool deserialize(ReadBufferInterface& buffer, int32& x) 
  { return WireFormatter::DeserializeInt(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, int64& x) 
  { return WireFormatter::DeserializeInt(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, uint32& x) 
  { return WireFormatter::DeserializeUInt(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, uint64& x) 
  { return WireFormatter::DeserializeUInt(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, sint32& x) 
  { return WireFormatter::DeserializeSInt(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, sint64& x) 
  { return WireFormatter::DeserializeSInt(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, boolean& x) 
  { return WireFormatter::DeserializeBool(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, fixed32& x) 
  { return WireFormatter::DeserializeFixed(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, fixed64& x) 
  { return WireFormatter::DeserializeFixed(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, sfixed32& x) 
  { return WireFormatter::DeserializeSFixed(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, sfixed64& x) 
  { return WireFormatter::DeserializeSFixed(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, floatfixed& x) 
  { return WireFormatter::DeserializeFloat(buffer, x.get()); }

  bool deserialize(ReadBufferInterface& buffer, doublefixed& x) 
  { return WireFormatter::DeserializeDouble(buffer, x.get()); }

} // End of namespace EmbeddedProto.
#endif