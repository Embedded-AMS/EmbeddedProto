{#
Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved

This file is part of Embedded Proto.

Embedded Proto is open source software: you can redistribute it and/or
modify it under the terms of the GNU General Public License as published
by the Free Software Foundation, version 3 of the license.

Embedded Proto  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.

For commercial and closed source application please visit:
<https://EmbeddedProto.com/license/>.

Embedded AMS B.V.
Info:
  info at EmbeddedProto dot com

Postal address:
  Atoomweg 2
  1627 LE, Hoorn
  the Netherlands
#}
{% macro assign(_oneof) %}
if(rhs.get_which_{{_oneof.get_name()}}() != {{_oneof.get_which_oneof()}})
{
  // First delete the old object in the oneof.
  clear_{{_oneof.get_name()}}();
}

switch(rhs.get_which_{{_oneof.get_name()}}())
{
  {% for field in _oneof.get_fields() %}
  case FieldNumber::{{field.get_variable_id_name()}}:
    set_{{field.get_name()}}(rhs.get_{{field.name}}());
    break;

  {% endfor %}
  default:
    break;
}
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro init(_oneof) %}
void init_{{_oneof.get_name()}}(const FieldNumber field_id)
{
  if(FieldNumber::NOT_SET != {{_oneof.get_which_oneof()}})
  {
    // First delete the old object in the oneof.
    clear_{{_oneof.get_name()}}();
  }

  {% if _oneof.oneof_allocation_required() %}
  // C++11 unions only support nontrivial members when you explicitly call the placement new statement.
  switch(field_id)
  {
    {% for field in _oneof.get_fields() %}
    {% if field.oneof_allocation_required() %}
    case FieldNumber::{{field.get_variable_id_name()}}:
      new(&{{field.get_variable_name()}}) {{field.get_type()}};
      break;

    {% endif %}
    {% endfor %}
    default:
      break;
   }

   {% endif %}
   {{_oneof.get_which_oneof()}} = field_id;
}
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro clear(_oneof) %}
void clear_{{_oneof.get_name()}}()
{
  switch({{_oneof.get_which_oneof()}})
  {
    {% for field in _oneof.get_fields() %}
    case FieldNumber::{{field.get_variable_id_name()}}:
      {% if field.oneof_allocation_required() %}
      ::EmbeddedProto::destroy_at(&{{field.get_variable_name()}});
	    {% elif field.of_type_enum %}
	    {{field.get_variable_name()}} = {{field.get_default_value()}};
      {% else %}
      {{field.get_variable_name()}}.set(0);
      {% endif %}
      break;
    {% endfor %}
    default:
      break;
  }
  {{_oneof.get_which_oneof()}} = FieldNumber::NOT_SET;
}
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro deserialize(_oneof, _environment) %}
::EmbeddedProto::Error deserialize_{{_oneof.get_name()}}(const FieldNumber field_id, 
                              ::EmbeddedProto::ReadBufferInterface& buffer,
                              const ::EmbeddedProto::WireFormatter::WireType wire_type)
{
  ::EmbeddedProto::Error return_value = ::EmbeddedProto::Error::NO_ERRORS;
  
  if(field_id != {{_oneof.get_which_oneof()}})
  {
    init_{{_oneof.get_name()}}(field_id);
  }

  switch({{_oneof.get_which_oneof()}})
  {
    {% for field in _oneof.get_fields() %}
    case FieldNumber::{{field.get_variable_id_name()}}:
      {{ field.render_deserialize(_environment)|indent(6) }}
      break;
    {% endfor %}
    default:
      break;
  }

  if(::EmbeddedProto::Error::NO_ERRORS != return_value)
  {
    clear_{{_oneof.get_name()}}();
  }
  return return_value;
}
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro to_string(_oneof) %}
::EmbeddedProto::string_view to_string_{{_oneof.get_name()}}(::EmbeddedProto::string_view& str, const uint32_t indent_level, const bool first_field) const
{
  ::EmbeddedProto::string_view left_chars = str;

  switch({{_oneof.get_which_oneof()}})
  {
    {% for field in _oneof.get_fields() %}
    case FieldNumber::{{field.get_variable_id_name()}}:
      left_chars = {{field.get_variable_name()}}.to_string(left_chars, indent_level, {{field.get_name()|upper}}_NAME, first_field);
      break;
    {% endfor %}
    default:
      break;
  }

  return left_chars;
}
{% endmacro %}