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

from .TypeDefinitions import *
import os
from toposort import CircularDependencyError, toposort_flatten
from google.protobuf.descriptor_pb2 import FieldDescriptorProto


# -----------------------------------------------------------------------------

def toposort_add_msg(msg, namespace, dependency_data):

    local_definitions = []
    local_namespace = namespace + "." + msg.name

    dependencies = {namespace}

    for nested_msg in msg.nested_type:
        dependency_data = toposort_add_msg(nested_msg, local_namespace, dependency_data)
        full_msg_type = local_namespace + "." + nested_msg.name
        local_definitions.append(full_msg_type)
        for dep in dependency_data[full_msg_type]:
            # Add requirements on other namespaces.
            if not dep.startswith(local_namespace):
                dependencies.add(dep)

    for nested_enum in msg.enum_type:
        full_enum_type = local_namespace + "." + nested_enum.name
        dependency_data[full_enum_type] = {local_namespace}
        local_definitions.append(full_enum_type)

    for f in msg.field:
        if ((FieldDescriptorProto.TYPE_MESSAGE == f.type) or (FieldDescriptorProto.TYPE_ENUM == f.type)) \
                and (f.type_name not in local_definitions):
            dependencies.add(f.type_name)

    # If we have any dependencies add them
    if 0 < len(dependencies):
        dependency_data[local_namespace] = dependencies

    return dependency_data


# -----------------------------------------------------------------------------

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

        self.all_parameters_registered = False

        # Sort the message definitions such that the dependencies work out.
        dependency_data = {}
        toposort_namespace = ""
        if self.descriptor.package:
            toposort_namespace = "." + self.descriptor.package

        for msg in self.descriptor.message_type:
            dependency_data = toposort_add_msg(msg, toposort_namespace, dependency_data)

        try:
            # Sort the message in the order they should appear in the code.
            message_order = toposort_flatten(dependency_data)

            # If we are in a namespace this appears in the list and should be removed.
            my_scope = ""
            if self.scope and (0 < len(message_order)):
                my_scope = "." + self.scope.get_scope_str().replace("::", ".")
                message_order.remove(my_scope)

            # Based on the desired order assign each message definition an index.
            for index, msg_name in enumerate(message_order):
                for msg_def in self.msg_definitions:
                    if msg_def.name == msg_name.replace(my_scope + ".", ''):
                        msg_def.sorted_index = index
                        break

            # Next sort the messages based on their index.
            self.msg_definitions.sort(key=lambda msg: msg.sorted_index)

            # Sort al the nested message definitions.
            for msg in self.msg_definitions:
                msg.sort_nested_msg_definitions(message_order)

        except CircularDependencyError as e:
            raise Exception("There are possible circular dependencies in the message definitions of "
                            + proto_descriptor.name + ". Embedded Proto is not able to support this. "
                            "Please remove these dependencies.")

    def get_dependencies(self):
        imported_dependencies = []
        if self.descriptor.dependency:
            imported_dependencies = [os.path.splitext(dependency)[0] + ".h" for dependency in
                                     self.descriptor.dependency if "embedded_proto_options.proto" not in dependency]
        return imported_dependencies

    def get_namespaces(self):
        if self.scope:
            result = self.scope.get_list_of_scope_str()
        else:
            result = []
        return result

    def get_header_guard(self):
        return self.filename_with_folder.replace("/", "_").upper()

    # Obtain a dictionary with references to all nested enums and messages
    def get_all_nested_types(self):
        nested_types = {"enums": [], "messages": []}
        nested_types["enums"].extend(self.enum_definitions)
        for msg in self.msg_definitions:
            nt = msg.get_all_nested_types()
            nested_types["enums"].extend(nt["enums"])
            nested_types["messages"].extend(nt["messages"])
            nested_types["messages"].append(msg)

        return nested_types

    def match_fields_with_definitions(self, all_types_definitions):
        for msg in self.msg_definitions:
            msg.match_fields_with_definitions(all_types_definitions)

    def register_template_parameters(self):
        all_parameters_registered = True
        for msg in self.msg_definitions:
            all_parameters_registered = msg.register_template_parameters() and all_parameters_registered
        return all_parameters_registered

    def render(self, jinja_environment):
        template_file = "Header.h"
        template = jinja_environment.get_template(template_file)
        file_str = template.render(proto_file=self, environment=jinja_environment)
        return file_str

    def print_template_data(self, indent):
        print(indent + "File: " + self.filename_without_folder)
        if self.msg_definitions:
            for msg in self.msg_definitions:
                msg.print_template_data(indent + "\t")

