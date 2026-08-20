#
# Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
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
# <https://embeddedproto.com/pricing/>.
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
import locale
import json
from datetime import datetime
from EmbeddedProto.ProtoFile import ProtoFile, is_excluded_proto_file
from EmbeddedProto import custom_header
from EmbeddedProto import embedded_proto_options_pb2
from google.protobuf import descriptor_pb2
from google.protobuf.compiler import plugin_pb2 as plugin
import jinja2
from importlib.resources import path as resource_path

def load_version_info():
    """Load version information from version.json"""
    with resource_path("EmbeddedProto", "version.json") as filepath:
        with open(filepath) as f:
            version_data = json.load(f)
            version_parts = version_data["version"].split(".")
            return {
                "major": int(version_parts[0]),
                "minor": int(version_parts[1]),
                "patch": int(version_parts[2])
            }


# -----------------------------------------------------------------------------


# Describe an exception in the error reported back to protoc.
def describe_exception(e):
    # Errors this generator raises on purpose are a plain Exception, their text is the whole story. Any other type
    # signals an unexpected failure, there the type name is required as str() may hold nothing but a variable name.
    if type(e) is Exception:
        return str(e)
    return type(e).__name__ + ": " + str(e)


# The name of the message in embedded_proto_options.proto holding the generator options.
OPTIONS_MESSAGE_NAME = "Options"


# Raise a clear error when the compiled options module lags behind embedded_proto_options.proto.
def verify_options_module_is_current(request):
    # protoc parses embedded_proto_options.proto on every run, this generator reads the options through
    # embedded_proto_options_pb2.py. That module is generated once, while installing Embedded Proto. When an option
    # is added to the proto file without reinstalling, the module lags behind and reading the new option fails with
    # an AttributeError naming nothing but the option. Compare both and report what is actually wrong.
    compiled_options = {field.name for field in embedded_proto_options_pb2.Options.DESCRIPTOR.fields}
    for proto_file in request.proto_file:
        if "embedded_proto_options.proto" not in proto_file.name:
            continue
        for message in proto_file.message_type:
            if OPTIONS_MESSAGE_NAME != message.name:
                continue
            missing = [field.name for field in message.field if field.name not in compiled_options]
            if missing:
                raise Exception("The generated file embedded_proto_options_pb2.py is out of date, it lacks the "
                                "option(s): " + ", ".join(missing) + ". It is regenerated when installing Embedded "
                                "Proto, please rerun: python3 install.py")


def generate_code(request, respones):
    verify_options_module_is_current(request)

    # Create definitions for al proto files in the request except for the files we never generate code for. Our own
    # options file only holds generator settings. The google descriptor file only holds the definitions required to
    # declare custom options, they are used by protoc and other plugins but not by the embedded target.
    file_definitions = [ProtoFile(proto_file) for proto_file in request.proto_file
                        if not is_excluded_proto_file(proto_file.name)]

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

    # Add template parameters to the fields that need them. A message can only be resolved after the messages it uses
    # are resolved. As the files are not sorted by their mutual dependencies this may take several passes.
    unresolved_files = []
    for _ in range(3):
        unresolved_files = [fd for fd in file_definitions if not fd.register_template_parameters()]
        if not unresolved_files:
            break

    if unresolved_files:
        raise Exception("Messages with repeated, string or byte fields use template parameters to define their length. "
                        "For some reason it was not possible to add all required template parameters in the following "
                        "file(s): " + ", ".join(fd.descriptor.name for fd in unresolved_files) + ".")

    # Load version information
    version_info = load_version_info()

    # Resolve the optional custom header once per generation run (best-effort,
    # never blocks the build).
    plugin_version = "{major}.{minor}.{patch}".format(**version_info)
    custom_header_text = custom_header.resolve_custom_header(
        custom_header.resolve_token(), plugin_version)

    with resource_path("EmbeddedProto", "templates") as filepath:
        template_loader = jinja2.FileSystemLoader(searchpath=filepath)
        template_env = jinja2.Environment(loader=template_loader, trim_blocks=True, lstrip_blocks=True)
        # Add date, time and version information:
        template_env.globals['current_date_and_time'] = get_current_date_and_time()
        template_env.globals['version_major'] = version_info['major']
        template_env.globals['version_minor'] = version_info['minor']
        template_env.globals['version_patch'] = version_info['patch']
        template_env.globals['custom_header'] = custom_header_text

    for fd in file_definitions:
        file_str = fd.render(template_env)
        if file_str:
            f = respones.file.add()
            f.name = fd.filename_with_folder + ".h"
            f.content = file_str
        else:
            break


def get_current_date_and_time():
    locale.setlocale(locale.LC_TIME, '')
    return datetime.now().strftime('%c')


# -----------------------------------------------------------------------------

def configure_response_features(response):
    response.supported_features = (
        plugin.CodeGeneratorResponse.FEATURE_PROTO3_OPTIONAL |
        plugin.CodeGeneratorResponse.FEATURE_SUPPORTS_EDITIONS
    )

    if hasattr(descriptor_pb2, "Edition"):
        response.minimum_edition = descriptor_pb2.Edition.Value("EDITION_2023")
        response.maximum_edition = descriptor_pb2.Edition.Value("EDITION_2024")


def main_plugin():
    # The main function when running the scrip as a protoc plugin. It will read in the protoc data from the stdin and
    # write back the output to stdout.

    # Create the response object
    response = plugin.CodeGeneratorResponse()
    configure_response_features(response)

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
        response.error = "Embedded Proto ERROR - " + describe_exception(e)

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
        configure_response_features(response)

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
            response.error = "Embedded Proto ERROR - " + describe_exception(e)

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

# -----------------------------------------------------------------------------
if __name__ == "__main__":
    main()
