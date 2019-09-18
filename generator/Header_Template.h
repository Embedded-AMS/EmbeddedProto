{% macro enum_macro(_enum) %}
enum {{ _enum.name }}
{
  {% for value in _enum.values() %}
  {{ value.name }} = {{ value.number }}{{ "," if not loop.last }}
  {% endfor %}
};
{% endmacro %}

{% macro msg_macro(msg) %}
class {{ msg.name }} final: public ::EmbeddedProto::MessageInterface
{
  public:
    {{ msg.name }}() :
        {% for field in msg.fields() %}
        {{field.variable_name}}({{field.default_value}}){{"," if not loop.last}}
        {% endfor %}
    {

    };

    ~{{ msg.name }}() override = default;

    {% for enum in msg.nested_enums() %}
    {{ enum_macro(enum) }}

    {% endfor %}
    {% for field in msg.fields() %}
    static const uint32_t {{field.variable_id_name}} = {{field.variable_id}};
    {% if field.of_type_message %}
    inline void clear_{{field.name}}() { {{field.variable_name}}.clear(); };
    {% else %}
    inline void clear_{{field.name}}() { {{field.variable_name}} = {{field.default_value}}; };
    {% endif %}
    inline void set_{{field.name}}(const {{field.type}}& value) { {{field.variable_name}} = value; }
    inline const {{field.type}}& get_{{field.name}}() const { return {{field.variable_name}}; }
    {% if field.of_type_message %}
    inline {{field.type}}& mutable_{{field.name}}() { return {{field.variable_name}}; }
    {% endif %}

    {% endfor %}
    bool serialize(::EmbeddedProto::MessageBufferInterface& buffer) const final
    {
      bool result = true;

      {% for field in msg.fields() %}
      {% if field.of_type_message %}
      const uint32_t size_{{field.name}} = {{field.variable_name}}.serialized_size();
      result = (size_{{field.name}} <= buffer.get_available_size());
      if(result && (0 < size_{{field.name}}))
      {
        uint32_t tag = ::EmbeddedProto::WireFormatter::MakeTag({{field.variable_id_name}}, ::EmbeddedProto::WireFormatter::WireType::{{field.wire_type}});
        result = ::EmbeddedProto::WireFormatter::SerializeVarint(tag, buffer);
        result = result && ::EmbeddedProto::WireFormatter::SerializeVarint(size_{{field.name}}, buffer);
        result = result && {{field.variable_name}}.serialize(buffer);
      }
      {% else %}
      if(({{field.default_value}} != {{field.variable_name}}) && result)
      {
        result = ::EmbeddedProto::WireFormatter::{{field.serialization_func}}({{field.variable_id_name}}, {{field.variable_name}}, buffer);
      }
      {% endif %}

      {% endfor %}
      return result;
    };

    bool deserialize(::EmbeddedProto::MessageBufferInterface& buffer) final
    {
      bool result = true;
      ::EmbeddedProto::WireFormatter::WireType wire_type;
      uint32_t id_number = 0;

      while(result && ::EmbeddedProto::WireFormatter::DeserializeTag(buffer, wire_type, id_number))
      {
        switch(id_number)
        {
          {% for field in msg.fields() %}
          case {{field.variable_id_name}}:
          {
            if(::EmbeddedProto::WireFormatter::WireType::{{field.wire_type}} == wire_type)
            {
              {% if field.of_type_message %}
              uint32_t size;
              result = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, size);
              result = result && {{field.variable_name}}.deserialize(buffer);
              {% else %}
              result = ::EmbeddedProto::WireFormatter::{{field.deserialization_func}}(buffer, {{field.variable_name}});
              {% endif %}
            }
            else
            {
              // TODO Error wire type does not match field.
              result = false;
            }
            break;
          }

          {% endfor %}
          default:
            break;
        }
      }
      return result;
    };

    void clear() final
    {
      {% for field in msg.fields() %}
      clear_{{field.name}}();
      {% endfor %}
    }

    uint32_t serialized_size() const final
    {
      ::EmbeddedProto::MessageSizeCalculator calcBuffer;
      this->serialize(calcBuffer);
      return calcBuffer.get_size();
    }

  private:

    {% for field in msg.fields() %}
    {{field.type}} {{field.variable_name}};
    {% endfor %}
};
{% endmacro %}
// This file is generated. Please do not edit!
#pragma once

#include <cstdint>
{% if messages %}
#include <MessageInterface.h>
#include <WireFormatter.h>
#include <MessageSizeCalculator.h>
{% endif %}

{% if namespace %}
namespace {{ namespace }}
{ {% endif %}

{% for enum in enums %}
{{ enum_macro(enum) }}
{% endfor %}
{% for msg in messages %}
{{ msg_macro(msg) }}
{% endfor %}
{% if namespace %}
} // End of namespace {{ namespace }}
{% endif %}
