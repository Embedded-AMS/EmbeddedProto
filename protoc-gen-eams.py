import io
import sys
import itertools
import json
import os
from jinja2 import Template

from google.protobuf.compiler import plugin_pb2 as plugin
from google.protobuf.descriptor_pb2 import DescriptorProto, FieldDescriptorProto, EnumDescriptorProto

template_macro_enum = \
    """
{%- macro enum_macro(_enums) %}
{% for _enum in _enums %}
enum class {{ _enum.name }}
{
  {% for value in _enum.value -%}
  {{ value.name }} = {{ value.number }}, 
  {% endfor %}    
}
{% endfor -%}
{% endmacro -%}
"""

# template_macro_




def traverse(proto_file):
    def _traverse(package, items):
        for item in items:
            yield item, package

            if isinstance(item, DescriptorProto):
                for enum in item.enumfield.type:
                    yield enum, package

                for nested in item.nestedfield.type:
                    nested_package = package + item.name

                    for nested_item in _traverse(nested, nested_package):
                        yield nested_item, nested_package

    return itertools.chain(
        _traverse(proto_file.package, proto_file.enumfield.type),
        _traverse(proto_file.package, proto_file.messagefield.type),
    )


def generate_json(request, response):
    for proto_file in request.proto_file:
        json_output = []

        # Parse request
        for item, package in traverse(proto_file):
            data = {
                'package': proto_file.package or '&lt;root&gt;',
                'filename': proto_file.name,
                'name': item.name,
            }

            if isinstance(item, DescriptorProto):
                data.update({
                    'type': 'Message',
                    'properties': [{'name': f.name, 'type': int(f.type)}
                                   for f in item.field]
                })

            elif isinstance(item, EnumDescriptorProto):
                data.update({
                    'type': 'Enum',
                    'values': [{'name': v.name, 'value': v.number}
                               for v in item.value]
                })

            json_output.append(data)

        # Fill response
        f = response.file.add()
        f.name = proto_file.name + '.json'
        f.content = json.dumps(json_output, indent=2)


def generate_message_fields(fields):
    pass


def generate_message(message):
    # This function takes in a DescriptorProto object and generates the source code for this message class.

    # Define different template separate to keep easy overview.

    template_message_class = \
        """
class {{ msg.name }}: public ::EmbeddedProto::MessageInterface 
{
  public:
    {{ msg.name }}() : 
        {%- for field in single_fields %}
        {{field.name}}({{field.default_value}}),
        {%- endfor %}
    {
    
    };
    
    ~{{ msg.name }}() = default;
    
    {% for field in single_fields %}
    void set_{{field.name}}(const {{field.type}}& x) { _{{field.name}} = x; }
    {{field.type}} get_{{field.name}}() const { return _{{field.name}}; }
    {% endfor %}
    
  private:

    {% for field in single_fields -%}
    {{field.type}} _{{field.name}};
    {% endfor %}            
};
        """

    # Combine the the different templates into one.
    template_message = template_message_class

    class SingleField:
        def __init__(self, field):
            self.name = field.name
            self.type = self.type_to_str(field)
            self.default_value = self.type_to_default(field)

        @staticmethod
        def type_to_str(_field):
            if FieldDescriptorProto.TYPE_DOUBLE == _field.type:
                return "double"
            elif FieldDescriptorProto.TYPE_FLOAT == _field.type:
                return "float"
            elif FieldDescriptorProto.TYPE_INT64 == _field.type:
                return "int64_t"
            elif FieldDescriptorProto.TYPE_UINT64 == _field.type:
                return "uint64_t"
            elif FieldDescriptorProto.TYPE_INT32 == _field.type:
                return "int32_t"
            elif FieldDescriptorProto.TYPE_FIXED64 == _field.type:
                return "int64_t"
            elif FieldDescriptorProto.TYPE_FIXED32 == _field.type:
                return "int32_t"
            elif FieldDescriptorProto.TYPE_BOOL == _field.type:
                return "bool"
            elif FieldDescriptorProto.TYPE_STRING == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_GROUP == _field.type:
                # Deprecated
                pass
            elif FieldDescriptorProto.TYPE_MESSAGE == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_BYTES == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_UINT32 == _field.type:
                return "uint32_t"
            elif FieldDescriptorProto.TYPE_ENUM == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_SFIXED32 == _field.type:
                return "int32_t"
            elif FieldDescriptorProto.TYPE_SFIXED64 == _field.type:
                return "int64_t"
            elif FieldDescriptorProto.TYPE_SINT32 == _field.type:
                return "int32_t"
            elif FieldDescriptorProto.TYPE_SINT64 == _field.type:
                return "int64_t"
            else:
                pass

        @staticmethod
        def type_to_default(_field):
            if FieldDescriptorProto.TYPE_DOUBLE == _field.type:
                return "0.0"
            elif FieldDescriptorProto.TYPE_FLOAT == _field.type:
                return "0.0F"
            elif FieldDescriptorProto.TYPE_BOOL == _field.type:
                return "false"
            elif FieldDescriptorProto.TYPE_STRING == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_GROUP == _field.type:
                # Deprecated
                pass
            elif FieldDescriptorProto.TYPE_MESSAGE == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_BYTES == _field.type:
                # TODO
                pass
            elif FieldDescriptorProto.TYPE_ENUM == _field.type:
                # TODO
                pass
            else:
                # All other types are numbers with default zero.
                return "0"

    # Split out the simple fields from the more complicated ones.
    single_fields = []
    for field in message.field:
        if field.label == field.LABEL_REPEATED:
            # TODO do something with repeated fields.
            pass
        else:
            if field.type == field.TYPE_STRING:
                # TODO
                pass
            elif field.type == field.TYPE_GROUP:
                # Deprecated in protobuf
                pass
            elif field.type == field.TYPE_MESSAGE:
                # TODO
                pass
            elif field.type == field.TYPE_BYTES:
                # TODO
                pass
            elif field.type == field.TYPE_ENUM:
                # TODO
                pass
            else:
                # For all the simple types
                single_fields.append(SingleField(field))

    # Run the template render for this class
    template = Template(template_message)
    try:
        class_str = template.render(msg=message, single_fields=single_fields)
    except:
        pass
    else:
        return class_str


def proto_to_header_filename(proto_filename):
    return os.path.splitext(proto_filename)[0] + ".h"


def generate_code(request, respones):

    header_template = \
        """
#pragma once

#include <cstdint>
{% if messages %}#include <MessageInterface.h>{% endif %}

{%- if namespace %}
namespace {{ namespace }}
{ {% endif %}

{% for message in messages %}
{{message}} {% endfor %}

{% if namespace %}
} // End of namespace {{ namespace }}
{% endif %}
    """

    # Loop over all proto files in the request
    for proto_file in request.proto_file:

        message_list = []
        for msg in proto_file.message_type:
            message_list.append(generate_message(msg))

        template = Template(header_template)
        try:
            file_str = template.render(namespace=proto_file.package, messages=message_list)
        except:
            pass
        else:
            f = respones.file.add()
            f.name = proto_to_header_filename(proto_file.name)
            f.content = file_str


def main_plugin():
    # Read request message from stdin
    data = io.open(sys.stdin.fileno(), "rb").read()
    request = plugin.CodeGeneratorRequest.FromString(data)

    # Write the requests to a file for easy debugging.
    with open("debug_request.bin", 'wb') as file:
        file.write(request.SerializeToString())

    # Create response
    response = plugin.CodeGeneratorResponse()

    # Generate code
    generate_code(request, response)

    # Serialise response message
    output = response.SerializeToString()

    # Write to stdout
    io.open(sys.stdout.fileno(), "wb").write(output)


def main_cli():
    with open("debug_request.bin", 'rb') as file:
        data = file.read()
        request = plugin.CodeGeneratorRequest.FromString(data)

        # Create response
        response = plugin.CodeGeneratorResponse()

        # Generate code
        generate_code(request, response)

        for response_file in response.file:
            print(response_file.name)
            print(response_file.content)


if __name__ == '__main__':
    # Check if we are running as a plugin under protoc
    if '--protoc-plugin' in sys.argv:
        main_plugin()
    else:
        main_cli()

# protoc --plugin=protoc-gen-eams=protoc-gen-eams --eams_out=./build ./test/proto/state.proto
