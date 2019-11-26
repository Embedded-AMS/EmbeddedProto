
#include "Fields.h"
#include "MessageSizeCalculator.h"

namespace EmbeddedProto 
{
  uint32_t Field::serialized_size() const
  {
    ::EmbeddedProto::MessageSizeCalculator calcBuffer;
    this->serialize(calcBuffer);
    return calcBuffer.get_size();
  }

  bool int32::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool int64::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool uint32::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool uint64::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool sint32::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool sint64::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool boolean::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::VARINT), buffer) && serialize(buffer);
  }

  bool fixed32::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED32), buffer) && serialize(buffer);
  }

  bool fixed64::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED64), buffer) && serialize(buffer);
  }

  bool sfixed32::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED32), buffer) && serialize(buffer);
  }

  bool sfixed64::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED64), buffer) && serialize(buffer);
  }

  bool floatfixed::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED32), buffer) && serialize(buffer);
  }

  bool doublefixed::serialize(uint32_t field_number, WriteBufferInterface& buffer) const
  { 
    return WireFormatter::SerializeVarint(WireFormatter::MakeTag(field_number, WireFormatter::WireType::FIXED64), buffer) && serialize(buffer);
  }



  bool int32::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerializeVarint(static_cast<uint32_t>(get()), buffer);
  }

  bool int64::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerializeVarint(static_cast<uint64_t>(get()), buffer);
  }

  bool uint32::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerializeVarint(get(), buffer);
  }

  bool uint64::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerializeVarint(get(), buffer);
  }

  bool sint32::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerializeVarint(WireFormatter::ZigZagEncode(get()), buffer);
  }

  bool sint64::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerializeVarint(WireFormatter::ZigZagEncode(get()), buffer);
  }

  bool boolean::serialize(WriteBufferInterface& buffer) const
  {
    return buffer.push(get() ? 0x01 : 0x00);
  }

  bool fixed32::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerialzieFixedNoTag(get(), buffer);
  }

  bool fixed64::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerialzieFixedNoTag(get(), buffer);
  }

  bool sfixed32::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerialzieSFixedNoTag(get(), buffer);
  }

  bool sfixed64::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerialzieSFixedNoTag(get(), buffer);
  }

  bool floatfixed::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerialzieFloatNoTag(get(), buffer);
  }

  bool doublefixed::serialize(WriteBufferInterface& buffer) const
  {
    return WireFormatter::SerialzieDoubleNoTag(get(), buffer);
  }



  bool int32::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeInt(buffer, get());
  }

  bool int64::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeInt(buffer, get());
  }

  bool uint32::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeUInt(buffer, get());
  }

  bool uint64::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeUInt(buffer, get());
  }

  bool sint32::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeSInt(buffer, get());
  }

  bool sint64::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeSInt(buffer, get());
  }

  bool boolean::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeBool(buffer, get());
  }

  bool fixed32::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeFixed(buffer, get());
  }

  bool fixed64::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeFixed(buffer, get());
  }

  bool sfixed32::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeSFixed(buffer, get());
  }

  bool sfixed64::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeSFixed(buffer, get());
  }

  bool floatfixed::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeFloat(buffer, get());
  }

  bool doublefixed::deserialize(ReadBufferInterface& buffer) 
  { 
    return WireFormatter::DeserializeDouble(buffer, get());
  }
  
} // End of namespace EmbeddedProto
