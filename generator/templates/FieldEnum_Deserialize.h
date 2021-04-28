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
if(::EmbeddedProto::WireFormatter::WireType::{{field.get_wire_type_str()}} == wire_type)
{
  uint32_t value = 0;
  return_value = ::EmbeddedProto::WireFormatter::DeserializeVarint(buffer, value);
  if(::EmbeddedProto::Error::NO_ERRORS == return_value)
  {
    set_{{field.get_name()}}(static_cast<{{field.get_type()}}>(value));
  }
}
else
{
  // Wire type does not match field.
  return_value = ::EmbeddedProto::Error::INVALID_WIRETYPE;
}