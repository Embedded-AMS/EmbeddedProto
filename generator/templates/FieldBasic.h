FieldBasic.h{#
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
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro get_set(_field) %}
inline void clear_{{_field.name}}() { {{_field.variable_full_name}}.clear(); }
inline void set_{{_field.name}}(const {{_field.type}}& value) { {{_field.variable_full_name}} = value; }
inline void set_{{_field.name}}(const {{_field.type}}&& value) { {{_field.variable_full_name}} = value; }
inline {{_field.type}}& mutable_{{_field.name}}() { return {{_field.variable_full_name}}; }
inline const {{_field.type}}& get_{{_field.name}}() const { return {{_field.variable_full_name}}; }
inline {{_field.type}}::FIELD_TYPE {{_field.name}}() const { return {{_field.variable_full_name}}.get(); }
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro serialize(_field) %}
if(({{_field.default_value}} != {{_field.variable_full_name}}.get()) && (::EmbeddedProto::Error::NO_ERRORS == return_value))
{
  return_value = {{_field.variable_full_name}}.serialize_with_id(static_cast<uint32_t>(id::{{_field.variable_id_name}}), buffer);
}
{% endmacro %}
{# #}
{# ------------------------------------------------------------------------------------------------------------------ #}
{# #}
{% macro field_deserialize_macro(_field) %}
if(::EmbeddedProto::WireFormatter::WireType::{{_field.wire_type}} == wire_type)
{
  return_value = {{_field.variable_full_name}}.deserialize(buffer);
}
else
{
  // Wire type does not match field.
  return_value = ::EmbeddedProto::Error::INVALID_WIRETYPE;
}
{% endmacro %}
