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

"""Unit tests for EmbeddedProto/custom_header.py.

The network is always mocked: no test reaches a real server. Filesystem state
(token file, cache) is redirected to a per-test temporary directory via the
XDG_* environment variables.
"""

import io
import json
import os
import sys
import tempfile
import unittest
from unittest import mock

# Make the repository root importable when run as `python -m unittest`.
_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from EmbeddedProto import custom_header  # noqa: E402

VALID_TOKEN = "abcd1234.ABCD-5678_xyz:0"  # matches ^[A-Za-z0-9._:-]{16,256}$
OTHER_TOKEN = "zzzz9999.YYYY-0000_qqq:1"
FIXED_NOW = 1_000_000.0


def _ok_response(body):
    """A fake urlopen context manager returning HTTP 200 with a JSON body."""
    raw = json.dumps(body).encode("utf-8")
    resp = mock.MagicMock()
    resp.getcode.return_value = 200
    resp.read.return_value = raw
    resp.__enter__.return_value = resp
    resp.__exit__.return_value = False
    return resp


class _Base(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        root = self._tmp.name
        self.config_home = os.path.join(root, "config")
        self.cache_home = os.path.join(root, "cache")
        os.makedirs(self.config_home)
        os.makedirs(self.cache_home)

        # Clear every contract env var, then point XDG at the temp dirs.
        env = {
            "XDG_CONFIG_HOME": self.config_home,
            "XDG_CACHE_HOME": self.cache_home,
        }
        for name in ("EMBEDDEDPROTO_BUILD_TOKEN", "EMBEDDEDPROTO_LICENSE_URL",
                     "EMBEDDEDPROTO_LICENSE_CHECKIN_INTERVAL",
                     "EMBEDDEDPROTO_LICENSE_DISABLE_CHECKIN"):
            os.environ.pop(name, None)
        self._env_patch = mock.patch.dict(os.environ, env)
        self._env_patch.start()

        # Freeze time so check-in/grace windows are deterministic.
        self._time_patch = mock.patch.object(
            custom_header.time, "time", return_value=FIXED_NOW)
        self._time_patch.start()

    def tearDown(self):
        self._time_patch.stop()
        self._env_patch.stop()
        self._tmp.cleanup()

    # -- helpers ---------------------------------------------------------------

    def write_config(self, token="", server_url="", mode=0o600):
        """Write a config.ini with the given token/server_url and permissions."""
        path = custom_header.config.config_path()
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write("[license]\ntoken = %s\nserver_url = %s\n"
                         % (token, server_url))
        if os.name == "posix":
            os.chmod(path, mode)
        return path

    def cache_path(self, token):
        return custom_header._cache_file(token)

    def write_cache(self, token, header, fetched_at, expires_at,
                    last_attempt_at=None):
        # last_attempt_at defaults to fetched_at (the timestamp of a successful
        # fetch is also when that attempt happened).
        if last_attempt_at is None:
            last_attempt_at = fetched_at
        path = self.cache_path(token)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", encoding="utf-8") as handle:
            json.dump({"header": header, "fetched_at": fetched_at,
                       "expires_at": expires_at,
                       "last_attempt_at": last_attempt_at}, handle)
        return path


class TokenDiscovery(_Base):
    def test_env_token(self):
        os.environ["EMBEDDEDPROTO_BUILD_TOKEN"] = VALID_TOKEN
        self.assertEqual(custom_header.resolve_token(), VALID_TOKEN)

    def test_config_token_fallback(self):
        self.write_config(token=VALID_TOKEN)
        self.assertEqual(custom_header.resolve_token(), VALID_TOKEN)

    def test_env_overrides_config(self):
        self.write_config(token=OTHER_TOKEN)
        os.environ["EMBEDDEDPROTO_BUILD_TOKEN"] = VALID_TOKEN
        self.assertEqual(custom_header.resolve_token(), VALID_TOKEN)

    def test_no_token_anywhere(self):
        self.assertIsNone(custom_header.resolve_token())

    def test_malformed_env_token_rejected(self):
        os.environ["EMBEDDEDPROTO_BUILD_TOKEN"] = "too-short"
        self.assertIsNone(custom_header.resolve_token())

    @unittest.skipUnless(os.name == "posix", "POSIX permission check")
    def test_group_readable_config_token_rejected(self):
        self.write_config(token=VALID_TOKEN, mode=0o644)
        self.assertIsNone(custom_header.resolve_token())

    @unittest.skipUnless(os.name == "posix", "POSIX permission check")
    def test_owner_only_config_token_accepted(self):
        self.write_config(token=VALID_TOKEN, mode=0o600)
        self.assertEqual(custom_header.resolve_token(), VALID_TOKEN)


class ServerUrlResolution(_Base):
    def test_default_when_no_config(self):
        self.assertEqual(custom_header._server_url(),
                         custom_header.DEFAULT_SERVER_URL)

    def test_url_from_config(self):
        self.write_config(server_url="https://cfg.example/v1/header")
        self.assertEqual(custom_header._server_url(),
                         "https://cfg.example/v1/header")

    def test_env_url_overrides_config(self):
        self.write_config(server_url="https://cfg.example/v1/header")
        os.environ["EMBEDDEDPROTO_LICENSE_URL"] = "https://env.example/v1/header"
        self.assertEqual(custom_header._server_url(),
                         "https://env.example/v1/header")


class FirstRunScaffold(_Base):
    def test_no_token_creates_default_config(self):
        path = custom_header.config.config_path()
        self.assertFalse(os.path.exists(path))
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(204, None)):
            custom_header.resolve_custom_header(None, "4.0.0")
        self.assertTrue(os.path.exists(path))

    def test_env_token_skips_scaffold(self):
        os.environ["EMBEDDEDPROTO_BUILD_TOKEN"] = VALID_TOKEN
        path = custom_header.config.config_path()
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": "/* h */"})):
            custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertFalse(os.path.exists(path))


class CheckToken(_Base):
    def test_uses_server_url_and_token(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": "/* h */"})) as post:
            status, data = custom_header.check_token(
                VALID_TOKEN, url="https://probe.example/v1/header",
                plugin_version="4.0.0")
        self.assertEqual(status, 200)
        url, payload = post.call_args.args
        self.assertEqual(url, "https://probe.example/v1/header")
        self.assertEqual(payload["token"], VALID_TOKEN)


class Configure(_Base):
    def test_set_token_success_writes_and_returns_zero(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": "/* h */"})):
            rc = custom_header.configure(
                token=VALID_TOKEN, server_url="https://staging.example/v1/header")
        self.assertEqual(rc, 0)
        self.assertEqual(custom_header.config.load()["token"], VALID_TOKEN)
        self.assertEqual(custom_header.config.load()["server_url"],
                         "https://staging.example/v1/header")

    def test_malformed_token_is_hard_error_no_write(self):
        rc = custom_header.configure(token="too-short")
        self.assertEqual(rc, 1)
        self.assertIsNone(custom_header.config.load()["token"])

    def test_non_https_url_is_hard_error(self):
        rc = custom_header.configure(server_url="http://insecure.example/v1")
        self.assertEqual(rc, 1)
        self.assertIsNone(custom_header.config.load()["server_url"])

    def test_failed_check_still_returns_zero(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(None, None)):
            rc = custom_header.configure(token=VALID_TOKEN)
        self.assertEqual(rc, 0)  # a build is never blocked by a failed check

    def test_check_only_writes_nothing(self):
        os.environ["EMBEDDEDPROTO_BUILD_TOKEN"] = VALID_TOKEN
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": "/* h */"})):
            rc = custom_header.configure(check_only=True)
        self.assertEqual(rc, 0)
        self.assertFalse(os.path.exists(custom_header.config.config_path()))


class CachingFlow(_Base):
    def test_fresh_cache_no_network(self):
        self.write_cache(VALID_TOKEN, "/* cached */", FIXED_NOW - 10,
                         FIXED_NOW + 86400)
        with mock.patch.object(custom_header, "_post_json") as post:
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertEqual(header, "/* cached */")
        post.assert_not_called()

    def test_stale_cache_refresh_success_rewrites_cache(self):
        # Last attempt older than the check-in interval -> a check-in is made.
        self.write_cache(VALID_TOKEN, "/* old */", FIXED_NOW - 7200,
                         FIXED_NOW + 100)
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": "/* new */",
                                                   "ttl_seconds": 600})):
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertEqual(header, "/* new */")
        cached = custom_header._read_cache(self.cache_path(VALID_TOKEN))
        self.assertEqual(cached["header"], "/* new */")
        self.assertEqual(cached["fetched_at"], FIXED_NOW)
        self.assertEqual(cached["expires_at"], FIXED_NOW + 600)  # clamped ttl
        self.assertEqual(cached["last_attempt_at"], FIXED_NOW)

    def test_stale_cache_refresh_failure_uses_stale_within_grace(self):
        self.write_cache(VALID_TOKEN, "/* stale */", FIXED_NOW - 7200,
                         FIXED_NOW + 100)
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(None, None)):
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertEqual(header, "/* stale */")

    def test_refresh_failure_past_grace_no_usable_cache(self):
        self.write_cache(VALID_TOKEN, "/* expired */", FIXED_NOW - 7200,
                         FIXED_NOW - 10)  # already past offline grace
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(None, None)):
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertIsNone(header)

    def test_no_cache_refresh_success(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": "/* fresh */"})):
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertEqual(header, "/* fresh */")
        cached = custom_header._read_cache(self.cache_path(VALID_TOKEN))
        self.assertEqual(cached["expires_at"], FIXED_NOW + 86400)  # default grace

    def test_unknown_token_204_no_cache_returns_none(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(204, None)):
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertIsNone(header)

    def test_malformed_200_body_falls_back(self):
        # 200 but header fails sanitisation -> no header, and (no cache) None.
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(200, {"header": 12345})):
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertIsNone(header)


class CheckinThrottle(_Base):
    """The check-in interval is decoupled from header validity and gated on the
    last *attempt*, so rapid repeats collapse and a down server never stalls
    every build."""

    def test_within_interval_serves_cache_no_network(self):
        self.write_cache(VALID_TOKEN, "/* cached */", FIXED_NOW - 7200,
                         FIXED_NOW + 86400, last_attempt_at=FIXED_NOW - 10)
        with mock.patch.object(custom_header, "_post_json") as post:
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertEqual(header, "/* cached */")
        post.assert_not_called()

    def test_failed_attempt_records_attempt_and_throttles_next(self):
        # Header still valid (within grace), but the last attempt is older than the
        # interval, so the first build attempts a check-in; the server is down.
        self.write_cache(VALID_TOKEN, "/* stale */", FIXED_NOW - 7200,
                         FIXED_NOW + 100, last_attempt_at=FIXED_NOW - 400)
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(None, None)) as post:
            first = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
            second = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertEqual(first, "/* stale */")
        self.assertEqual(second, "/* stale */")
        post.assert_called_once()  # second build is throttled despite the failure
        cached = custom_header._read_cache(self.cache_path(VALID_TOKEN))
        self.assertEqual(cached["last_attempt_at"], FIXED_NOW)

    def test_no_cache_failure_writes_marker_and_throttles(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(None, None)) as post:
            first = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
            second = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertIsNone(first)
        self.assertIsNone(second)
        post.assert_called_once()  # marker throttles the retry
        cached = custom_header._read_cache(self.cache_path(VALID_TOKEN))
        self.assertIsNone(cached["header"])
        self.assertEqual(cached["last_attempt_at"], FIXED_NOW)

    def test_expired_header_within_interval_returns_none(self):
        # Recent attempt (throttled) but the header is past its grace window.
        self.write_cache(VALID_TOKEN, "/* expired */", FIXED_NOW - 7200,
                         FIXED_NOW - 10, last_attempt_at=FIXED_NOW - 10)
        with mock.patch.object(custom_header, "_post_json") as post:
            header = custom_header.resolve_custom_header(VALID_TOKEN, "4.0.0")
        self.assertIsNone(header)
        post.assert_not_called()


class FreeInstallCheckin(_Base):
    def test_no_token_sends_throttled_checkin(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(204, None)) as post:
            header = custom_header.resolve_custom_header(None, "4.0.0")
        self.assertIsNone(header)
        post.assert_called_once()
        # Tokenless body, carries plugin_version.
        _url, payload = post.call_args.args
        self.assertNotIn("token", payload)
        self.assertEqual(payload["plugin_version"], "4.0.0")
        # Marker written so the next build within the window is throttled.
        self.assertTrue(os.path.exists(custom_header._free_marker_path()))

    def test_checkin_throttled_within_window(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(204, None)) as post:
            custom_header.resolve_custom_header(None, "4.0.0")
            custom_header.resolve_custom_header(None, "4.0.0")
        self.assertEqual(post.call_count, 1)

    def test_checkin_fires_again_after_window(self):
        with mock.patch.object(custom_header, "_post_json",
                               return_value=(204, None)) as post:
            custom_header.resolve_custom_header(None, "4.0.0")
        # Advance beyond the freshness window and retry.
        with mock.patch.object(custom_header.time, "time",
                               return_value=FIXED_NOW + 4000), \
                mock.patch.object(custom_header, "_post_json",
                                  return_value=(204, None)) as post2:
            custom_header.resolve_custom_header(None, "4.0.0")
        self.assertEqual(post.call_count, 1)
        self.assertEqual(post2.call_count, 1)

    def test_opt_out_suppresses_checkin(self):
        os.environ["EMBEDDEDPROTO_LICENSE_DISABLE_CHECKIN"] = "1"
        with mock.patch.object(custom_header, "_post_json") as post:
            header = custom_header.resolve_custom_header(None, "4.0.0")
        self.assertIsNone(header)
        post.assert_not_called()


class Sanitisation(_Base):
    def test_oversized_header_rejected(self):
        self.assertIsNone(custom_header._sanitize_header("x" * 8193))

    def test_max_size_header_accepted(self):
        text = "x" * 8192
        self.assertEqual(custom_header._sanitize_header(text), text)

    def test_control_chars_rejected_except_tab_newline(self):
        self.assertIsNone(custom_header._sanitize_header("bad\x00null"))
        self.assertEqual(custom_header._sanitize_header("a\tb\nc"), "a\tb\nc")

    def test_crlf_normalised(self):
        self.assertEqual(custom_header._sanitize_header("a\r\nb\rc"), "a\nb\nc")

    def test_clamp_grace_bounds(self):
        self.assertEqual(custom_header._clamp_grace(10), 300)
        self.assertEqual(custom_header._clamp_grace(99999999), 2592000)
        self.assertEqual(custom_header._clamp_grace("nope"),
                         custom_header._DEFAULT_GRACE)


class NetworkLayer(_Base):
    def test_non_https_url_refused(self):
        os.environ["EMBEDDEDPROTO_LICENSE_URL"] = "http://insecure.example/v1"
        with mock.patch("urllib.request.urlopen") as urlopen:
            status, data = custom_header._post_json(
                custom_header._server_url(), {"token": VALID_TOKEN})
        self.assertIsNone(status)
        self.assertIsNone(data)
        urlopen.assert_not_called()

    def test_https_post_parses_json(self):
        os.environ["EMBEDDEDPROTO_LICENSE_URL"] = "https://example.test/v1/header"
        with mock.patch("urllib.request.urlopen",
                        return_value=_ok_response({"header": "/* h */"})):
            status, data = custom_header._post_json(
                custom_header._server_url(), {"token": VALID_TOKEN})
        self.assertEqual(status, 200)
        self.assertEqual(data, {"header": "/* h */"})

    def test_timeout_returns_none_status(self):
        import socket
        os.environ["EMBEDDEDPROTO_LICENSE_URL"] = "https://example.test/v1/header"
        with mock.patch("urllib.request.urlopen",
                        side_effect=socket.timeout()):
            status, data = custom_header._post_json(
                custom_header._server_url(), {"token": VALID_TOKEN})
        self.assertIsNone(status)
        self.assertIsNone(data)


if __name__ == "__main__":
    unittest.main()
