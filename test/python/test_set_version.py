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


"""Unit tests for scripts/set_version.py."""

import json
import os
import shutil
import sys
import tempfile
import unittest

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(_REPO_ROOT, "scripts"))

import set_version


class TestDeriveVersion(unittest.TestCase):

    def test_outside_github_actions_keeps_the_base_version(self):
        self.assertEqual(set_version.derive_version("4.1.0", None, None, None), ("4.1.0", False))

    def test_matching_tag_is_a_final_release(self):
        self.assertEqual(set_version.derive_version("4.1.0", "tag", "4.1.0", "12"), ("4.1.0", False))

    def test_mismatching_tag_is_rejected(self):
        with self.assertRaises(set_version.VersionError):
            set_version.derive_version("4.1.0", "tag", "4.1.1", "12")

    def test_release_branch_is_a_numbered_beta(self):
        version = set_version.derive_version("4.1.0", "branch", "release/4.1.0", "12", count_commits=lambda: 3)
        self.assertEqual(version, ("4.1.0b3", True))

    def test_release_branch_for_another_version_is_rejected(self):
        with self.assertRaises(set_version.VersionError):
            set_version.derive_version("4.1.0", "branch", "release/4.2.0", "12", count_commits=lambda: 3)

    def test_other_branch_is_a_development_build(self):
        self.assertEqual(set_version.derive_version("4.1.0", "branch", "develop", "12"), ("4.1.0.dev12", True))

    def test_other_branch_requires_a_run_number(self):
        with self.assertRaises(set_version.VersionError):
            set_version.derive_version("4.1.0", "branch", "develop", None)


class TestVersionFiles(unittest.TestCase):

    def setUp(self):
        self.tmp = tempfile.mkdtemp()
        self.version_json = os.path.join(self.tmp, "version.json")
        self.version_h = os.path.join(self.tmp, "Version.h")
        shutil.copy(set_version.VERSION_JSON, self.version_json)
        shutil.copy(set_version.VERSION_H, self.version_h)

    def tearDown(self):
        shutil.rmtree(self.tmp)

    def test_read_base_version_rejects_a_suffix(self):
        with open(self.version_json, "w") as f:
            json.dump({"version": "4.1.0b1"}, f)
        with self.assertRaises(set_version.VersionError):
            set_version.read_base_version(self.version_json)

    def test_write_version_files_updates_json_and_version_string_only(self):
        with open(self.version_h) as f:
            before = f.read()
        set_version.write_version_files("4.1.0b2", self.version_json, self.version_h)

        with open(self.version_json) as f:
            self.assertEqual(json.load(f)["version"], "4.1.0b2")
        with open(self.version_h) as f:
            after = f.read()
        self.assertIn('#define EMBEDDEDPROTO_VERSION_STRING "4.1.0b2"', after)
        # Everything except the VERSION_STRING line is untouched, the numeric macros included.
        strip = lambda text: [l for l in text.splitlines() if "EMBEDDEDPROTO_VERSION_STRING" not in l]
        self.assertEqual(strip(before), strip(after))


if __name__ == "__main__":
    unittest.main()
