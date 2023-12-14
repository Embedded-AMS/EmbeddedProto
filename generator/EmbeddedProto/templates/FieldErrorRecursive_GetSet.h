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
#warning "You have a recursive inclusion with the field: '{{field.get_name()}}'. Embedded Proto is unable to determine the template parameters in this case. We could only generate this message by leaving it out."
static constexpr char const* {{field.get_name()|upper}}_NAME = "{{field.get_name()}}";
inline void clear_{{field.get_name()}}() { }
inline void set_{{field.get_name()}}(const uint8_t& value) { }
inline void set_{{field.get_name()}}(const uint8_t&& value) { }
inline const uint8_t get_{{field.get_name()}}() const { return 0; }
inline uint8_t {{field.get_name()}}() const { return 0; }