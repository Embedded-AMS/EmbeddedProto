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

"""Resolve an optional custom header to place at the top of generated files.

The header text is fetched from a configurable server for builds that present a
build token, and cached locally so repeated builds reuse it without a network
round-trip. Everything in this module is best-effort: a missing token, an
unreachable server, a slow response, or any malformed reply all resolve to "no
custom header" and the generator falls back to its default banner. Nothing here
ever raises into, blocks, or slows a build beyond a single short timeout.
"""

import hashlib
import json
import os
import re
import socket
import ssl
import sys
import time
import urllib.error
import urllib.request

from EmbeddedProto import config

# --- Contract constants (see the server-side API contract) --------------------

# Re-exported from config (the lowest-level module) so there is a single
# source of truth for the default endpoint.
DEFAULT_SERVER_URL = config.DEFAULT_SERVER_URL
TOKEN_RE = re.compile(r"^[A-Za-z0-9._:-]{16,256}$")

_ENV_TOKEN = "EMBEDDEDPROTO_BUILD_TOKEN"
_ENV_URL = "EMBEDDEDPROTO_LICENSE_URL"
_ENV_INTERVAL = "EMBEDDEDPROTO_LICENSE_CHECKIN_INTERVAL"
_ENV_DISABLE = "EMBEDDEDPROTO_LICENSE_DISABLE_CHECKIN"

_DEFAULT_INTERVAL = 300            # 5 minutes
_DEFAULT_GRACE = 86400             # offline-grace default when server omits ttl
_GRACE_MIN = 300
_GRACE_MAX = 2592000
_TIMEOUT = 5                       # seconds; single attempt, no retry
_MAX_HEADER_BYTES = 8192
_FREE_MARKER = "free-install.json"


# --- Small helpers ------------------------------------------------------------

def _warn(message):
    """Write a hygiene warning to stderr. Never includes the token value."""
    sys.stderr.write("EmbeddedProto custom header: " + message + "\n")


def _env_truthy(value):
    return bool(value) and value.strip().lower() in ("1", "true", "yes", "on")


def _cache_dir():
    """Platform path to the per-token cache directory."""
    if os.name == "nt":
        base = os.environ.get("LOCALAPPDATA")
        return os.path.join(base, "embeddedproto", "licenses") if base else None
    base = os.environ.get("XDG_CACHE_HOME") or os.path.join(
        os.path.expanduser("~"), ".cache")
    return os.path.join(base, "embeddedproto", "licenses")


def _cache_file(token):
    directory = _cache_dir()
    if not directory:
        return None
    digest = hashlib.sha256(token.encode("utf-8")).hexdigest()
    return os.path.join(directory, digest + ".json")


def _checkin_interval():
    raw = os.environ.get(_ENV_INTERVAL, "").strip()
    if not raw:
        return _DEFAULT_INTERVAL
    try:
        return max(0, int(raw))
    except ValueError:
        return _DEFAULT_INTERVAL


def _server_url():
    env_url = os.environ.get(_ENV_URL, "").strip()
    if env_url:
        return env_url
    # check_perms=False: we only need the (non-secret) URL here, so do not run the
    # token permission check (and its warning) for an inline token we won't read.
    return config.load(check_perms=False).get("server_url") or DEFAULT_SERVER_URL


def _atomic_write_json(path, payload):
    """Write JSON via a temp file + rename so a reader never sees a torn file."""
    try:
        os.makedirs(os.path.dirname(path), exist_ok=True)
        tmp = path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as handle:
            json.dump(payload, handle)
        os.replace(tmp, path)
    except OSError:
        pass  # caching is an optimisation, never fatal


# --- Token discovery ----------------------------------------------------------

def resolve_token():
    """Return a validated build token, or None when none is configured.

    Priority: the EMBEDDEDPROTO_BUILD_TOKEN environment variable, then the inline
    token in the user config file. A malformed value is treated as absent.
    """
    try:
        token = os.environ.get(_ENV_TOKEN, "").strip()
        if not token:
            token = (config.load().get("token") or "").strip()
        if not token:
            return None
        if not TOKEN_RE.match(token):
            _warn("ignoring malformed build token")
            return None
        return token
    except Exception:  # pragma: no cover - defensive: never break a build
        return None


# --- Header sanitisation ------------------------------------------------------

def _sanitize_header(text):
    """Apply the contract's header rules; return None if the text is unusable."""
    if not isinstance(text, str):
        return None
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    if len(text.encode("utf-8")) > _MAX_HEADER_BYTES:
        return None
    for char in text:
        if char in ("\n", "\t"):
            continue
        code = ord(char)
        if code < 0x20 or code == 0x7F:
            return None
    return text


def _clamp_grace(ttl_seconds):
    try:
        ttl = int(ttl_seconds)
    except (TypeError, ValueError):
        return _DEFAULT_GRACE
    return max(_GRACE_MIN, min(_GRACE_MAX, ttl))


# --- Cache --------------------------------------------------------------------

def _read_cache(path):
    """Return the cache dict or None.

    Keys: ``header`` (str or None for an attempt-only marker), ``fetched_at``,
    ``expires_at`` and ``last_attempt_at`` (floats). ``last_attempt_at`` records
    the time of the last check-in attempt regardless of its outcome, and is what
    throttles the next attempt.
    """
    try:
        with open(path, "r", encoding="utf-8") as handle:
            data = json.load(handle)
        if not isinstance(data, dict):
            return None
        header = data.get("header")
        if header is not None and not isinstance(header, str):
            header = None
        return {
            "header": header,
            "fetched_at": float(data.get("fetched_at", 0)),
            "expires_at": float(data.get("expires_at", 0)),
            "last_attempt_at": float(data.get("last_attempt_at", 0)),
        }
    except (OSError, ValueError, TypeError):
        return None


def _write_cache(path, header, fetched_at, expires_at, last_attempt_at):
    """Persist the cache dict (best-effort; see _atomic_write_json)."""
    if path:
        _atomic_write_json(path, {
            "header": header,
            "fetched_at": fetched_at,
            "expires_at": expires_at,
            "last_attempt_at": last_attempt_at,
        })


def _usable_header(cached, now):
    """The cached header if still within its offline-grace window, else None."""
    if cached is not None and cached["header"] is not None and now < cached["expires_at"]:
        return cached["header"]
    return None


# --- Network ------------------------------------------------------------------

def _post_json(url, payload):
    """POST a JSON body. Return (status, parsed_dict_or_None). Never raises.

    A status of None means the request did not complete (timeout, DNS, TLS,
    connection error). HTTPS is mandatory; a non-HTTPS URL is refused.
    """
    if not url.lower().startswith("https://"):
        _warn("server URL must use https; skipping request")
        return None, None
    body = json.dumps(payload).encode("utf-8")
    request = urllib.request.Request(
        url, data=body, method="POST",
        headers={"Content-Type": "application/json"})
    context = ssl.create_default_context()
    try:
        with urllib.request.urlopen(request, timeout=_TIMEOUT,
                                    context=context) as response:
            status = response.getcode()
            raw = response.read()
    except urllib.error.HTTPError as error:
        return error.code, None
    except (urllib.error.URLError, socket.timeout, OSError, ValueError):
        return None, None
    try:
        parsed = json.loads(raw.decode("utf-8"))
    except (ValueError, UnicodeDecodeError):
        return status, None
    return status, parsed if isinstance(parsed, dict) else None


def _payload(token, plugin_version):
    payload = {}
    if token is not None:
        payload["token"] = token
    if plugin_version:
        payload["plugin_version"] = plugin_version
    return payload


# --- Free-install check-in (throttled, opt-out) -------------------------------

def _free_marker_path():
    directory = _cache_dir()
    return os.path.join(directory, _FREE_MARKER) if directory else None


def _free_install_due(now):
    """True if no token is present and the throttle window has elapsed.

    Without a cache directory there is nowhere to throttle, so we decline rather
    than risk a check-in on every build.
    """
    path = _free_marker_path()
    if not path:
        return False
    try:
        with open(path, "r", encoding="utf-8") as handle:
            pinged_at = float(json.load(handle).get("pinged_at", 0))
    except (OSError, ValueError, TypeError):
        pinged_at = 0.0
    return (now - pinged_at) >= _checkin_interval()


def _free_install_checkin(now, plugin_version):
    """Send the throttled, opt-out, best-effort tokenless check-in."""
    if _env_truthy(os.environ.get(_ENV_DISABLE)):
        return
    if not _free_install_due(now):
        return
    _post_json(_server_url(), _payload(None, plugin_version))
    # Record the attempt regardless of outcome so a down server cannot turn the
    # throttle off and cause a check-in on every build.
    path = _free_marker_path()
    if path:
        _atomic_write_json(path, {"pinged_at": now})


# --- Public entry point -------------------------------------------------------

def resolve_custom_header(token, plugin_version=""):
    """Return a sanitized custom-header string, or None to use the default.

    Never raises. Flow:
      * no token            -> throttled free-install check-in, return None
      * within the check-in
        interval             -> return the cached header without a network call
      * otherwise            -> best-effort check-in; on success use/refresh the
                                header, on failure fall back to the cached header
                                within its offline-grace window, else None
    """
    try:
        return _resolve(token, plugin_version)
    except Exception:  # pragma: no cover - defensive: never break a build
        return None


def check_token(token, url=None, plugin_version=""):
    """Probe the server for ``token`` and return ``(status, parsed_dict_or_None)``.

    A one-shot POST used by the configuration command to verify a token. The
    HTTPS guard and 5s timeout of ``_post_json`` apply. ``status`` is None when the
    request did not complete (timeout/DNS/TLS). Never raises.
    """
    try:
        return _post_json(url or _server_url(), _payload(token, plugin_version))
    except Exception:  # pragma: no cover - defensive
        return None, None


# CLI color helpers (best-effort ANSI; harmless when the terminal ignores them).
_C_OK = "\033[92m"
_C_WARN = "\033[93m"
_C_ERR = "\033[91m"
_C_END = "\033[0m"


def _describe_check(status, data):
    """Map a check_token result to ``(color, message)`` for display."""
    if status == 200 and isinstance(data, dict) and isinstance(data.get("header"), str):
        size = len(data["header"].encode("utf-8"))
        return _C_OK, "License active - custom header retrieved (%d bytes)." % size
    if status == 200:
        return _C_WARN, "Server returned 200 but no usable header; build uses the default."
    if status == 204:
        return _C_WARN, ("Server reachable, but this token returned no header (unknown or "
                         "inactive token). Builds still work with the default header.")
    if status == 400:
        return _C_ERR, "Server rejected the request as malformed."
    if status is None:
        return _C_WARN, ("Could not reach the server (network/TLS/timeout). Settings saved; "
                         "builds still work and will retry later.")
    return _C_WARN, "Unexpected server response (status %s)." % status


def configure(token=None, server_url=None, check_only=False, plugin_version=""):
    """Save token/url to the user config and probe the server. Returns an exit code.

    Shared by ``embeddedproto --set-token`` and ``install.py``. With ``check_only``
    nothing is written and the already-configured token/url is probed. A malformed
    token or a non-https URL is a hard error (returns 1, nothing written). A failed
    connectivity check is reported but returns 0 - a build is never blocked by it.
    """
    if token is not None:
        token = token.strip()
        if not TOKEN_RE.match(token):
            sys.stdout.write(_C_ERR + "Invalid token format; nothing was saved." + _C_END + "\n")
            return 1
    if server_url is not None:
        server_url = server_url.strip()
        if not server_url.lower().startswith("https://"):
            sys.stdout.write(_C_ERR + "Server URL must start with https://; nothing was saved."
                             + _C_END + "\n")
            return 1

    if not check_only and (token is not None or server_url is not None):
        try:
            path = config.set_values(token=token, server_url=server_url)
            sys.stdout.write("Saved license settings to " + path + "\n")
        except (OSError, RuntimeError) as error:
            sys.stdout.write(_C_ERR + "Could not write config: " + str(error) + _C_END + "\n")
            return 1

    probe_token = token if token is not None else resolve_token()
    if not probe_token:
        sys.stdout.write(_C_WARN + "No token configured; skipping server check." + _C_END + "\n")
        return 0

    status, data = check_token(probe_token, url=server_url, plugin_version=plugin_version)
    color, message = _describe_check(status, data)
    sys.stdout.write(color + message + _C_END + "\n")
    return 0


def _resolve(token, plugin_version):
    now = time.time()

    # Zero-touch first run: drop a commented default config so the user can find
    # and edit it. Skipped when a token is supplied via the environment (e.g. CI),
    # and a no-op when a config already exists. Best-effort; never blocks a build.
    if not os.environ.get(_ENV_TOKEN, "").strip():
        config.ensure_default()

    if token is None:
        _free_install_checkin(now, plugin_version)
        return None

    cache_path = _cache_file(token)
    cached = _read_cache(cache_path) if cache_path else None

    # Throttle the check-in (one network call + one usage signal) by the interval,
    # gated on the last *attempt* (recorded regardless of outcome). This collapses
    # rapid repeats within a single build and stops a down server from stalling
    # every build. The cached header keeps serving in the meantime.
    if cached is not None and (now - cached["last_attempt_at"]) < _checkin_interval():
        return _usable_header(cached, now)

    status, data = _post_json(_server_url(), _payload(token, plugin_version))
    if status == 200 and data is not None:
        header = _sanitize_header(data.get("header"))
        if header is not None:
            grace = _clamp_grace(data.get("ttl_seconds", _DEFAULT_GRACE))
            _write_cache(cache_path, header, now, now + grace, now)
            return header

    # Check-in failed or served no header. Record the attempt (so the next build is
    # throttled even against a down server) and keep serving the cached header
    # within its offline-grace window; a header-less marker throttles the no-cache
    # case the same way.
    if cached is not None:
        _write_cache(cache_path, cached["header"], cached["fetched_at"],
                     cached["expires_at"], now)
        return _usable_header(cached, now)

    _write_cache(cache_path, None, 0, 0, now)
    return None
