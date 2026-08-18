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

from google.protobuf.descriptor_pb2 import DescriptorProto, FieldDescriptorProto
from EmbeddedProto.ProtoFile import toposort_add_msg


class TestToposortAddMsg(unittest.TestCase):
    """Tests for the toposort_add_msg function to ensure it handles circular dependencies."""

    def test_excludes_google_protobuf_types(self):
        """Test that google.protobuf.* types are excluded from dependencies."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add a field referencing a google.protobuf type
        field = msg.field.add()
        field.name = "options_field"
        field.type = FieldDescriptorProto.TYPE_MESSAGE
        field.type_name = ".google.protobuf.FieldOptions"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # The google.protobuf type should NOT be in dependencies
        self.assertNotIn(".google.protobuf.FieldOptions", result.get(".test.TestMessage", set()))
        # Should only have the namespace dependency
        self.assertEqual(result.get(".test.TestMessage"), {".test"})

    def test_excludes_same_namespace_types(self):
        """Test that types in the same namespace are excluded from dependencies."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add a field referencing a type in the same namespace
        field = msg.field.add()
        field.name = "nested_field"
        field.type = FieldDescriptorProto.TYPE_MESSAGE
        field.type_name = ".test.TestMessage.NestedType"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # The same-namespace type should NOT be in dependencies
        self.assertNotIn(".test.TestMessage.NestedType", result.get(".test.TestMessage", set()))
        # Should only have the namespace dependency
        self.assertEqual(result.get(".test.TestMessage"), {".test"})

    def test_includes_different_namespace_types(self):
        """Test that types from different namespaces are included as dependencies."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add a field referencing a type in a different namespace
        field = msg.field.add()
        field.name = "other_field"
        field.type = FieldDescriptorProto.TYPE_MESSAGE
        field.type_name = ".other.SomeType"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # The different-namespace type SHOULD be in dependencies
        self.assertIn(".other.SomeType", result.get(".test.TestMessage", set()))
        # Should have both namespace and the other type
        self.assertEqual(result.get(".test.TestMessage"), {".test", ".other.SomeType"})

    def test_excludes_nested_google_protobuf_types(self):
        """Test that nested google.protobuf.* types are excluded."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add a field referencing a deeply nested google.protobuf type
        field = msg.field.add()
        field.name = "feature_field"
        field.type = FieldDescriptorProto.TYPE_MESSAGE
        field.type_name = ".google.protobuf.FeatureSet"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # The google.protobuf type should NOT be in dependencies
        self.assertNotIn(".google.protobuf.FeatureSet", result.get(".test.TestMessage", set()))

    def test_handles_multiple_fields(self):
        """Test that multiple fields are processed correctly."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add multiple fields with different type references
        field1 = msg.field.add()
        field1.name = "google_field"
        field1.type = FieldDescriptorProto.TYPE_MESSAGE
        field1.type_name = ".google.protobuf.FieldOptions"

        field2 = msg.field.add()
        field2.name = "same_ns_field"
        field2.type = FieldDescriptorProto.TYPE_MESSAGE
        field2.type_name = ".test.TestMessage.Nested"

        field3 = msg.field.add()
        field3.name = "other_ns_field"
        field3.type = FieldDescriptorProto.TYPE_MESSAGE
        field3.type_name = ".other.Type"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # Only the different namespace type should be in dependencies
        deps = result.get(".test.TestMessage", set())
        self.assertNotIn(".google.protobuf.FieldOptions", deps)
        self.assertNotIn(".test.TestMessage.Nested", deps)
        self.assertIn(".other.Type", deps)
        self.assertIn(".test", deps)

    def test_handles_enum_fields(self):
        """Test that enum field types are also handled correctly."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add an enum field referencing a google.protobuf enum
        field = msg.field.add()
        field.name = "enum_field"
        field.type = FieldDescriptorProto.TYPE_ENUM
        field.type_name = ".google.protobuf.FieldOptions.JSType"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # The google.protobuf enum should NOT be in dependencies
        self.assertNotIn(".google.protobuf.FieldOptions.JSType", result.get(".test.TestMessage", set()))

    def test_handles_local_definitions(self):
        """Test that locally defined types in the same message are not added as dependencies."""
        msg = DescriptorProto()
        msg.name = "TestMessage"

        # Add a nested message
        nested_msg = msg.nested_type.add()
        nested_msg.name = "NestedType"

        # Add a field referencing the nested message
        field = msg.field.add()
        field.name = "nested_field"
        field.type = FieldDescriptorProto.TYPE_MESSAGE
        field.type_name = ".test.TestMessage.NestedType"

        dependency_data = {}
        result = toposort_add_msg(msg, ".test", dependency_data)

        # The nested type is in local_definitions, so it should not be in dependencies
        deps = result.get(".test.TestMessage", set())
        self.assertNotIn(".test.TestMessage.NestedType", deps)


if __name__ == "__main__":
    unittest.main()
