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


"""Unit tests for the version parsing in EmbeddedProto/main.py."""

import os
import sys
import unittest

# Make the repository root importable when run as `python -m unittest`.
_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from EmbeddedProto.main import parse_version, load_version_info


class TestParseVersion(unittest.TestCase):

    def test_final_release(self):
        self.assertEqual(parse_version("4.1.0"),
                         {"major": 4, "minor": 1, "patch": 0, "string": "4.1.0"})

    def test_beta_release(self):
        self.assertEqual(parse_version("4.1.0b1"),
                         {"major": 4, "minor": 1, "patch": 0, "string": "4.1.0b1"})

    def test_release_candidate(self):
        self.assertEqual(parse_version("4.1.0rc2"),
                         {"major": 4, "minor": 1, "patch": 0, "string": "4.1.0rc2"})

    def test_development_build(self):
        self.assertEqual(parse_version("4.0.12.dev7"),
                         {"major": 4, "minor": 0, "patch": 12, "string": "4.0.12.dev7"})

    def test_malformed(self):
        for bad in ["4.1", "v4.1.0", "4.1.x", ""]:
            with self.subTest(version=bad):
                with self.assertRaises(Exception):
                    parse_version(bad)

    def test_load_version_info_reads_version_json(self):
        info = load_version_info()
        for key in ("major", "minor", "patch"):
            self.assertIsInstance(info[key], int)
        self.assertTrue(info["string"].startswith("%d.%d.%d" % (info["major"], info["minor"], info["patch"])))


if __name__ == "__main__":
    unittest.main()
