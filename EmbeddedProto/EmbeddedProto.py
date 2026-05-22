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


def run_protoc(argv=sys.argv):
    # Remove the program name
    argv.pop(0)

    # Check if the --cpp-src-location parameter is present
    if "--cpp-src-location" in argv:
        # Get the location of the src folder
        src_location = str((resources.files("EmbeddedProto") / "src").resolve())
        print(src_location)
        # Exit the script after printing the source location
        return

    # Check if the well known types should be included.
    argv = ["-I" + get_well_known_types_location() if x == "--IncludeWellKnownTypes" else x for x in argv]

    # Check if a plugin is included
    if not [x for x in argv if x.startswith("--plugin")]:
        # If not add the EmbeddedProto plugin
        argv.insert(0, "--plugin=protoc-gen-eams")

    protoc.main(argv)


####################################################################################

if __name__ == "__main__":
    run_protoc(sys.argv)
