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
      {{field.variable_name}}.serialize(buffer);
      {% else %}
      if(({{field.default_value}} != {{field.variable_name}}) && buffer.good())
      {
        serialize_tag({{field.variable_id_name}}, buffer);
        serialize_field({{field.variable_name}}, buffer);
      }
      {% endif %}

      {% endfor %}
      return buffer.good();
    };

    Result deserialize(const uint8_t* buffer, uint32_t length) final
    {
      return Result::OK;
    };

    void clear() final
    {
      {% for field in msg.fields() %}
      clear_{{field.name}}();
      {% endfor %}
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
