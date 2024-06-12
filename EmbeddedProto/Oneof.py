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

from .Field import Field


class Oneof:
    def __init__(self, oneof_proto_descriptor, index, msg_descriptor, parent_msg):
        # A reference to the OneofDescriptorProto object which defines this field.
        self.descriptor = oneof_proto_descriptor

        # A reference to the parent message in which this oneof is defined.
        self.parent = parent_msg

        self.fields = []
        # Loop over all the fields in this oneof
        for f in msg_descriptor.field:
            if f.HasField('oneof_index') and index == f.oneof_index:
                new_field = Field.factory(f, self.parent, oneof=self)
                self.fields.append(new_field)

    def get_name(self):
        return self.descriptor.name

    def get_variable_name(self):
        return self.get_name() + "_"

    def get_which_oneof(self):
        return "which_" + self.get_name() + "_"

    def get_fields(self):
        return self.fields

    def match_field_with_definitions(self, all_types_definitions):
        for field in self.fields:
            field.match_field_with_definitions(all_types_definitions)

    def register_template_parameters(self):
        all_parameters_registered = True
        for field in self.fields:
            all_parameters_registered = field.register_template_parameters() and all_parameters_registered
        return all_parameters_registered

    # Returns true if in oneof.init the new& function needs to be call to initialize already allocated memory.
    def oneof_allocation_required(self):
        result = False
        for field in self.fields:
            result = field.oneof_allocation_required()
            if result:
                break
        return result