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
{% if field.oneof is not none %}
inline void clear_{{field.get_name()}}()
{
  if(id::{{field.get_variable_id_name()}} == {{field.get_which_oneof()}})
  {
    {{field.get_which_oneof()}} = id::NOT_SET;
    {{field.get_variable_name()}}.set({{field.get_default_value()}});
  }
}
inline void set_{{field.get_name()}}(const {{field.get_type()}}::FIELD_TYPE& value)
{
  if(id::{{field.get_variable_id_name()}} != {{field.get_which_oneof()}})
  {
    init_{{field.get_oneof_name()}}(id::{{field.get_variable_id_name()}});
  }
  {{field.get_variable_name()}}.set(value);
}
inline void set_{{field.get_name()}}(const {{field.get_type()}}::FIELD_TYPE&& value)
{
  if(id::{{field.get_variable_id_name()}} != {{field.get_which_oneof()}})
  {
    init_{{field.get_oneof_name()}}(id::{{field.get_variable_id_name()}});
  }
  {{field.get_variable_name()}}.set(value);
}
inline void set_{{field.get_name()}}(const {{field.get_type()}}& value)
{
  if(id::{{field.get_variable_id_name()}} != {{field.get_which_oneof()}})
  {
    init_{{field.get_oneof_name()}}(id::{{field.get_variable_id_name()}});
  }
  {{field.get_variable_name()}} = value;
}
inline void set_{{field.get_name()}}(const {{field.get_type()}}&& value)
{
  if(id::{{field.get_variable_id_name()}} != {{field.get_which_oneof()}})
  {
    init_{{field.get_oneof_name()}}(id::{{field.get_variable_id_name()}});
  }
  {{field.get_variable_name()}} = value;
}
{% else %}
inline void clear_{{field.get_name()}}() { {{field.get_variable_name()}}.clear(); }
inline void set_{{field.get_name()}}(const {{field.get_type()}}& value) { {{field.get_variable_name()}} = value; }
inline void set_{{field.get_name()}}(const {{field.get_type()}}&& value) { {{field.get_variable_name()}} = value; }
inline {{field.get_type()}}& mutable_{{field.get_name()}}() { return {{field.get_variable_name()}}; }
{% endif %}
inline const {{field.get_type()}}& get_{{field.get_name()}}() const { return {{field.get_variable_name()}}; }
inline {{field.get_type()}}::FIELD_TYPE {{field.get_name()}}() const { return {{field.get_variable_name()}}.get(); }