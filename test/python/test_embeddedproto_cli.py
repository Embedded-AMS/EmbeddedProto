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
import sys
import unittest
from unittest import mock

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from EmbeddedProto import EmbeddedProto as cli  # noqa: E402


class BuildProtocArgvTest(unittest.TestCase):

    def test_include_paths_are_appended(self):
        argv = cli.build_protoc_argv(["-Iproto", "--eams_out=gen", "proto/a.proto"])
        includes = [x[2:] for x in argv if x.startswith("-I")]
        self.assertEqual("proto", includes[0])
        self.assertTrue(os.path.isfile(os.path.join(includes[1], "embedded_proto_options.proto")))
        self.assertTrue(os.path.isfile(os.path.join(includes[2], "google", "protobuf", "descriptor.proto")))
        self.assertEqual(["--eams_out=gen", "proto/a.proto"], [x for x in argv[1:] if not x.startswith("-I")])

    def test_legacy_well_known_types_flag_is_dropped(self):
        argv = cli.build_protoc_argv(["--IncludeWellKnownTypes", "a.proto"])
        self.assertNotIn("--IncludeWellKnownTypes", argv)

    def test_user_plugin_is_kept(self):
        argv = cli.build_protoc_argv(["--plugin=protoc-gen-eams=/my/plugin", "a.proto"])
        self.assertEqual(["--plugin=protoc-gen-eams=/my/plugin"], [x for x in argv if x.startswith("--plugin")])


class RunProtocTest(unittest.TestCase):

    def test_exit_code_of_protoc_is_returned(self):
        with mock.patch.object(cli.protoc, "main", return_value=3):
            with self.assertRaises(SystemExit) as raised:
                cli.run_protoc(["embeddedproto", "a.proto"])
        self.assertEqual(3, raised.exception.code)


if __name__ == "__main__":
    unittest.main()
