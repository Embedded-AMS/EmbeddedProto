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

    ~{{ msg.name }}() = default;

    {% for enum in msg.nested_enums() %}
    {{ enum_macro(enum) }}

    {% endfor %}
    {% for field in msg.fields() %}
    static const uint32_t {{field.variable_id_name}} = {{field.variable_id}};
    void set_{{field.name}}(const {{field.type}}& value) { {{field.variable_name}} = value; }
    {{field.type}} get_{{field.name}}() const { return {{field.variable_name}}; }
    {% endfor %}

    Result serialize(uint8_t* buffer, uint32_t length) const final
    {
    {% for field in single_fields %}
      if({{field.default_value}} != {{field.variable_name}})
      {
        //buffer = ::EmbeddedProto::WireFormatter::{{field.serialize}}({{field.number}}, {{field.variable_name}}, buffer);
      }
    {% endfor %}
        return Result::OK;
    };

    Result deserialize(const uint8_t* buffer, uint32_t length) final
    {
        return Result::OK;
    };

    void clear() final
    {

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

{%- if namespace %}
namespace {{ namespace }}
{ {% endif %}

{%- for enum in enums %}
{{ enum_macro(enum) }}
{%- endfor %}

{%- for msg in messages %}
{{ msg_macro(msg) }}
{%- endfor %}

{% if namespace %}
} // End of namespace {{ namespace }}
{% endif %}
