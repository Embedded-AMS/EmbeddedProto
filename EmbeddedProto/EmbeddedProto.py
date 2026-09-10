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

import sys
import grpc_tools.protoc as protoc
from importlib import resources


def get_well_known_types_location():
    # Obtain the location of the Protobuf well known types, they are a resource
    # to the grpc-tools package the file system.
    file_name = (
        resources.files("grpc_tools") / "_proto"
    ).resolve()
    return str(file_name)


def _run_license_command(argv):
    # Configure / verify the license token without running a build. Imported
    # lazily so the normal protoc path does not pay for it.
    import argparse
    from EmbeddedProto import custom_header

    parser = argparse.ArgumentParser(
        prog="embeddedproto",
        description="Configure the EmbeddedProto license token and server URL.")
    parser.add_argument("--set-token", metavar="TOKEN", default=None,
                        help="Store this build token in the user config.")
    parser.add_argument("--server-url", metavar="URL", default=None,
                        help="Store this license server URL (must be https://).")
    parser.add_argument("--check-license", action="store_true",
                        help="Verify the configured token against the server; write nothing.")
    args = parser.parse_args(argv)
    return custom_header.configure(token=args.set_token, server_url=args.server_url,
                                   check_only=args.check_license)


def get_options_proto_location():
    # Obtain the folder holding embedded_proto_options.proto, it ships with this package.
    return str(resources.files("EmbeddedProto").resolve())


def build_protoc_argv(argv):
    # Turn the command line of embeddedproto into the one handed to protoc.
    #
    # The include paths for embedded_proto_options.proto and the Protobuf well known types
    # (google/protobuf/descriptor.proto, imported by the options file) are always appended,
    # so a user only lists the folders holding their own proto files. The flag
    # --IncludeWellKnownTypes from earlier versions is accepted and ignored.
    argv = [x for x in argv if x != "--IncludeWellKnownTypes"]
    argv = argv + ["-I" + get_options_proto_location(), "-I" + get_well_known_types_location()]

    # Check if a plugin is included
    if not [x for x in argv if x.startswith("--plugin")]:
        # If not add the EmbeddedProto plugin
        argv.insert(0, "--plugin=protoc-gen-eams")

    return argv


def run_protoc(argv=sys.argv):
    # Remove the program name
    argv.pop(0)

    # License configuration sub-commands, handled before protoc. These let
    # `embeddedproto --set-token <KEY> [--server-url <URL>]` and
    # `embeddedproto --check-license` manage the user config without a build.
    _license_flags = ("--set-token", "--server-url", "--check-license")
    if any(arg == flag or arg.startswith(flag + "=")
           for arg in argv for flag in _license_flags):
        sys.exit(_run_license_command(argv))

    # Check if the --cpp-src-location parameter is present
    if "--cpp-src-location" in argv:
        # Get the location of the src folder
        src_location = str((resources.files("EmbeddedProto") / "src").resolve())
        print(src_location)
        # Exit the script after printing the source location
        return

    protoc.main(build_protoc_argv(argv))
if __name__ == "__main__":
    run_protoc(sys.argv)
