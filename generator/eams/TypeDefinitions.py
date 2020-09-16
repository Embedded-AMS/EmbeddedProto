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

from .Field import Field
from .Oneof import Oneof


# This class deal with the scope in which definitions and field are located. It is used to keep track of template
# parameters required for a given scope.
class Scope:
    def __init__(self, scope_name, parent):
        # The name of this scope/namespace
        self.name = scope_name

        # A reference to the scope above this scope. If we are at the top level this will be None.
        self.parent = parent

        # Start of without any children.
        self.child_scopes = []

        # Register this object with it's parent.
        if self.parent:
            self.parent.add_child_scope(self)

        self.fields_with_templates = []

    def get_scope_str(self):
        if self.parent:
            scope_str = self.parent.get_scope_str() + "::" + self.name
        else:
            scope_str = self.name

        return scope_str

    def register_template_parameters(self, field):
        self.fields_with_templates.append(field)

# -----------------------------------------------------------------------------


class TypeDefinition:
    def __init__(self, proto_descriptor, parent_scope):
        self.descriptor = proto_descriptor
        self.name = proto_descriptor.name
        self.scope = Scope(self.name, parent_scope)


# -----------------------------------------------------------------------------

class EnumDefinition(TypeDefinition):
    def __init__(self, proto_descriptor, parent_scope):
        super().__init__(proto_descriptor, parent_scope)

    # Loop through the values defined in the enum.
    def values(self):
        for value in self.descriptor.value:
            yield value


# -----------------------------------------------------------------------------

class MessageDefinition(TypeDefinition):
    def __init__(self, proto_descriptor, parent_scope):
        super().__init__(proto_descriptor, parent_scope)

        self.nested_enum_definitions = [EnumDefinition(enum, self.scope) for enum in self.descriptor.enum_type]
        self.nested_msg_definitions = [MessageDefinition(msg, self.scope) for msg in self.descriptor.nested_type]

        # Store the id numbers of all the fields to create the ID enum.
        self.field_ids = []

        # Store all the variable fields in this message.
        self.fields_array = []
        for f in self.descriptor.field:
            if not f.HasField('oneof_index'):
                new_field = Field.factory(f, self)
                self.fields_array.append(new_field)
                self.field_ids.append((new_field.variable_id, new_field.variable_id_name))

        # Store all the oneof definitions in this message.
        self.oneof_fields = []
        for index, oneof in enumerate(self.descriptor.oneof_decl):
            new_oneof = Oneof(oneof, index, proto_descriptor, self)
            self.oneof_fields.append(new_oneof)
            for oneof_field in oneof.fields():
                self.field_ids.append((oneof_field.variable_id, oneof_field.variable_id_name))

        # Sort the field id's such they will appear in order in the id enum.
        self.field_ids.sort()

        # Not all template parameters for this message definition have been registered with the scope. This is
        # most likely due to a message defined later in the proto file.
        self.all_parameters_registered = False

        # Does this message definition contains fields with template parameters.
        self.contains_template_parameters = False

    # Obtain a dictionary with references to all nested enums and messages
    def get_all_nested_types(self):
        nested_types = {"enums": self.nested_enum_definitions, "messages": []}
        for msg in self.nested_msg_definitions:
            nt = msg.get_all_nested_types()
            nested_types["enums"].extend(nt["enums"])
            nested_types["messages"].extend(nt["messages"])
            nested_types["messages"].append(msg)

        return nested_types

    def match_fields_with_definitions(self, all_types_definitions):
        # Resolve the types of the nested messages.
        for msg in self.nested_msg_definitions:
            msg.match_fields_with_definitions(all_types_definitions)

        # Resolve the types of the fields defined in this message.
        for field in self.fields_array:
            field.match_field_with_definitions(all_types_definitions)

        for oneof in self.oneof_fields:
            oneof.match_field_with_definitions(all_types_definitions)

    # TODO This function will fail to return True if this definitions contains it self as a nested field.
    def register_template_parameters(self):
        self.all_parameters_registered = True
        # First resolve the template parameters for all nested message definitions.
        for nested_msg in self.nested_enum_definitions:
            self.all_parameters_registered = nested_msg.register_template_parameters() \
                                             and self.all_parameters_registered

        # Next see if we our self have any fields that have template parameters.
        for field in self.fields_array:
            self.all_parameters_registered = field.register_template_parameters() and self.all_parameters_registered

        for oneof in self.oneof_fields:
            self.all_parameters_registered = oneof.register_template_parameters() and self.all_parameters_registered

        return self.all_parameters_registered

    def register_child_with_template(self, child):
        self.scope.register_template_parameters(child)
        self.contains_template_parameters = True
