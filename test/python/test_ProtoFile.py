#
# Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
#
# This file is part of Embedded Proto.
#
# Embedded Proto is open source software: you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published
# by the Free Software Foundation, version 3 of the License.
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

"""Unit tests for EmbeddedProto/ProtoFile.py - specifically the toposort_add_msg function."""

import os
import sys
import unittest

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from google.protobuf import descriptor_pb2
from google.protobuf.descriptor_pb2 import DescriptorProto, FieldDescriptorProto, FileDescriptorProto
from toposort import CircularDependencyError, toposort_flatten
from EmbeddedProto.ProtoFile import toposort_add_msg


def add_message_field(msg, name, type_name):
    field = msg.field.add()
    field.name = name
    field.type = FieldDescriptorProto.TYPE_MESSAGE
    field.type_name = type_name
    return field


class TestToposortAddMsg(unittest.TestCase):
    """Tests for the toposort_add_msg function to ensure it handles circular dependencies."""

    def test_includes_types_of_other_files(self):
        """Types defined in another file are a real dependency and must be kept."""
        msg = DescriptorProto()
        msg.name = "TestMessage"
        add_message_field(msg, "duration_field", ".google.protobuf.Duration")

        result = toposort_add_msg(msg, ".test", {})

        self.assertEqual(result.get(".test.TestMessage"), {".test", ".google.protobuf.Duration"})

    def test_includes_different_namespace_types(self):
        """Test that types from different namespaces are included as dependencies."""
        msg = DescriptorProto()
        msg.name = "TestMessage"
        add_message_field(msg, "other_field", ".other.SomeType")

        result = toposort_add_msg(msg, ".test", {})

        self.assertEqual(result.get(".test.TestMessage"), {".test", ".other.SomeType"})

    def test_excludes_own_nested_types(self):
        """A field using a type nested in this message is not a dependency of this message.

        The nested type depends on the message it is defined in. Listing it as a dependency of that message as well
        results in a circular dependency which is not there.
        """
        msg = DescriptorProto()
        msg.name = "TestMessage"
        add_message_field(msg, "nested_field", ".test.TestMessage.NestedType")

        result = toposort_add_msg(msg, ".test", {})

        self.assertEqual(result.get(".test.TestMessage"), {".test"})

    def test_excludes_deeply_nested_types(self):
        """The same but for a type nested more than one level deep, it is not in the local definitions."""
        msg = DescriptorProto()
        msg.name = "A"
        nested_b = msg.nested_type.add()
        nested_b.name = "B"
        nested_c = nested_b.nested_type.add()
        nested_c.name = "C"
        add_message_field(msg, "c_field", ".test.A.B.C")

        result = toposort_add_msg(msg, ".test", {})

        self.assertEqual(result.get(".test.A"), {".test"})

    def test_deeply_nested_types_can_be_sorted(self):
        """The regression of PROTO-298 expressed without google/protobuf/descriptor.proto."""
        msg = DescriptorProto()
        msg.name = "A"
        nested_b = msg.nested_type.add()
        nested_b.name = "B"
        nested_c = nested_b.nested_type.add()
        nested_c.name = "C"
        add_message_field(msg, "c_field", ".test.A.B.C")

        try:
            order = toposort_flatten(toposort_add_msg(msg, ".test", {}))
        except CircularDependencyError as e:
            self.fail("Unexpected circular dependency: " + str(e))

        # A nested message is defined inside the message holding it, so the outer message comes first.
        self.assertLess(order.index(".test.A"), order.index(".test.A.B"))
        self.assertLess(order.index(".test.A.B"), order.index(".test.A.B.C"))

    def test_handles_local_definitions(self):
        """Test that locally defined types in the same message are not added as dependencies."""
        msg = DescriptorProto()
        msg.name = "TestMessage"
        nested_msg = msg.nested_type.add()
        nested_msg.name = "NestedType"
        add_message_field(msg, "nested_field", ".test.TestMessage.NestedType")

        result = toposort_add_msg(msg, ".test", {})

        self.assertNotIn(".test.TestMessage.NestedType", result.get(".test.TestMessage", set()))

    def test_handles_enum_fields(self):
        """Test that enum field types are handled the same way as message field types."""
        msg = DescriptorProto()
        msg.name = "TestMessage"
        field = msg.field.add()
        field.name = "enum_field"
        field.type = FieldDescriptorProto.TYPE_ENUM
        field.type_name = ".other.SomeEnum"

        result = toposort_add_msg(msg, ".test", {})

        self.assertEqual(result.get(".test.TestMessage"), {".test", ".other.SomeEnum"})

    def test_handles_multiple_fields(self):
        """Test that multiple fields are processed correctly."""
        msg = DescriptorProto()
        msg.name = "TestMessage"
        add_message_field(msg, "same_msg_field", ".test.TestMessage.Nested")
        add_message_field(msg, "other_ns_field", ".other.Type")
        add_message_field(msg, "google_field", ".google.protobuf.Duration")

        deps = toposort_add_msg(msg, ".test", {}).get(".test.TestMessage", set())

        self.assertEqual(deps, {".test", ".other.Type", ".google.protobuf.Duration"})

    def test_real_google_descriptor_proto(self):
        """The actual google/protobuf/descriptor.proto must not report a circular dependency.

        No code is generated for this file but users may import it to declare custom options. This test guards the
        toposort logic against the deeply nested and self referencing messages in that file.
        """
        file_descriptor = FileDescriptorProto()
        file_descriptor.ParseFromString(descriptor_pb2.DESCRIPTOR.serialized_pb)

        dependency_data = {}
        for msg in file_descriptor.message_type:
            dependency_data = toposort_add_msg(msg, "." + file_descriptor.package, dependency_data)

        try:
            order = toposort_flatten(dependency_data)
        except CircularDependencyError as e:
            self.fail("Unexpected circular dependency: " + str(e))

        # A message must appear after the messages it depends on.
        self.assertLess(order.index(".google.protobuf.FieldDescriptorProto"),
                        order.index(".google.protobuf.DescriptorProto"))


if __name__ == "__main__":
    unittest.main()
