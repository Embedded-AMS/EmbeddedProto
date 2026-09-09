#
#  Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
#
#  This file is part of Embedded Proto.
#
#  Embedded Proto is open source software: you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as published
#  by the Free Software Foundation, version 3 of the license.
#
#  Embedded Proto  is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
#
#  For commercial and closed source application please visit:
#  <https://embeddedproto.com/pricing/>.
#
#  Embedded AMS B.V.
#  Info:
#    info at EmbeddedProto dot com
#
#  Postal address:
#    Atoomweg 2
#    1627 LE, Hoorn
#    the Netherlands
#
# Drives the actual code generator (the protoc-gen-eams plugin) to verify map support: a map field
# is recognised as such, the key and value sizes reach the generated entry class, and a size option
# on a key or value that can not carry one is rejected with a clear error. These are generator level
# behaviours, they can not be exercised from the C++ tests.

import os
import re
import subprocess
import sys
import tempfile
import unittest

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PLUGIN = os.path.join(REPO_ROOT, "protoc-gen-eams")
OPTIONS_INCLUDE = os.path.join(REPO_ROOT, "EmbeddedProto")

PROTO_HEADER = (
    'syntax = "proto3";\n'
    'import "embedded_proto_options.proto";\n'
    'package maptest;\n'
)


def run_generator(message_definition, header=PROTO_HEADER):
    # Write a proto with the given message to a temporary directory, run the generator on it and
    # return the process result together with the generated header, if any.
    with tempfile.TemporaryDirectory() as tmp_dir:
        proto_path = os.path.join(tmp_dir, "case.proto")
        with open(proto_path, "w") as proto_file:
            proto_file.write(header + message_definition + "\n")

        command = [
            sys.executable, "-m", "grpc_tools.protoc",
            "--plugin=protoc-gen-eams=" + PLUGIN,
            "-I" + OPTIONS_INCLUDE,
            "-I" + tmp_dir,
            "--eams_out=" + tmp_dir,
            proto_path,
        ]
        result = subprocess.run(command, capture_output=True, text=True, cwd=REPO_ROOT)

        generated = ""
        header_path = os.path.join(tmp_dir, "case.h")
        if os.path.exists(header_path):
            with open(header_path) as header_file:
                generated = header_file.read()
        return result, generated


class MapSupport(unittest.TestCase):

    def test_a_map_generates_the_map_api(self):
        result, generated = run_generator(
            'message M { map<string, int32> scores = 1 '
            '[(EmbeddedProto.options) = { maxLength: 4, keyMaxLength: 8 }]; }')
        self.assertEqual(0, result.returncode, result.stderr)

        # The accessors that make it a map rather than a plain repeated field.
        self.assertIn("uint32_t scores_size() const", generated)
        self.assertIn("bool has_scores(const char* key) const", generated)
        self.assertIn("int32_t get_scores(const char* key) const", generated)
        self.assertIn("set_scores(const char* key, int32_t value)", generated)
        self.assertIn("remove_scores(const char* key)", generated)
        self.assertIn("find_scores(const char* key) const", generated)

    def test_a_plain_repeated_message_keeps_the_repeated_api(self):
        # Guards the map detection against catching an ordinary repeated message field.
        result, generated = run_generator(
            'message Point { int32 x = 1; }\n'
            'message M { repeated Point points = 1 '
            '[(EmbeddedProto.options) = { maxLength: 4 }]; }')
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("add_points", generated)
        self.assertNotIn("find_points", generated)
        self.assertNotIn("points_size()", generated)

    def test_key_and_value_sizes_reach_the_entry(self):
        # Without the sizes the entry would expose bare C++ template parameters instead.
        result, generated = run_generator(
            'message M { map<string, string> text = 1 '
            '[(EmbeddedProto.options) = { maxLength: 2, keyMaxLength: 6, valueMaxLength: 10 }]; }')
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("::EmbeddedProto::FieldString<6> key_;", generated)
        self.assertIn("::EmbeddedProto::FieldString<10> value_;", generated)
        # Every size is known, so the message needs no template parameter at all.
        self.assertTrue(re.search(r"^class M final", generated, re.MULTILINE), generated)

    def test_a_map_without_sizes_exposes_template_parameters(self):
        result, generated = run_generator(
            'message M { map<string, int32> scores = 1; }')
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("M_scores_REP_LENGTH", generated)

    def test_key_size_on_a_non_string_key_is_rejected(self):
        result, _ = run_generator(
            'message M { map<int32, int32> scores = 1 '
            '[(EmbeddedProto.options) = { maxLength: 4, keyMaxLength: 8 }]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("keyMaxLength is only valid on a map with a string key", result.stderr)

    def test_value_size_on_a_non_string_value_is_rejected(self):
        result, _ = run_generator(
            'message M { map<string, int32> scores = 1 '
            '[(EmbeddedProto.options) = { maxLength: 4, valueMaxLength: 8 }]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("valueMaxLength is only valid on a map with a string or bytes value",
                      result.stderr)

    def test_callback_storage_on_a_map_is_accepted(self):
        # A map keeps the length delimited entry framing, so unlike a plain repeated message field
        # it does not require features.message_encoding = DELIMITED.
        result, generated = run_generator(
            'message M { map<string, int32> scores = 1 '
            '[(EmbeddedProto.options) = { callbackStorage: true, keyMaxLength: 8 }]; }')
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("::EmbeddedProto::MessageCallback<ScoresEntry> scores_;", generated)
        self.assertIn("serialize_expanded_len", generated)
        # A stream has no entries to search, so no lookup accessors are emitted.
        self.assertNotIn("find_scores", generated)

    def test_callback_storage_on_a_repeated_message_still_requires_delimited(self):
        # The map exemption must not weaken the rule for an ordinary repeated message field.
        result, _ = run_generator(
            'message Point { int32 x = 1; }\n'
            'message M { repeated Point points = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("requires features.message_encoding = DELIMITED", result.stderr)


if __name__ == "__main__":
    unittest.main()
