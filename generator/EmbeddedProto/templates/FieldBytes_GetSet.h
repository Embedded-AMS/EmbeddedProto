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
static constexpr char const* {{field.get_name()|upper}}_NAME = "{{field.get_name()}}";
{% if field.oneof is not none %}
inline bool has_{{field.get_name()}}() const
{
  return FieldNumber::{{field.get_variable_id_name()}} == {{field.get_which_oneof()}};
}
inline void clear_{{field.get_name()}}()
{
  if(FieldNumber::{{field.get_variable_id_name()}} == {{field.get_which_oneof()}})
  {
    {{field.get_which_oneof()}} = FieldNumber::NOT_SET;
    {{field.get_variable_name()}}.~{{field.get_short_type()}}();
  }
}
inline {{field.get_type()}}& mutable_{{field.get_name()}}()
{
  if(FieldNumber::{{field.get_variable_id_name()}} != {{field.get_which_oneof()}})
  {
    init_{{field.get_oneof_name()}}(FieldNumber::{{field.get_variable_id_name()}});
  }
  return {{field.get_variable_name()}};
}
inline void set_{{field.get_name()}}(const {{field.get_type()}}& rhs)
{
  if(FieldNumber::{{field.get_variable_id_name()}} != {{field.get_which_oneof()}})
  {
    init_{{field.get_oneof_name()}}(FieldNumber::{{field.get_variable_id_name()}});
  }
  {{field.get_variable_name()}}.set(rhs);
}
{% elif field.optional %}
inline bool has_{{field.get_name()}}() const
{
  return 0 != (presence::mask(presence::fields::{{field.get_name().upper()}}) & presence_[presence::index(presence::fields::{{field.get_name().upper()}})]);
}
inline void clear_{{field.get_name()}}()
{
  presence_[presence::index(presence::fields::{{field.get_name().upper()}})] &= ~(presence::mask(presence::fields::{{field.get_name().upper()}}));
  {{field.get_variable_name()}}.clear();
}
inline {{field.get_type()}}& mutable_{{field.get_name()}}()
{
  presence_[presence::index(presence::fields::{{field.get_name().upper()}})] |= presence::mask(presence::fields::{{field.get_name().upper()}});
  return {{field.get_variable_name()}};
}
inline void set_{{field.get_name()}}(const {{field.get_type()}}& rhs)
{
  presence_[presence::index(presence::fields::{{field.get_name().upper()}})] |= presence::mask(presence::fields::{{field.get_name().upper()}});
  {{field.get_variable_name()}}.set(rhs);
}
{% else %}
inline void clear_{{field.get_name()}}() { {{field.get_variable_name()}}.clear(); }
inline {{field.get_type()}}& mutable_{{field.get_name()}}() { return {{field.get_variable_name()}}; }
inline void set_{{field.get_name()}}(const {{field.get_type()}}& rhs) { {{field.get_variable_name()}}.set(rhs); }
{% endif %}
inline const {{field.get_type()}}& get_{{field.get_name()}}() const { return {{field.get_variable_name()}}; }
inline const uint8_t* {{field.get_name()}}() const { return {{field.get_variable_name()}}.get_const(); }