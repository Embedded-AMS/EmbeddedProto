#
# Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
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
#   Atoomweg 2
#   1627 LE, Hoorn
#   the Netherlands
#

import io
import sys
from EmbeddedProto.ProtoFile import ProtoFile
from google.protobuf.compiler import plugin_pb2 as plugin
import jinja2
from importlib.resources import path as resource_path


# -----------------------------------------------------------------------------


def generate_code(request, respones):
    # Create definitions for al proto files in the request except for our own options file which is not required in cpp
    # code. First also ignore the google descriptor file, only add it later if it is required by the user.
    file_definitions = []
    google_descriptor_file = None
    add_google_descriptor_file = False
    for proto_file in request.proto_file:
        if ("embedded_proto_options.proto" not in proto_file.name) and \
           ("google/protobuf/descriptor.proto" not in proto_file.name):
            file_definitions.append(ProtoFile(proto_file))
            if "google/protobuf/descriptor.proto" in file_definitions[-1].descriptor.dependency:
                add_google_descriptor_file = True

        # If we come by the descriptor just store it so we can easily use it when needed.
        if "google/protobuf/descriptor.proto" in proto_file.name:
            google_descriptor_file = proto_file

    # See if we should include google/protobuf/descriptor.proto after all as it is directly include by one of user
    # defined proto files.
    if add_google_descriptor_file and google_descriptor_file:
        # Insert it at the front so the header file will include it before the classes requiring it.
        file_definitions.insert(0, ProtoFile(google_descriptor_file))

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

    # Add template parameters to the fields that need them.
    all_parameters_registered = True
    for _ in range(3):
        for fd in file_definitions:
            all_parameters_registered = fd.register_template_parameters() and all_parameters_registered
        if all_parameters_registered:
            break

    if not all_parameters_registered:
        raise Exception("Messages with repeated, string or byte fields use template parameters to define their length."
                        "For some reason it was not to add all required template parameters.")

    with resource_path("EmbeddedProto", "templates") as filepath:
        template_loader = jinja2.FileSystemLoader(searchpath=filepath)
        template_env = jinja2.Environment(loader=template_loader, trim_blocks=True, lstrip_blocks=True)

    for fd in file_definitions:
        file_str = fd.render(template_env)
        if file_str:
            f = respones.file.add()
            f.name = fd.filename_with_folder + ".h"
            f.content = file_str
        else:
            break


# -----------------------------------------------------------------------------

def main_plugin():
    # The main function when running the scrip as a protoc plugin. It will read in the protoc data from the stdin and
    # write back the output to stdout.

    # Create the response object
    response = plugin.CodeGeneratorResponse()
    response.supported_features = plugin.CodeGeneratorResponse.FEATURE_PROTO3_OPTIONAL

    # Read request message from stdin
    data = io.open(sys.stdin.fileno(), "rb").read()
    request = plugin.CodeGeneratorRequest.FromString(data)

    # If desired output debug data.
    if '--debug' in sys.argv:
        # Write the requests to a file for easy debugging.
        with open("./debug_embedded_proto.bin", 'wb') as file:
            file.write(request.SerializeToString())

        from google.protobuf.json_format import MessageToJson

        with open("./debug_embedded_proto.json", 'w') as file:
            file.write(MessageToJson(request))

    # Generate code
    try:
        generate_code(request, response)
    except jinja2.UndefinedError as e:
        response.error = "Embedded Proto ERROR - Template Undefined Error exception: " + str(e)
    except jinja2.TemplateRuntimeError as e:
        response.error = "Embedded Proto ERROR - Template Runtime Error exception: " + str(e)
    except jinja2.TemplateAssertionError as e:
        response.error = "Embedded Proto ERROR - TemplateAssertionError exception: " + str(e)
    except jinja2.TemplateSyntaxError as e:
        response.error = "Embedded Proto ERROR - TemplateSyntaxError exception: " + str(e)
    except jinja2.TemplateError as e:
        response.error = "Embedded Proto ERROR - TemplateError exception: " + str(e)
    except Exception as e:
        response.error = "Embedded Proto ERROR - " + str(e)

    # Serialize response message
    output = response.SerializeToString()

    # Write to stdout
    io.open(sys.stdout.fileno(), "wb").write(output)


# -----------------------------------------------------------------------------

def main_cli():
    # The main function when running from the command line and debugging.  Instead of receiving data from protoc this
    # will read in a binary file stored the previous time main_plugin() is ran.

    with open("debug_embedded_proto.bin", 'rb') as file:
        # Create the response object
        response = plugin.CodeGeneratorResponse()
        response.supported_features = plugin.CodeGeneratorResponse.FEATURE_PROTO3_OPTIONAL

        data = file.read()
        request = plugin.CodeGeneratorRequest.FromString(data)

        # Generate code
        try:
            generate_code(request, response)
        except jinja2.UndefinedError as e:
            response.error = "Embedded Proto ERROR - Template Undefined Error exception: " + str(e)
        except jinja2.TemplateRuntimeError as e:
            response.error = "Embedded Proto ERROR - Template Runtime Error exception: " + str(e)
        except jinja2.TemplateAssertionError as e:
            response.error = "Embedded Proto ERROR - TemplateAssertionError exception: " + str(e)
        except jinja2.TemplateSyntaxError as e:
            response.error = "Embedded Proto ERROR - TemplateSyntaxError exception: " + str(e)
        except jinja2.TemplateError as e:
            response.error = "Embedded Proto ERROR - TemplateError exception: " + str(e)
        except Exception as e:
            response.error = "Embedded Proto ERROR - " + str(e)

        # For debugging purposes print the result to the console.
        for response_file in response.file:
            print(response_file.name)
            print(response_file.content)

        if response.error:
            print(response.error)

# -----------------------------------------------------------------------------
def main():
    # Check if we are running as a plugin under protoc
    if '--protoc-plugin' in sys.argv:
        main_plugin()
    else:
        main_cli()


if __name__ == "__main__":
    main()