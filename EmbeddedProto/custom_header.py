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

# --- Contract constants (see the server-side API contract) --------------------

DEFAULT_SERVER_URL = "https://license.embeddedproto.com/v1/header"
TOKEN_RE = re.compile(r"^[A-Za-z0-9._:-]{16,256}$")

_ENV_TOKEN = "EMBEDDEDPROTO_BUILD_TOKEN"
_ENV_URL = "EMBEDDEDPROTO_LICENSE_URL"
_ENV_FRESHNESS = "EMBEDDEDPROTO_LICENSE_FRESHNESS"
_ENV_DISABLE = "EMBEDDEDPROTO_LICENSE_DISABLE_CHECKIN"

_DEFAULT_FRESHNESS = 3600          # reuse cache younger than this without a call
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


def _config_token_path():
    """Platform path to the optional token file (see token discovery rules)."""
    if os.name == "nt":
        base = os.environ.get("APPDATA")
        return os.path.join(base, "embeddedproto", "token.txt") if base else None
    base = os.environ.get("XDG_CONFIG_HOME") or os.path.join(
        os.path.expanduser("~"), ".config")
    return os.path.join(base, "embeddedproto", "token.txt")


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


def _freshness():
    raw = os.environ.get(_ENV_FRESHNESS, "").strip()
    if not raw:
        return _DEFAULT_FRESHNESS
    try:
        return max(0, int(raw))
    except ValueError:
        return _DEFAULT_FRESHNESS


def _server_url():
    return os.environ.get(_ENV_URL, "").strip() or DEFAULT_SERVER_URL


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

def _read_token_file(path):
    """Return the token-file contents, rejecting world/group-accessible files."""
    try:
        if os.name == "posix":
            mode = os.stat(path).st_mode
            if mode & 0o077:
                _warn("ignoring token file (readable by group/other; "
                      "tighten permissions to 0600): " + path)
                return None
        with open(path, "r", encoding="utf-8") as handle:
            return handle.read().strip()
    except OSError:
        return None


def resolve_token():
    """Return a validated build token, or None when none is configured.

    Priority: the EMBEDDEDPROTO_BUILD_TOKEN environment variable, then the
    platform token file. A malformed value is treated as absent.
    """
    try:
        token = os.environ.get(_ENV_TOKEN, "").strip()
        if not token:
            path = _config_token_path()
            token = (_read_token_file(path) or "").strip() if path else ""
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
    """Return (header, fetched_at, expires_at) or None."""
    try:
        with open(path, "r", encoding="utf-8") as handle:
            data = json.load(handle)
        header = data.get("header")
        if isinstance(header, str):
            return header, float(data.get("fetched_at", 0)), float(
                data.get("expires_at", 0))
    except (OSError, ValueError, TypeError):
        pass
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
    return (now - pinged_at) >= _freshness()


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
      * no token       -> throttled free-install check-in, return None
      * fresh cache    -> return cached header without a network call
      * otherwise      -> best-effort refresh; on failure fall back to a stale
                          cached header within its offline-grace window, else None
    """
    try:
        return _resolve(token, plugin_version)
    except Exception:  # pragma: no cover - defensive: never break a build
        return None


def _resolve(token, plugin_version):
    now = time.time()

    if token is None:
        _free_install_checkin(now, plugin_version)
        return None

    cache_path = _cache_file(token)
    cached = _read_cache(cache_path) if cache_path else None

    if cached is not None:
        header, fetched_at, _expires_at = cached
        if (now - fetched_at) < _freshness():
            return header  # within freshness window: no network call

    status, data = _post_json(_server_url(), _payload(token, plugin_version))
    if status == 200 and data is not None:
        header = _sanitize_header(data.get("header"))
        if header is not None:
            grace = _clamp_grace(data.get("ttl_seconds", _DEFAULT_GRACE))
            if cache_path:
                _atomic_write_json(cache_path, {
                    "header": header,
                    "fetched_at": now,
                    "expires_at": now + grace,
                })
            return header

    # Refresh failed or served no header: reuse stale cache within offline grace.
    if cached is not None:
        header, _fetched_at, expires_at = cached
        if now < expires_at:
            return header

    return None
