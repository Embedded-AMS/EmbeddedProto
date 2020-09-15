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

    # Each scope should register with it's parent scope.
    def add_child_scope(self, child_scope):
        self.child_scopes.append(child_scope)


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
