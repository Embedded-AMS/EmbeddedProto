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

"""Unit tests for EmbeddedProto/Features.py (editions feature resolution)."""

import os
import sys
import unittest

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from google.protobuf import descriptor_pb2  # noqa: E402

from EmbeddedProto.Features import (  # noqa: E402
    FeatureResolver,
    FieldPresence,
    EnumType,
    RepeatedFieldEncoding,
    MessageEncoding,
    EDITION_PROTO3,
    EDITION_2023,
    EDITION_2024,
)


def _file(edition):
    """Build a minimal editions FileDescriptorProto for the given edition."""
    fd = descriptor_pb2.FileDescriptorProto()
    fd.name = "test.proto"
    fd.syntax = "editions"
    fd.edition = edition
    return fd


def _proto3_file():
    fd = descriptor_pb2.FileDescriptorProto()
    fd.name = "test.proto"
    fd.syntax = "proto3"
    return fd


class EditionDefaults(unittest.TestCase):
    def test_2023_defaults(self):
        resolver = FeatureResolver(_file(EDITION_2023))
        resolved = resolver.resolve()
        self.assertEqual(resolved["field_presence"], FieldPresence.EXPLICIT)
        self.assertEqual(resolved["enum_type"], EnumType.OPEN)
        self.assertEqual(resolved["repeated_field_encoding"], RepeatedFieldEncoding.PACKED)
        self.assertEqual(resolved["message_encoding"], MessageEncoding.LENGTH_PREFIXED)

    def test_2024_defaults_match_2023_core(self):
        r2023 = FeatureResolver(_file(EDITION_2023)).resolve()
        r2024 = FeatureResolver(_file(EDITION_2024)).resolve()
        self.assertEqual(r2023, r2024)

    def test_proto3_defaults(self):
        resolver = FeatureResolver(_proto3_file())
        self.assertEqual(resolver.edition, EDITION_PROTO3)
        resolved = resolver.resolve()
        # proto3 plain scalars are implicit; presence is added per-field via
        # proto3_optional which is handled in the Field layer, not here.
        self.assertEqual(resolved["field_presence"], FieldPresence.IMPLICIT)


class OverridePrecedence(unittest.TestCase):
    def setUp(self):
        self.resolver = FeatureResolver(_file(EDITION_2023))

    def _opts(self, presence):
        opts = descriptor_pb2.FieldOptions()
        opts.features.field_presence = presence
        return opts

    def test_default_when_no_override(self):
        resolved = self.resolver.resolve()
        self.assertEqual(resolved["field_presence"], FieldPresence.EXPLICIT)

    def test_file_override_beats_default(self):
        file_opts = descriptor_pb2.FileOptions()
        file_opts.features.field_presence = FieldPresence.IMPLICIT
        resolved = self.resolver.resolve(file_opts)
        self.assertEqual(resolved["field_presence"], FieldPresence.IMPLICIT)

    def test_message_override_beats_file(self):
        file_opts = descriptor_pb2.FileOptions()
        file_opts.features.field_presence = FieldPresence.IMPLICIT
        msg_opts = descriptor_pb2.MessageOptions()
        msg_opts.features.field_presence = FieldPresence.LEGACY_REQUIRED
        resolved = self.resolver.resolve(file_opts, msg_opts)
        self.assertEqual(resolved["field_presence"], FieldPresence.LEGACY_REQUIRED)

    def test_field_override_beats_message(self):
        file_opts = descriptor_pb2.FileOptions()
        file_opts.features.field_presence = FieldPresence.IMPLICIT
        msg_opts = descriptor_pb2.MessageOptions()
        msg_opts.features.field_presence = FieldPresence.LEGACY_REQUIRED
        resolved = self.resolver.resolve(file_opts, msg_opts,
                                         self._opts(FieldPresence.EXPLICIT))
        self.assertEqual(resolved["field_presence"], FieldPresence.EXPLICIT)

    def test_unset_feature_falls_through_to_default(self):
        # A field that overrides only repeated encoding must keep the edition's
        # default field presence.
        opts = descriptor_pb2.FieldOptions()
        opts.features.repeated_field_encoding = RepeatedFieldEncoding.EXPANDED
        resolved = self.resolver.resolve(opts)
        self.assertEqual(resolved["repeated_field_encoding"], RepeatedFieldEncoding.EXPANDED)
        self.assertEqual(resolved["field_presence"], FieldPresence.EXPLICIT)


class Edition2024(unittest.TestCase):
    """Edition 2024 is cumulative on 2023: it shares every 2023 feature handler and
    only adds codegen-only features."""

    def test_2024_inherits_2023_feature_handling(self):
        resolver = FeatureResolver(_file(EDITION_2024))
        # A DELIMITED message_encoding override (a 2023 feature) resolves the same
        # way in a 2024 file.
        opts = descriptor_pb2.FieldOptions()
        opts.features.message_encoding = MessageEncoding.DELIMITED
        resolved = resolver.resolve(opts)
        self.assertEqual(resolved["message_encoding"], MessageEncoding.DELIMITED)

    def test_2024_only_feature_is_legal_in_2024(self):
        fd = _file(EDITION_2024)
        fd.options.features.default_symbol_visibility = 1
        # Must not raise.
        FeatureResolver(fd)

    def test_same_usage_resolves_equally_in_2023_and_2024(self):
        opts = descriptor_pb2.FieldOptions()
        opts.features.field_presence = FieldPresence.IMPLICIT
        r2023 = FeatureResolver(_file(EDITION_2023)).resolve(opts)
        r2024 = FeatureResolver(_file(EDITION_2024)).resolve(opts)
        self.assertEqual(r2023, r2024)


class Legality(unittest.TestCase):
    def test_2024_only_feature_rejected_in_2023(self):
        fd = _file(EDITION_2023)
        fd.options.features.enforce_naming_style = 1
        with self.assertRaises(Exception):
            FeatureResolver(fd)

    def test_2024_only_feature_accepted_in_2024(self):
        fd = _file(EDITION_2024)
        fd.options.features.enforce_naming_style = 1
        # Must not raise.
        FeatureResolver(fd)

    def test_unsupported_edition_rejected(self):
        # A valid but unsupported edition enumerator (legacy) must be rejected.
        fd = _file(descriptor_pb2.Edition.Value("EDITION_LEGACY"))
        with self.assertRaises(Exception):
            FeatureResolver(fd)


if __name__ == "__main__":
    unittest.main()
