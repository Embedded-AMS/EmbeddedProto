{% macro enum_macro(_enum) %}
enum {{ _enum.name }}
{
  {% for value in _enum.values() %}
  {{ value.name }} = {{ value.number }}{{ "," if not loop.last }}
  {% endfor %}
}
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
    void clear_{{field.name}}() { {{field.variable_name}}.clear(); };
    {% else %}
    void clear_{{field.name}}() { {{field.variable_name}} = {{field.default_value}}; };
    {% endif %}
    void set_{{field.name}}(const {{field.type}}& value) { {{field.variable_name}} = value; }
    const {{field.type}}& get_{{field.name}}() const { return {{field.variable_name}}; }
    {% if field.of_type_message %}
    {{field.type}}& mutable_{{field.name}}() { return {{field.variable_name}}; }
    {% endif %}

    {% endfor %}
    bool serialize(::EmbeddedProto::Buffer& buffer) const final
    {
      {% for field in msg.fields() %}
      {% if field.of_type_message %}
      const uint32_t size_{{field.name}} = {{field.variable_name}}.serialized_size();
      if(0 < size_{{field.name}} && buffer.good())
      {
        serialize_tag({{field.variable_id_name}}, ::EmbeddedProto::WireType::{{field.wire_type}}, buffer);
        serialize_VARINT(size_{{field.name}}, buffer);
        {{field.variable_name}}.serialize(buffer);
      }
      {% else %}
      if(({{field.default_value}} != {{field.variable_name}}) && buffer.good())
      {
        serialize_tag({{field.variable_id_name}}, ::EmbeddedProto::WireType::{{field.wire_type}}, buffer);
        serialize_field({{field.variable_name}}, buffer);
      }
      {% endif %}

      {% endfor %}
      return buffer.good();
    };

    bool deserialize(::EmbeddedProto::Buffer& buffer) final
    {
      bool result = True;
      ::EmbeddedProto::WireType wire_type;
      uint32_t id_number = 0;

      while(result && buffer.good() && deserialize_tag(buffer, wire_type, id_number))
      {
        switch(id_number)
        {
          {% for field in msg.fields() %}
          case {{field.variable_id_name}}:
          {
            if(::EmbeddedProto::WireType::{{field.wire_type}} == wire_type)
            {
              {% if field.of_type_message %}
              uint32_t size;
              result = deserialize_VARINT(buffer, size);
              PartialBuffer<size> partial_buffer(buffer);
              result = result && {{field.variable_name}}.deserialize(partial_buffer);
              {% else %}
              result = deserialized_size_{{field.wire_type}}(buffer, {{field.variable_name}});
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
      uint32_t size = 0;
      {% for field in msg.fields() %}
      {% if field.of_type_message %}
      size += {{field.variable_name}}.serialized_size();
      {% else %}
      size += serialized_size_{{field.wire_type}}({{field.variable_name}});
      {% endif %}
      {% endfor %}
      return size;
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
{% if messages %}#include <MessageInterface.h>{% endif %}

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
