import io
import sys
import os
import jinja2

from google.protobuf.compiler import plugin_pb2 as plugin

# -----------------------------------------------------------------------------


class EnumTemplateParameters:
    def __init__(self, enum_proto):
        self.name = enum_proto.name
        self.enum_proto = enum_proto

    def values(self):
        for value in self.enum_proto.value:
            yield value


def generate_enums(enums):
    for enum in enums:
        yield EnumTemplateParameters(enum)

# -----------------------------------------------------------------------------


class FieldTemplateParameters:
    def __init__(self, field_proto):
        self.name = field_proto.name
        self.variable_name = self.name + "_"
        self.variable_id_name = self.name + "_id"
        self.default_value = 0
        self.type = "some_type"
        self.field_proto = field_proto

# -----------------------------------------------------------------------------


class MessageTemplateParameters:
    def __init__(self, msg_proto):
        self.name = msg_proto.name
        self.msg_proto = msg_proto

    def fields(self):
        for f in self.msg_proto.field:
            yield FieldTemplateParameters(f)

    def nested_enums(self):
        for enum in self.msg_proto.enum_type:
            yield EnumTemplateParameters(enum)


def generate_messages(message_types):
    for msg in message_types:
        yield MessageTemplateParameters(msg)

# -----------------------------------------------------------------------------


def proto_to_header_filename(proto_filename):
    return os.path.splitext(proto_filename)[0] + ".h"


def generate_code(request, respones):
    # Based upon the request from protoc generate

    template_loader = jinja2.FileSystemLoader(searchpath="./generator/")
    template_env = jinja2.Environment(loader=template_loader)
    template_file = "Header_Template.h"
    template = template_env.get_template(template_file)

    # Loop over all proto files in the request
    for proto_file in request.proto_file:

        if "proto2" == proto_file.syntax:
            raise Exception(proto_file.name + ": Sorry, proto2 is not supported, please use proto3.")

        messages_generator = generate_messages(proto_file.message_type)
        enums_generator = generate_enums(proto_file.enum_type)

        try:
            file_str = template.render(namespace=proto_file.package, messages=messages_generator, enums=enums_generator,
                                       trim_blocks=True, lstrip_blocks=True)
        except Exception as e:
            print("Template renderer exception: " + str(e))
        else:
            f = respones.file.add()
            f.name = proto_to_header_filename(proto_file.name)
            f.content = file_str


def main_plugin():
    # The main function when running the scrip as a protoc plugin. It will read in the protoc data from the stdin and
    # write back the output to stdout.

    # Read request message from stdin
    data = io.open(sys.stdin.fileno(), "rb").read()
    request = plugin.CodeGeneratorRequest.FromString(data)

    # Write the requests to a file for easy debugging.
    with open("../debug_request.bin", 'wb') as file:
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
    # The main function when running from the command line and debugging.  Instead of receiving data from protoc this
    # will read in a binary file stored the previous time main_plugin() is ran.

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

# -----------------------------------------------------------------------------


if __name__ == '__main__':
    # Check if we are running as a plugin under protoc
    if '--protoc-plugin' in sys.argv:
        main_plugin()
    else:
        main_cli()

# protoc --plugin=protoc-gen-eams=protoc-gen-eams --eams_out=./build ./test/proto/state.proto
