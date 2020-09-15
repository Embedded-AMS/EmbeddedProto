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

from .TypeDefinitions import *
import os


class ProtoFile:
    def __init__(self, proto_descriptor):
        self.descriptor = proto_descriptor

        if "proto2" == proto_descriptor.syntax:
            raise Exception(proto_descriptor.name + ": Sorry, proto2 is not supported, please use proto3.")

        # These file names are the ones used for creating the C++ files.
        self.filename_with_folder = os.path.splitext(proto_descriptor.name)[0]
        self.filename_without_folder = os.path.basename(self.filename_with_folder)

        # Construct the base scope used in this file.
        self.scope = None
        if self.descriptor.package:
            package_list = self.descriptor.package.split(".")
            # The first element is the base scope.
            self.scope = Scope(package_list[0], None)
            # Next add additional scopes nesting the previous one.
            for package in package_list[1:]:
                self.scope = Scope(package, self.scope)

        self.enum_definitions = [EnumDefinition(enum, self.scope) for enum in self.descriptor.enum_type]
        self.msg_definitions = [MessageDefinition(msg, self.scope) for msg in self.descriptor.message_type]

    def get_dependencies(self):
        imported_dependencies = []
        if self.descriptor.dependency:
            imported_dependencies = [os.path.splitext(dependency)[0] + ".h" for dependency in
                                     self.descriptor.dependency]
        return imported_dependencies

    def get_header_guard(self):
        return self.filename_with_folder.replace("/", "_").upper()

    # Obtain a dictionary with references to all nested enums and messages
    def get_all_nested_types(self):
        nested_types = {"enums": self.enum_definitions, "messages": []}
        for msg in self.msg_definitions:
            nt = msg.get_all_nested_types()
            nested_types["enums"].extend(nt["enums"])
            nested_types["messages"].extend(nt["messages"])
            nested_types["messages"].append(msg)

        return nested_types

    def match_fields_with_definitions(self, all_types_definitions):
        for msg in self.msg_definitions:
            msg.match_fields_with_definitions(all_types_definitions)
