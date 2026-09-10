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

"""Unit tests for EmbeddedProto/EmbeddedProto.py (the embeddedproto command)."""
import os
import stat
import sys
import tempfile
import unittest
from unittest import mock

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from EmbeddedProto import EmbeddedProto as cli  # noqa: E402


class BuildProtocArgvTest(unittest.TestCase):

    def test_program_name_comes_first(self):
        argv = cli.build_protoc_argv(["a.proto"], program="/some/bin/embeddedproto")
        self.assertEqual("/some/bin/embeddedproto", argv[0])

    def test_include_paths_are_appended(self):
        argv = cli.build_protoc_argv(["-Iproto", "--eams_out=gen", "proto/a.proto"], program="embeddedproto")
        includes = [x[2:] for x in argv if x.startswith("-I")]
        self.assertEqual("proto", includes[0])
        self.assertTrue(os.path.isfile(os.path.join(includes[1], "embedded_proto_options.proto")))
        self.assertTrue(os.path.isfile(os.path.join(includes[2], "google", "protobuf", "descriptor.proto")))
        self.assertEqual(["--eams_out=gen", "proto/a.proto"], [x for x in argv[2:] if not x.startswith("-I")])

    def test_legacy_well_known_types_flag_is_dropped(self):
        argv = cli.build_protoc_argv(["--IncludeWellKnownTypes", "a.proto"], program="embeddedproto")
        self.assertNotIn("--IncludeWellKnownTypes", argv)

    def test_plugin_next_to_the_command_is_used_by_path(self):
        with tempfile.TemporaryDirectory() as bin_dir:
            plugin = os.path.join(bin_dir, "protoc-gen-eams")
            with open(plugin, "w") as f:
                f.write("#!/bin/sh\n")
            os.chmod(plugin, os.stat(plugin).st_mode | stat.S_IXUSR)
            argv = cli.build_protoc_argv(["a.proto"], program=os.path.join(bin_dir, "embeddedproto"))
        self.assertEqual("--plugin=protoc-gen-eams=" + plugin, argv[1])

    def test_plugin_falls_back_to_the_name_when_not_found(self):
        with mock.patch.object(cli.shutil, "which", return_value=None):
            argv = cli.build_protoc_argv(["a.proto"], program="/nowhere/embeddedproto")
        self.assertEqual("--plugin=protoc-gen-eams", argv[1])

    def test_user_plugin_is_kept(self):
        argv = cli.build_protoc_argv(["--plugin=protoc-gen-eams=/my/plugin", "a.proto"], program="embeddedproto")
        self.assertEqual(["--plugin=protoc-gen-eams=/my/plugin"], [x for x in argv if x.startswith("--plugin")])


class RunProtocTest(unittest.TestCase):

    def test_exit_code_of_protoc_is_returned(self):
        with mock.patch.object(cli.protoc, "main", return_value=3):
            with self.assertRaises(SystemExit) as raised:
                cli.run_protoc(["embeddedproto", "a.proto"])
        self.assertEqual(3, raised.exception.code)


if __name__ == "__main__":
    unittest.main()
