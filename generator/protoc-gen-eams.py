#
# Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
#
# This file is part of Embedded Proto.
#
# Embedded Proto is open source software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3 of the license.
#
# Embedded Proto  is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
#
# For commercial and closed source application please visit:
# <https://EmbeddedProto.com/license/>.
#
# Embedded AMS B.V.
# Info:
#   info at EmbeddedProto dot com
#
# Postal address:
#   Johan Huizingalaan 763a
#   1066 VH, Amsterdam
#   the Netherlands
#

import io
import sys
from generator.eams.ProtoFile import ProtoFile
from google.protobuf.compiler import plugin_pb2 as plugin


# -----------------------------------------------------------------------------


def generate_code(request, respones):
    # Create definitions for al proto files in the request.
    file_definitions = [ProtoFile(proto_file) for proto_file in request.proto_file]

    # Obtain all definitions made in all the files to properly link definitions with fields using them. This to properly
    # create template parameters.
    all_types_definitions = {"enums": [], "messages": []}
    for fd in file_definitions:
        nt = fd.get_all_nested_types()
        all_types_definitions["enums"].extend(nt["enums"])
        all_types_definitions["messages"].extend(nt["messages"])

    # Match all fields with their respective type definition.
    for fd in file_definitions:
        fd.match_fields_with_definitions(all_types_definitions)


# -----------------------------------------------------------------------------

def main_plugin():
    # The main function when running the scrip as a protoc plugin. It will read in the protoc data from the stdin and
    # write back the output to stdout.

    # Read request message from stdin
    data = io.open(sys.stdin.fileno(), "rb").read()
    request = plugin.CodeGeneratorRequest.FromString(data)

    if '--debug' in sys.argv:
        # Write the requests to a file for easy debugging.
        with open("./debug_embbeded_proto.bin", 'wb') as file:
            file.write(request.SerializeToString())

    # Create response
    response = plugin.CodeGeneratorResponse()

    # Generate code
    generate_code(request, response)

    # Serialize response message
    output = response.SerializeToString()

    # Write to stdout
    io.open(sys.stdout.fileno(), "wb").write(output)

# -----------------------------------------------------------------------------

def main_cli():
    # The main function when running from the command line and debugging.  Instead of receiving data from protoc this
    # will read in a binary file stored the previous time main_plugin() is ran.

    with open("debug_embbeded_proto.bin", 'rb') as file:
        data = file.read()
        request = plugin.CodeGeneratorRequest.FromString(data)

        # Create response
        response = plugin.CodeGeneratorResponse()

        # Generate code
        generate_code(request, response)

        # For debugging purposes print the result to the console.
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
