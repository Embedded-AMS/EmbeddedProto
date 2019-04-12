import io
import sys
import itertools
import json

from google.protobuf.compiler import plugin_pb2 as plugin
from google.protobuf.descriptor_pb2 import DescriptorProto, EnumDescriptorProto


def traverse(proto_file):

    def _traverse(package, items):
        for item in items:
            yield item, package

            if isinstance(item, DescriptorProto):
                for enum in item.enum_type:
                    yield enum, package

                for nested in item.nested_type:
                    nested_package = package + item.name

                    for nested_item in _traverse(nested, nested_package):
                        yield nested_item, nested_package

    return itertools.chain(
        _traverse(proto_file.package, proto_file.enum_type),
        _traverse(proto_file.package, proto_file.message_type),
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
    generate_json(request, response)

    # Serialise response message
    output = response.SerializeToString()

    # Write to stdout
    io.open(sys.stdout.fileno(), "wb").write(response.SerializeToString())


def main_cli():
    with open("debug_request.bin", 'rb') as file:
        data = file.read()
        request = plugin.CodeGeneratorRequest.FromString(data)

        # Create response
        response = plugin.CodeGeneratorResponse()

        # Generate code
        generate_json(request, response)

        # Serialise response message
        output = response.SerializeToString()

        print(output)


if __name__ == '__main__':
    # Check if we are running as a plugin under protoc
    if '--protoc-plugin' in sys.argv:
        main_plugin()
    else:
        main_cli()


# protoc --plugin=protoc-gen-eams=protoc-gen-eams.sh --eams_out=./build ./test/proto/state.proto