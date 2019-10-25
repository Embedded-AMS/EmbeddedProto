
#include "Fields.h"


namespace EmbeddedProto 
{


  bool serialize(uint32_t field_number, const int32& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const int64& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const uint32& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const uint64& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const sint32& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const sint64& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const boolean& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const fixed32& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED32), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const fixed64& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED64), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const sfixed32& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED32), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const sfixed64& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED64), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const floatfixed& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED32), buffer) && serialize(x, buffer);
  }

  bool serialize(uint32_t field_number, const doublefixed& x, WriteBufferInterface& buffer) 
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED64), buffer) && serialize(x, buffer);
  }



  bool serialize(const int32& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerializeVarint(static_cast<uint32_t>(x.get()), buffer);
  }

  bool serialize(const int64& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerializeVarint(static_cast<uint64_t>(x.get()), buffer);
  }

  bool serialize(const uint32& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerializeVarint(x.get(), buffer);
  }

  bool serialize(const uint64& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerializeVarint(x.get(), buffer);
  }

  bool serialize(const sint32& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerializeVarint(WireFormatter::ZigZagEncode(x.get()), buffer);
  }

  bool serialize(const sint64& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerializeVarint(WireFormatter::ZigZagEncode(x.get()), buffer);
  }

  bool serialize(const boolean& x, WriteBufferInterface& buffer)
  {
    return buffer.push(x.get() ? 0x01 : 0x00);
  }

  bool serialize(const fixed32& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerialzieFixedNoTag(x.get(), buffer);
  }

  bool serialize(const fixed64& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerialzieFixedNoTag(x.get(), buffer);
  }

  bool serialize(const sfixed32& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerialzieSFixedNoTag(x.get(), buffer);
  }

  bool serialize(const sfixed64& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerialzieSFixedNoTag(x.get(), buffer);
  }

  bool serialize(const floatfixed& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerialzieFloatNoTag(x.get(), buffer);
  }

  bool serialize(const doublefixed& x, WriteBufferInterface& buffer)
  {
    return WireFormatter::SerialzieDoubleNoTag(x.get(), buffer);
  }



  bool deserialize(ReadBufferInterface& buffer, int32& x) 
  { 
    return WireFormatter::DeserializeInt(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, int64& x) 
  { 
    return WireFormatter::DeserializeInt(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, uint32& x) 
  { 
    return WireFormatter::DeserializeUInt(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, uint64& x) 
  { 
    return WireFormatter::DeserializeUInt(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, sint32& x) 
  { 
    return WireFormatter::DeserializeSInt(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, sint64& x) 
  { 
    return WireFormatter::DeserializeSInt(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, boolean& x) 
  { 
    return WireFormatter::DeserializeBool(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, fixed32& x) 
  { 
    return WireFormatter::DeserializeFixed(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, fixed64& x) 
  { 
    return WireFormatter::DeserializeFixed(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, sfixed32& x) 
  { 
    return WireFormatter::DeserializeSFixed(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, sfixed64& x) 
  { 
    return WireFormatter::DeserializeSFixed(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, floatfixed& x) 
  { 
    return WireFormatter::DeserializeFloat(buffer, x.get());
  }

  bool deserialize(ReadBufferInterface& buffer, doublefixed& x) 
  { 
    return WireFormatter::DeserializeDouble(buffer, x.get());
  }
  
} // End of namespace EmbeddedProto
