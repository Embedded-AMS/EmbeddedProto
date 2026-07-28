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

# Drives the actual code generator (the protoc-gen-eams plugin) to verify the callbackStorage
# option: it is accepted on a repeated scalar field and rejected with a clear error on every
# placement that is not yet supported. This is the interface seam for generator rejection; it
# cannot be exercised from the C++ tests.

import os
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
    'package cbtest;\n'
)

EDITION_HEADER = (
    'edition = "2023";\n'
    'import "embedded_proto_options.proto";\n'
    'package cbtest;\n'
)


def run_generator(message_definition, header=PROTO_HEADER):
    # Write a proto with the given message to a temporary directory and run the generator on it.
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
        return subprocess.run(command, capture_output=True, text=True, cwd=REPO_ROOT)


class CallbackStorageOption(unittest.TestCase):

    def test_repeated_scalar_is_accepted(self):
        result = run_generator(
            'message M { repeated int32 x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertEqual(0, result.returncode, result.stderr)

    def test_singular_bytes_is_accepted(self):
        result = run_generator(
            'message M { bytes x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertEqual(0, result.returncode, result.stderr)

    def test_singular_string_is_accepted(self):
        result = run_generator(
            'message M { string x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertEqual(0, result.returncode, result.stderr)

    def test_oneof_member_is_rejected(self):
        result = run_generator(
            'message M { oneof o { int32 x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; } }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("callbackStorage cannot be used on oneof members", result.stderr)

    def test_singular_scalar_is_rejected(self):
        result = run_generator(
            'message M { int32 x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("singular scalar field is not supported", result.stderr)

    def test_explicit_presence_singular_scalar_is_rejected(self):
        result = run_generator(
            'message M { optional int32 x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("singular scalar field is not supported", result.stderr)

    def test_explicit_presence_bytes_is_rejected(self):
        result = run_generator(
            'message M { optional bytes x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("explicit-presence", result.stderr)

    def test_repeated_string_is_rejected(self):
        result = run_generator(
            'message M { repeated string x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }')
        self.assertNotEqual(0, result.returncode)
        self.assertIn("not yet supported", result.stderr)

    def test_repeated_message_delimited_is_accepted(self):
        result = run_generator(
            'message Inner { int32 a = 1; } '
            'message M { repeated Inner x = 1 ['
            'features.message_encoding = DELIMITED, '
            '(EmbeddedProto.options).callbackStorage = true]; }',
            header=EDITION_HEADER)
        self.assertEqual(0, result.returncode, result.stderr)

    def test_repeated_message_without_delimited_is_rejected(self):
        result = run_generator(
            'message Inner { int32 a = 1; } '
            'message M { repeated Inner x = 1 '
            '[(EmbeddedProto.options).callbackStorage = true]; }',
            header=EDITION_HEADER)
        self.assertNotEqual(0, result.returncode)
        self.assertIn("DELIMITED", result.stderr)


if __name__ == "__main__":
    unittest.main()
