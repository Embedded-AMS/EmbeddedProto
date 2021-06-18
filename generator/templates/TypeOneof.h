{#
Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved

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
  Johan Huizingalaan 763a
  1066 VH, Amsterdam
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
  case id::{{field.get_variable_id_name()}}:
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
void init_{{_oneof.get_name()}}(const id field_id)
{
  if(id::NOT_SET != {{_oneof.get_which_oneof()}})
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
    case id::{{field.get_variable_id_name()}}:
      new(&{{field.get_variable_name()}}) {{field.get_type()}};
      {{_oneof.get_which_oneof()}} = id::{{field.get_variable_id_name()}};
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
    case id::{{field.get_variable_id_name()}}:
      {% if field.oneof_allocation_required() %}
      {{field.get_variable_name()}}.~{{field.get_short_type()}}(); // NOSONAR Unions require this.
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
  {{_oneof.get_which_oneof()}} = id::NOT_SET;
}
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro deserialize(_oneof) %}
::EmbeddedProto::Error deserialize_{{_oneof.get_name()}}(const id field_id, ::EmbeddedProto::Field& field,
                              ::EmbeddedProto::ReadBufferInterface& buffer,
                              const ::EmbeddedProto::WireFormatter::WireType wire_type)
{
  if(field_id != {{_oneof.get_which_oneof()}})
  {
    init_{{_oneof.get_name()}}(field_id);
  }
  ::EmbeddedProto::Error return_value = field.deserialize_check_type(buffer, wire_type);
  if(::EmbeddedProto::Error::NO_ERRORS != return_value)
  {
    clear_{{_oneof.get_name()}}();
  }
  return return_value;
}
{% endmacro %}