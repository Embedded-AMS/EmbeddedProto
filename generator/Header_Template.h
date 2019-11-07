{% macro enum_macro(_enum) %}
enum {{ _enum.name }}
{
  {% for value in _enum.values() %}
  {{ value.name }} = {{ value.number }}{{ "," if not loop.last }}
  {% endfor %}
};

{% endmacro %}

{% macro msg_macro(msg) %}
{% if msg.templates is defined %}
{% for template in msg.templates %}
{{"template<" if loop.first}}uint32_t {{template}}{{"SIZE, " if not loop.last}}{{"SIZE>" if loop.last}}
{% endfor %}
{% endif %}
class {{ msg.name }} final: public ::EmbeddedProto::MessageInterface
{
  public:
    {{ msg.name }}() :
    {% for field in msg.fields() %}
        {% if field.of_type_enum %}
        {{field.variable_name}}({{field.default_value}}){{"," if not loop.last}}
        {% else %}
        {{field.variable_name}}(){{"," if not loop.last}}
        {% endif %}
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
    inline const {{field.type}}& {{field.name}}() const { return {{field.variable_name}}; }
    inline void clear_{{field.name}}() { {{field.variable_name}}.clear(); }
    inline void set_{{field.name}}(const {{field.type}}& value) { {{field.variable_name}} = value; }
    inline void set_{{field.name}}(const {{field.type}}&& value) { {{field.variable_name}} = value; }
    inline const {{field.type}}& get_{{field.name}}() const { return {{field.variable_name}}; }
    inline {{field.type}}& mutable_{{field.name}}() { return {{field.variable_name}}; }
    {% elif field.of_type_enum %}
    inline {{field.type}} {{field.name}}() const { return {{field.variable_name}}; }
    inline void clear_{{field.name}}() { {{field.variable_name}} = static_cast<{{field.type}}>({{field.default_value}}); }
    inline void set_{{field.name}}(const {{field.type}}& value) { {{field.variable_name}} = value; }
    inline void set_{{field.name}}(const {{field.type}}&& value) { {{field.variable_name}} = value; }
    inline {{field.type}} get_{{field.name}}() const { return {{field.variable_name}}; }
    {% elif field.is_repeated_field %}
    inline const {{field.type}}& {{field.name}}(uint32_t index) const { return {{field.variable_name}}[index]; }
    inline void clear_{{field.name}}() { {{field.variable_name}}.clear(); }
    inline void set_{{field.name}}(uint32_t index, const {{field.type}}& value) { {{field.variable_name}}.set(index, value); }
    inline void set_{{field.name}}(uint32_t index, const {{field.type}}&& value) { {{field.variable_name}}.set(index, value); }
    inline void add_{{field.name}}(const {{field.type}}& value) { {{field.variable_name}}.add(value); }
    inline const {{field.repeated_type}}& get_{{field.name}}() const { return {{field.variable_name}}; }
    inline {{field.repeated_type}}& mutable_{{field.name}}() { return {{field.variable_name}}; }
    {% else %}
    inline {{field.type}}::FIELD_TYPE {{field.name}}() const { return {{field.variable_name}}.get(); }
    inline void clear_{{field.name}}() { {{field.variable_name}}.set({{field.default_value}}); }
    inline void set_{{field.name}}(const {{field.type}}::FIELD_TYPE& value) { {{field.variable_name}}.set(value); }
    inline void set_{{field.name}}(const {{field.type}}::FIELD_TYPE&& value) { {{field.variable_name}}.set(value); }
    inline {{field.type}}::FIELD_TYPE get_{{field.name}}() const { return {{field.variable_name}}.get(); }
    {% endif %}

    {% endfor %}
    bool serialize(::EmbeddedProto::WriteBufferInterface& buffer) const final
    {
      bool result = true;

      {% for field in msg.fields() %}
      {% if field.is_repeated_field %}
      if(result)
      {
        result = {{field.variable_name}}.serialize({{field.variable_id_name}}, buffer);
      }
      {% elif field.of_type_message %}
      if(result)
      {
        const ::EmbeddedProto::MessageInterface* x = &{{field.variable_name}};
        result = x->serialize({{field.variable_id_name}}, buffer);
      }
      {% elif field.of_type_enum %}
      if(({{field.default_value}} != {{field.variable_name}}) && result)
      {
        EmbeddedProto::uint32 value;
        value.set(static_cast<uint32_t>({{field.variable_name}}));
        result = value.serialize({{field.variable_id_name}}, buffer);
      }
      {% else %}
      if(({{field.default_value}} != {{field.variable_name}}.get()) && result)
      {
        result = {{field.variable_name}}.serialize({{field.variable_id_name}}, buffer);
      }
      {% endif %}

      {% endfor %}
      return result;
    };

    bool deserialize(::EmbeddedProto::ReadBufferInterface& buffer) final
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
            {% if field.is_repeated_field %}
            if(::EmbeddedProto::WireFormatter::WireType::LENGTH_DELIMITED == wire_type)
            {
              uint32_t size;
              result = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, size);
              ::EmbeddedProto::ReadBufferSection bufferSection(buffer, size);
              result = result && {{field.variable_name}}.deserialize(bufferSection);
            }
            {% else %}
            if(::EmbeddedProto::WireFormatter::WireType::{{field.wire_type}} == wire_type)
            {
              {% if field.of_type_message %}
              uint32_t size;
              result = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, size);
              ::EmbeddedProto::ReadBufferSection bufferSection(buffer, size);
              result = result && {{field.variable_name}}.deserialize(bufferSection);
              {% elif field.of_type_enum %}
              uint32_t value;
              result = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, value);
              if(result)
              {
                {{field.variable_name}} = static_cast<{{field.type}}>(value);
              }
              {% else %}
              result = {{field.variable_name}}.deserialize(buffer);
              {% endif %}
            }
            {% endif %}
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
    {% if field.is_repeated_field %}
    {{field.repeated_type}} {{field.variable_name}};
    {% else %}
    {{field.type}} {{field.variable_name}};
    {% endif %}
    {% endfor %}
};
{% endmacro %}
// This file is generated. Please do not edit!
#pragma once

#include <cstdint>
{% if messages %}
#include <MessageInterface.h>
#include <WireFormatter.h>
#include <Fields.h>
#include <MessageSizeCalculator.h>
#include <ReadBufferSection.h>
#include <RepeatedField.h>
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
