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
{% if (field.optional or (field.oneof is not none)) %}
if(has_{{field.get_name()}}() && (::EmbeddedProto::Error::NO_ERRORS == return_value))
{% else %}
if(({{field.get_default_value()}} != {{field.get_variable_name()}}.get()) && (::EmbeddedProto::Error::NO_ERRORS == return_value))
{% endif %}
{
  return_value = {{field.get_variable_name()}}.serialize_with_id(static_cast<uint32_t>(FieldNumber::{{field.get_variable_id_name()}}), buffer, {{ "true" if (field.optional or (field.oneof is not none)) else "false" }});
}