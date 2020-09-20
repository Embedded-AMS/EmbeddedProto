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
{% for tmpl_param in typedef.get_templates() %}
{{"template<" if loop.first}}{{tmpl_param['type']}} {{tmpl_param['name']}}{{", " if not loop.last}}{{">" if loop.last}}
{% endfor %}
class {{ typedef.name }} final: public ::EmbeddedProto::MessageInterface
{
  public:
    {{ typedef.name }}(){% if (typedef.fields or typedef.oneofs) %} :
    {% endif %}
    {% for field in typedef.fields %}
        {{field.variable_name}}({{field.get_default_value()}}){{"," if not loop.last}}{{"," if loop.last and typedef.oneofs}}
    {% endfor %}
    {% for oneof in typedef.oneofs %}
        {{oneof.which_oneof}}(id::NOT_SET){{"," if not loop.last}}
    {% endfor %}
    {

    };
    ~{{ typedef.name }}() override = default;

    {% for enum in typedef.nested_enum_definitions %}
    {{ enum.render(environment)|indent(4) }}
    {% endfor %}

    {% for msg in typedef.nested_msg_definitions %}
    {{ msg.render(environment)|indent(4) }}
    {% endfor %}


    enum class id
    {
      NOT_SET = 0,
      {% for id_set in typedef.field_ids %}
      {{id_set[1]}} = {{id_set[0]}}{{ "," if not loop.last }}
      {% endfor %}
    };

    private:

      {% for field in typedef.fields %}
      {{field.get_type()}} {{field.variable_name}};
      {% endfor %}
};
