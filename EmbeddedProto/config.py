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

"""Read and write the EmbeddedProto user config file.

A single per-user INI file holds non-secret defaults for the license plugin:
the optional build token and the license server URL. It lives at
``${XDG_CONFIG_HOME:-~/.config}/embeddedproto/config.ini`` (POSIX) or
``%APPDATA%\\embeddedproto\\config.ini`` (Windows).

The file is optional in every sense: a missing file means "use the built-in
defaults", and an exported ``EMBEDDEDPROTO_*`` environment variable always wins
over the file (see ``custom_header.py``). INI is used so the whole thing stays
in the standard library (``configparser``) with no extra dependency; TOML has no
standard-library writer and ``tomllib`` only reads on Python 3.11+.
"""

import configparser
import os
import sys

# The default license server. Defined here (the lowest-level module) so both
# the config template and custom_header.py can reference one source of truth.
DEFAULT_SERVER_URL = "https://license.embeddedproto.com/v1/header"

_SECTION = "license"

# The commented template written on first run / when (re)setting values. The two
# placeholders are filled with the current token and server URL. Keeping the file
# rendered from this template (rather than via configparser.write) preserves the
# explanatory comments across updates.
_TEMPLATE = """\
[{section}]
# Your build token. Leave empty to read EMBEDDEDPROTO_BUILD_TOKEN from the
# environment instead (recommended for CI / shared machines).
token = {token}

# License server endpoint (must be https://). Point at a staging server to test.
server_url = {server_url}
"""


def _warn(message):
    """Write a hygiene warning to stderr. Never includes the token value."""
    sys.stderr.write("EmbeddedProto license config: " + message + "\n")


def _config_dir():
    """Platform path to the per-user EmbeddedProto config directory."""
    if os.name == "nt":
        base = os.environ.get("APPDATA")
        return os.path.join(base, "embeddedproto") if base else None
    base = os.environ.get("XDG_CONFIG_HOME") or os.path.join(
        os.path.expanduser("~"), ".config")
    return os.path.join(base, "embeddedproto")


def config_path():
    """Full path to ``config.ini`` or None when no config dir can be resolved."""
    directory = _config_dir()
    return os.path.join(directory, "config.ini") if directory else None


def _check_owner_only(path):
    """True if ``path`` is safe to read a secret from (POSIX owner-only).

    On non-POSIX systems we cannot check reliably, so we allow the read.
    """
    if os.name != "posix":
        return True
    try:
        mode = os.stat(path).st_mode
    except OSError:
        return False
    if mode & 0o077:
        _warn("ignoring inline token in config (readable by group/other; "
              "tighten permissions to 0600): " + path)
        return False
    return True


def _render(token, server_url):
    return _TEMPLATE.format(section=_SECTION, token=token, server_url=server_url)


def _atomic_write(path, text):
    """Write ``text`` via a temp file + rename, owner-only on POSIX."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as handle:
        handle.write(text)
    if os.name == "posix":
        os.chmod(tmp, 0o600)
    os.replace(tmp, path)


def load(check_perms=True):
    """Return ``{"token": str|None, "server_url": str|None}`` from the config.

    A missing/unreadable file yields all-None. The ``server_url`` is non-secret
    and always returned; the inline ``token`` is only returned when the file
    passes the owner-only permission check (unless ``check_perms`` is False, used
    internally to preserve a value while rewriting the file).
    """
    result = {"token": None, "server_url": None}
    path = config_path()
    if not path or not os.path.exists(path):
        return result
    parser = configparser.ConfigParser(interpolation=None)
    try:
        parser.read(path, encoding="utf-8")
    except (configparser.Error, OSError, UnicodeDecodeError):
        return result
    if not parser.has_section(_SECTION):
        return result
    result["server_url"] = parser.get(_SECTION, "server_url", fallback="").strip() or None
    token = parser.get(_SECTION, "token", fallback="").strip()
    if token and (not check_perms or _check_owner_only(path)):
        result["token"] = token
    return result


def ensure_default():
    """Best-effort: write the commented default config if none exists.

    Used as a zero-touch first-run scaffold by the plugin. Never raises and never
    overwrites an existing file, so it cannot disturb a configured machine or fail
    a build on a read-only home.
    """
    path = config_path()
    if not path or os.path.exists(path):
        return
    try:
        _atomic_write(path, _render("", DEFAULT_SERVER_URL))
    except OSError:
        pass  # scaffolding is a convenience, never fatal


def set_values(token=None, server_url=None):
    """Update ``token`` and/or ``server_url`` in the config, preserving the other.

    Returns the path written. Raises ``RuntimeError`` if no config path can be
    resolved (so the caller can report it).
    """
    path = config_path()
    if not path:
        raise RuntimeError("cannot determine the EmbeddedProto config location")
    current = load(check_perms=False)
    new_token = token if token is not None else (current["token"] or "")
    new_url = (server_url if server_url is not None
               else (current["server_url"] or DEFAULT_SERVER_URL))
    _atomic_write(path, _render(new_token, new_url))
    return path
