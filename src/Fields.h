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
      Field();
      virtual ~Field() = default;
  };

  template<class TYPE>
  class FieldTemplate : public Field
  {
    public:
      FieldTemplate() = default;
      ~FieldTemplate() override = default;

      void set(const TYPE& v) { value_ = v; }      
      void set(const TYPE&& v) { value_ = v; }
      void operator=(const TYPE& v) { value_ = v; }
      void operator=(const TYPE&& v) { value_ = v; }

      TYPE& get() const { return value_; }
      TYPE& get() { return value_; }

    private:
      TYPE value_;
  };

  class int32 : public FieldTemplate<int32_t> { };      
  class int64 : public FieldTemplate<int64_t> { };      
  class uint32 : public FieldTemplate<uint32_t> { };    
  class uint64 : public FieldTemplate<uint64_t> { };    
  class sint32 : public FieldTemplate<uint32_t> { };    
  class sint64 : public FieldTemplate<uint64_t> { };    
  class boolean : public FieldTemplate<bool> { };
  // TODO enum
  class fixed32 : public FieldTemplate<uint32_t> { };   
  class fixed64 : public FieldTemplate<uint64_t> { };   
  class sfixed32 : public FieldTemplate<int32_t> { };   
  class sfixed64 : public FieldTemplate<int64_t> { };   
  class floatfixed : public FieldTemplate<float> { };   
  class doublefixed : public FieldTemplate<double> { }; 

  bool serialize(uint32_t field_number, int32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, int64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, uint32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeUInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, uint64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeUInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, sint32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, sint64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSInt(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, bool x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeBool(field_number, x, buffer); }

  bool serialize(uint32_t field_number, fixed32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, fixed64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, sfixed32& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, sfixed64& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeSFixed(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, floatfixed& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeFloat(field_number, x.get(), buffer); }

  bool serialize(uint32_t field_number, doublefixed& x, WriteBufferInterface& buffer) 
  { return WireFormatter::SerializeDouble(field_number, x.get(), buffer); }



  bool deserialize(int32& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeInt(buffer, x.get()); }

  bool deserialize(int64& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeInt(buffer, x.get()); }

  bool deserialize(uint32& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeUInt(buffer, x.get()); }

  bool deserialize(uint64& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeUInt(buffer, x.get()); }

  bool deserialize(sint32& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeSInt(buffer, x.get()); }

  bool deserialize(sint64& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeSInt(buffer, x.get()); }

  bool deserialize(bool x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeBool(buffer, x); }

  bool deserialize(fixed32& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeFixed(buffer, x.get()); }

  bool deserialize(fixed64& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeFixed(buffer, x.get()); }

  bool deserialize(sfixed32& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeSFixed(buffer, x.get()); }

  bool deserialize(sfixed64& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeSFixed(buffer, x.get()); }

  bool deserialize(floatfixed& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeFloat(buffer, x.get()); }

  bool deserialize(doublefixed& x, ReadBufferInterface& buffer) 
  { return WireFormatter::DeserializeDouble(buffer, x.get()); }

} // End of namespace EmbeddedProto.
#endif