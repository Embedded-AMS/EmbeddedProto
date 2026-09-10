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

# Drives the actual code generator (the protoc-gen-eams plugin) with an external field options file. This is the
# interface seam for the feature: what matters is that an option supplied from the file reaches the generated code
# exactly as the same option written inline in the .proto does. The file format itself is covered by
# test_options_file.py.

import json
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
    'package opttest;\n'
)

# Every option of every kind, written inline. The equivalence test below generates this and the same message without
# any inline option but with an options file, and requires the two headers to be identical.
MESSAGE_WITH_INLINE_OPTIONS = (
    'message M {\n'
    '  repeated int32 values = 1 [(EmbeddedProto.options).maxLength = 16];\n'
    '  string name = 2 [(EmbeddedProto.options).maxLength = 32];\n'
    '  repeated string tags = 3 [(EmbeddedProto.options).maxLength = 4,\n'
    '                            (EmbeddedProto.options).nestedMaxLength = 12];\n'
    '  bytes blob = 4 [(EmbeddedProto.options).callbackStorage = true];\n'
    '  repeated int32 custom = 5 [(EmbeddedProto.options).customStorage = true];\n'
    '  Inner inner = 6;\n'
    '  message Inner { string label = 1 [(EmbeddedProto.options).maxLength = 8]; }\n'
    '}\n'
)

MESSAGE_WITHOUT_OPTIONS = (
    'message M {\n'
    '  repeated int32 values = 1;\n'
    '  string name = 2;\n'
    '  repeated string tags = 3;\n'
    '  bytes blob = 4;\n'
    '  repeated int32 custom = 5;\n'
    '  Inner inner = 6;\n'
    '  message Inner { string label = 1; }\n'
    '}\n'
)

THE_SAME_OPTIONS_IN_A_FILE = {
    "opttest": {
        "M": {
            "values": {"maxLength": 16},
            "name": {"maxLength": 32},
            "tags": {"maxLength": 4, "nestedMaxLength": 12},
            "blob": {"callbackStorage": True},
            "custom": {"customStorage": True},
            "Inner": {"label": {"maxLength": 8}},
        }
    }
}

# The generated header carries the moment it was generated. Two runs can straddle a second, so the line is removed
# before two headers are compared.
GENERATED_ON = re.compile(r"^.*Generated on:.*$", re.MULTILINE)


def run_generator(message_definition, options_files=None, parameters=(), proto_sub_dir=""):
    """Generate code for one message and return (result, generated header text or None).

    ``options_files`` maps a file name to the object to write as JSON; the file lands next to the .proto unless the
    name is an absolute path. ``parameters`` are passed as --eams_opt flags, ``{dir}`` in them is replaced by the
    temporary directory so a test can point at a file by absolute path. ``proto_sub_dir`` puts the .proto in a
    sub directory, standing in for a schema that lives somewhere you do not control.
    """
    with tempfile.TemporaryDirectory() as tmp_dir:
        proto_dir = os.path.join(tmp_dir, proto_sub_dir) if proto_sub_dir else tmp_dir
        os.makedirs(proto_dir, exist_ok=True)

        proto_path = os.path.join(proto_dir, "case.proto")
        with open(proto_path, "w") as proto_file:
            proto_file.write(PROTO_HEADER + message_definition)

        for name, content in (options_files or {}).items():
            path = name if os.path.isabs(name) else os.path.join(tmp_dir, name)
            with open(path, "w") as options_file:
                json.dump(content, options_file)

        out_dir = os.path.join(tmp_dir, "generated")
        os.makedirs(out_dir, exist_ok=True)

        command = [
            sys.executable, "-m", "grpc_tools.protoc",
            "--plugin=protoc-gen-eams=" + PLUGIN,
            "-I" + OPTIONS_INCLUDE,
            "-I" + proto_dir,
            "--eams_out=" + out_dir,
        ]
        command += ["--eams_opt=" + parameter.format(dir=tmp_dir) for parameter in parameters]
        command.append(proto_path)
        result = subprocess.run(command, capture_output=True, text=True, cwd=REPO_ROOT)

        header_path = os.path.join(out_dir, "case.h")
        header = None
        if os.path.isfile(header_path):
            with open(header_path) as generated:
                header = GENERATED_ON.sub("", generated.read())

        return result, header


class OptionsFromAFile(unittest.TestCase):

    def test_a_file_generates_the_same_code_as_the_inline_options(self):
        # The acceptance criterion of the whole feature: every option is settable from the file, and the result is
        # what writing it in the .proto would have produced. Comparing generator output with generator output keeps
        # this test alive when the templates change.
        inline_result, inline_header = run_generator(MESSAGE_WITH_INLINE_OPTIONS)
        self.assertEqual(0, inline_result.returncode, inline_result.stderr)

        file_result, file_header = run_generator(
            MESSAGE_WITHOUT_OPTIONS,
            options_files={"board.options.json": THE_SAME_OPTIONS_IN_A_FILE},
            parameters=("options_file={dir}/board.options.json",))
        self.assertEqual(0, file_result.returncode, file_result.stderr)

        self.assertIsNotNone(inline_header)
        self.assertEqual(inline_header, file_header)

    def test_without_a_parameter_nothing_changes(self):
        result, header = run_generator(MESSAGE_WITH_INLINE_OPTIONS)
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("32", header)

    def test_a_schema_in_another_directory_is_configured_from_outside_it(self):
        # The reason the file exists: the .proto comes from a library you do not own, so neither the schema nor a
        # file next to it can be edited. The options file is keyed by package, not by path, and lives elsewhere.
        result, header = run_generator(
            MESSAGE_WITHOUT_OPTIONS,
            proto_sub_dir=os.path.join("vendor", "foo"),
            options_files={"board.options.json": THE_SAME_OPTIONS_IN_A_FILE},
            parameters=("options_file={dir}/board.options.json",))
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("32", header)

    def test_a_dotted_scope_reaches_the_same_field(self):
        # The scope of a field may be written as a dotted path instead of nested objects. Both must arrive at the
        # same field, a spelling that quietly reached nothing would leave the buffer at its default size.
        result, header = run_generator(
            'message M { string name = 1; }\n',
            options_files={"board.options.json": {"opttest.M": {"name": {"maxLength": 24}}}},
            parameters=("options_file={dir}/board.options.json",))
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("24", header)


        result, header = run_generator(
            'message M { string name = 1 [(EmbeddedProto.options).maxLength = 10]; }\n',
            options_files={"board.options.json": {"opttest": {"M": {"name": {"maxLength": 20}}}}},
            parameters=("options_file={dir}/board.options.json",))
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("20", header)
        self.assertNotIn("FieldString<10>", header.replace(" ", ""))
        # An override of a value that is written in the .proto is reported, it must not happen unnoticed.
        self.assertIn("overrides", result.stderr)

    def test_parameters_written_before_the_output_directory_also_work(self):
        # Protoc accepts plugin parameters in two places: as --eams_opt flags, used everywhere else in these tests,
        # and comma separated in front of the output directory. Both end up in the same request, and both are
        # documented, so both are covered.
        with tempfile.TemporaryDirectory() as tmp_dir:
            proto_path = os.path.join(tmp_dir, "case.proto")
            with open(proto_path, "w") as proto_file:
                proto_file.write(PROTO_HEADER + 'message M { string name = 1; }\n')

            options_path = os.path.join(tmp_dir, "board.options.json")
            with open(options_path, "w") as options_file:
                json.dump({"opttest": {"M": {"name": {"maxLength": 40}}}}, options_file)

            out_dir = os.path.join(tmp_dir, "generated")
            os.makedirs(out_dir)

            result = subprocess.run([
                sys.executable, "-m", "grpc_tools.protoc",
                "--plugin=protoc-gen-eams=" + PLUGIN,
                "-I" + OPTIONS_INCLUDE,
                "-I" + tmp_dir,
                "--eams_out=options_file=" + options_path + ":" + out_dir,
                proto_path,
            ], capture_output=True, text=True, cwd=REPO_ROOT)

            self.assertEqual(0, result.returncode, result.stderr)
            with open(os.path.join(out_dir, "case.h")) as generated:
                self.assertIn("40", generated.read())

    def test_the_last_of_two_files_wins(self):
        result, header = run_generator(
            'message M { string name = 1; }\n',
            options_files={"base.options.json": {"opttest": {"M": {"name": {"maxLength": 10}}}},
                           "board.options.json": {"opttest": {"M": {"name": {"maxLength": 20}}}}},
            parameters=("options_file={dir}/base.options.json",
                        "options_file={dir}/board.options.json"))
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("20", header)

    def test_an_option_from_the_file_is_validated_like_an_inline_one(self):
        # callbackStorage is rejected on a repeated string. Setting it from the file must hit the same check, or the
        # file would be a way around the rules the generator enforces.
        result, _ = run_generator(
            'message M { repeated string tags = 1; }\n',
            options_files={"board.options.json": {"opttest": {"M": {"tags": {"callbackStorage": True}}}}},
            parameters=("options_file={dir}/board.options.json",))
        self.assertNotEqual(0, result.returncode)
        self.assertIn("callbackStorage", result.stderr)


class RejectedByTheGenerator(unittest.TestCase):

    def test_a_missing_options_file_is_an_error(self):
        # Never a silent fall back to the default sizes: the user asked for this file by name.
        result, _ = run_generator(
            MESSAGE_WITHOUT_OPTIONS,
            parameters=("options_file={dir}/absent.options.json",))
        self.assertNotEqual(0, result.returncode)
        self.assertIn("absent.options.json", result.stderr)

    def test_a_misspelled_field_is_an_error(self):
        result, _ = run_generator(
            MESSAGE_WITHOUT_OPTIONS,
            options_files={"board.options.json": {"opttest": {"M": {"naem": {"maxLength": 32}}}}},
            parameters=("options_file={dir}/board.options.json",))
        self.assertNotEqual(0, result.returncode)
        self.assertIn("opttest.M.naem", result.stderr)

    def test_an_unknown_parameter_is_an_error(self):
        result, _ = run_generator(MESSAGE_WITHOUT_OPTIONS, parameters=("nonsense=1",))
        self.assertNotEqual(0, result.returncode)
        self.assertIn("nonsense", result.stderr)


if __name__ == "__main__":
    unittest.main()
