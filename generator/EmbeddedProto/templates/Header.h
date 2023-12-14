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
/*
 *  This file is generated with Embedded Proto, PLEASE DO NOT EDIT!
 *  source: {{proto_file.descriptor.name}}
 */

// This file is generated. Please do not edit!
#ifndef {{proto_file.get_header_guard()}}_H
#define {{proto_file.get_header_guard()}}_H

#include <cstdint>
{% if proto_file.msg_definitions %}
#include <MessageInterface.h>
#include <WireFormatter.h>
#include <Fields.h>
#include <MessageSizeCalculator.h>
#include <ReadBufferSection.h>
#include <RepeatedFieldFixedSize.h>
#include <FieldStringBytes.h>
#include <Errors.h>
#include <Defines.h>
#include <limits>

{% endif %}
{% if proto_file.get_dependencies() is defined %}
// Include external proto definitions
{% for dependency in proto_file.get_dependencies() %}
#include "{{dependency}}"
{% endfor %}

{% endif %}
{% for namespace in proto_file.get_namespaces() %}
namespace {{ namespace }} {
{% endfor %}

{% for enum in proto_file.enum_definitions %}
{{ enum.render(environment) }}

{% endfor %}
{% for msg in proto_file.msg_definitions %}
{{ msg.render(environment) }}

{% endfor %}
{% for namespace in proto_file.get_namespaces()|reverse %}
} // End of namespace {{ namespace }}
{% endfor %}
#endif // {{proto_file.get_header_guard()}}_H
