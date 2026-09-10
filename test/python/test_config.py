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

"""Unit tests for EmbeddedProto/config.py (the user config file)."""

import os
import sys
import tempfile
import unittest
from unittest import mock

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from EmbeddedProto import config  # noqa: E402

VALID_TOKEN = "abcd1234.ABCD-5678_xyz:0"


class _Base(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        # Redirect both the POSIX (XDG) and Windows (APPDATA) config homes.
        env = {"XDG_CONFIG_HOME": self._tmp.name, "APPDATA": self._tmp.name}
        self._env_patch = mock.patch.dict(os.environ, env)
        self._env_patch.start()

    def tearDown(self):
        self._env_patch.stop()
        self._tmp.cleanup()


class ConfigPath(_Base):
    def test_path_under_config_home(self):
        path = config.config_path()
        self.assertEqual(path, os.path.join(self._tmp.name, "embeddedproto", "config.ini"))


class EnsureDefault(_Base):
    def test_creates_commented_default_0600(self):
        config.ensure_default()
        path = config.config_path()
        self.assertTrue(os.path.exists(path))
        with open(path, encoding="utf-8") as handle:
            text = handle.read()
        self.assertIn("[license]", text)
        self.assertIn("token =", text)
        self.assertIn(config.DEFAULT_SERVER_URL, text)
        if os.name == "posix":
            self.assertEqual(os.stat(path).st_mode & 0o777, 0o600)

    def test_does_not_clobber_existing(self):
        config.set_values(token=VALID_TOKEN)
        config.ensure_default()  # must be a no-op now
        self.assertEqual(config.load()["token"], VALID_TOKEN)

    def test_best_effort_no_raise_without_home(self):
        with mock.patch.object(config, "config_path", return_value=None):
            config.ensure_default()  # must not raise


class LoadAndSet(_Base):
    def test_missing_file_is_all_none(self):
        self.assertEqual(config.load(),
                         {"token": None, "server_url": None})

    def test_set_values_round_trip(self):
        config.set_values(token=VALID_TOKEN,
                                  server_url="https://staging.example/v1/header")
        loaded = config.load()
        self.assertEqual(loaded["token"], VALID_TOKEN)
        self.assertEqual(loaded["server_url"], "https://staging.example/v1/header")

    def test_set_values_preserves_other_key(self):
        config.set_values(token=VALID_TOKEN,
                                  server_url="https://a.example/v1/header")
        # Update only the URL; the token must survive.
        config.set_values(server_url="https://b.example/v1/header")
        loaded = config.load()
        self.assertEqual(loaded["token"], VALID_TOKEN)
        self.assertEqual(loaded["server_url"], "https://b.example/v1/header")

    def test_set_values_writes_0600(self):
        path = config.set_values(token=VALID_TOKEN)
        if os.name == "posix":
            self.assertEqual(os.stat(path).st_mode & 0o777, 0o600)


class Permissions(_Base):
    @unittest.skipUnless(os.name == "posix", "POSIX permission check")
    def test_group_readable_inline_token_ignored_url_kept(self):
        config.set_values(token=VALID_TOKEN,
                                  server_url="https://cfg.example/v1/header")
        os.chmod(config.config_path(), 0o644)
        loaded = config.load()
        self.assertIsNone(loaded["token"])  # secret ignored
        self.assertEqual(loaded["server_url"], "https://cfg.example/v1/header")

    @unittest.skipUnless(os.name == "posix", "POSIX permission check")
    def test_check_perms_false_returns_token(self):
        config.set_values(token=VALID_TOKEN)
        os.chmod(config.config_path(), 0o644)
        # Internal preserve-path bypasses the permission check.
        self.assertEqual(config.load(check_perms=False)["token"], VALID_TOKEN)


if __name__ == "__main__":
    unittest.main()
