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

# Protobuf editions feature-resolution layer.
#
# Protobuf editions (edition = "2023" / "2024") replace the proto2/proto3 syntax
# switch with per-element "features" that can be overridden at file -> message ->
# field / enum / oneof scope. The descriptor sent to a code generator carries only
# the *explicitly set* overrides; a non-C++ plugin must resolve the effective value
# itself by walking the scope chain and falling back to the edition's defaults.
#
# This module provides:
#   * EditionProfile: per-edition default value of each feature + which feature
#     fields are legal in that edition.
#   * FeatureResolver: picks the profile for a file and resolves a feature for any
#     scope (file / message / field) by merging explicit overrides over the
#     defaults.
#
# Resolution only happens here; consuming the resolved values (changing generated
# code) is done by the individual feature tickets. ED-1 only introduces the layer.

# -----------------------------------------------------------------------------
# Edition numbers (mirror google.protobuf.descriptor_pb2.Edition).

EDITION_PROTO2 = 998
EDITION_PROTO3 = 999
EDITION_2023 = 1000
EDITION_2024 = 1001


# -----------------------------------------------------------------------------
# Feature value constants (mirror the google.protobuf.FeatureSet enums). Using
# named constants keeps the consuming tickets readable instead of comparing magic
# numbers.

class FieldPresence:
    EXPLICIT = 1
    IMPLICIT = 2
    LEGACY_REQUIRED = 3


class EnumType:
    OPEN = 1
    CLOSED = 2


class RepeatedFieldEncoding:
    PACKED = 1
    EXPANDED = 2


class Utf8Validation:
    VERIFY = 2
    NONE = 3


class MessageEncoding:
    LENGTH_PREFIXED = 1
    DELIMITED = 2


class JsonFormat:
    ALLOW = 1
    LEGACY_BEST_EFFORT = 2


# The six core feature fields EmbeddedProto resolves. These are the fields on the
# google.protobuf.FeatureSet message that carry behavior for the wire format /
# code generation.
CORE_FEATURE_FIELDS = (
    "field_presence",
    "enum_type",
    "repeated_field_encoding",
    "utf8_validation",
    "message_encoding",
    "json_format",
)

# Edition 2024 added these feature fields. They are codegen / naming concerns with
# no runtime effect for EmbeddedProto, but they are only legal from edition 2024.
EDITION_2024_FEATURE_FIELDS = (
    "enforce_naming_style",
    "default_symbol_visibility",
)


# -----------------------------------------------------------------------------

class EditionProfile:
    """Holds, for a single edition, the default value of every core feature and
    the set of feature fields that may be explicitly set in that edition."""

    def __init__(self, edition, defaults, legal_features):
        self.edition = edition
        # A complete mapping of every core feature field to its default value for
        # this edition.
        self.defaults = dict(defaults)
        # The names of feature fields that are allowed to be explicitly set.
        self.legal_features = set(legal_features)


# The default value of each core feature per edition. Editions are cumulative:
# 2024 keeps the 2023 core defaults and only adds new (codegen-only) features.
_PROTO3_DEFAULTS = {
    "field_presence": FieldPresence.IMPLICIT,
    "enum_type": EnumType.OPEN,
    "repeated_field_encoding": RepeatedFieldEncoding.PACKED,
    "utf8_validation": Utf8Validation.VERIFY,
    "message_encoding": MessageEncoding.LENGTH_PREFIXED,
    "json_format": JsonFormat.ALLOW,
}

_EDITION_2023_DEFAULTS = {
    "field_presence": FieldPresence.EXPLICIT,
    "enum_type": EnumType.OPEN,
    "repeated_field_encoding": RepeatedFieldEncoding.PACKED,
    "utf8_validation": Utf8Validation.VERIFY,
    "message_encoding": MessageEncoding.LENGTH_PREFIXED,
    "json_format": JsonFormat.ALLOW,
}

# Edition 2024 inherits the 2023 core defaults unchanged.
_EDITION_2024_DEFAULTS = dict(_EDITION_2023_DEFAULTS)

PROFILES = {
    EDITION_PROTO3: EditionProfile(EDITION_PROTO3, _PROTO3_DEFAULTS, CORE_FEATURE_FIELDS),
    EDITION_2023: EditionProfile(EDITION_2023, _EDITION_2023_DEFAULTS, CORE_FEATURE_FIELDS),
    EDITION_2024: EditionProfile(
        EDITION_2024, _EDITION_2024_DEFAULTS,
        tuple(CORE_FEATURE_FIELDS) + EDITION_2024_FEATURE_FIELDS),
}


# -----------------------------------------------------------------------------

def detect_edition(file_descriptor):
    """Map a FileDescriptorProto onto an edition number understood by PROFILES.
    proto2 is rejected upstream; an unknown/absent syntax is treated as proto3."""
    syntax = file_descriptor.syntax
    if "editions" == syntax:
        return file_descriptor.edition
    # proto3 (syntax == "proto3") and the default empty syntax map to the proto3
    # profile so the existing generation path becomes a special case of the same
    # machinery.
    return EDITION_PROTO3


def _feature_set_of(options):
    """Return the FeatureSet sub-message of an *Options message, or None when the
    options object does not carry a features field (older descriptor types)."""
    if options is None:
        return None
    try:
        return options.features
    except AttributeError:
        return None


# -----------------------------------------------------------------------------

class FeatureResolver:
    """Resolves the effective value of each feature for a single proto file by
    merging the explicitly set overrides over the edition defaults."""

    def __init__(self, file_descriptor):
        self.edition = detect_edition(file_descriptor)
        if self.edition not in PROFILES:
            raise Exception("Unsupported protobuf edition: " + str(self.edition))
        self.profile = PROFILES[self.edition]

        # Verify the file itself does not set a feature that is illegal in this
        # edition, then compute the file-scope resolved feature set.
        self._check_legality(file_descriptor.options, "file " + file_descriptor.name)
        self.file_features = self._merge(self.profile.defaults, file_descriptor.options)

    def _check_legality(self, options, scope_description):
        """Raise when an options object explicitly sets a feature field that is not
        legal in this edition (e.g. a 2024-only feature used in a 2023 file)."""
        feature_set = _feature_set_of(options)
        if feature_set is None:
            return
        for field_descriptor, _value in feature_set.ListFields():
            name = field_descriptor.name
            if name not in self.profile.legal_features:
                raise Exception(
                    "The feature '" + name + "' set on " + scope_description +
                    " is not supported in edition " + str(self.edition) + ".")

    def _merge(self, base_features, options):
        """Return a new resolved feature dict: a copy of base_features with the
        explicit overrides from options applied on top."""
        result = dict(base_features)
        feature_set = _feature_set_of(options)
        if feature_set is not None:
            for name in CORE_FEATURE_FIELDS:
                if feature_set.HasField(name):
                    result[name] = getattr(feature_set, name)
        return result

    def merge(self, base_features, options, scope_description=""):
        """Public merge used to derive a child scope's resolved features from its
        parent's. Also enforces edition legality of the explicit overrides."""
        self._check_legality(options, scope_description)
        return self._merge(base_features, options)

    def resolve(self, *option_holders):
        """Resolve all core features from an ordered list of option holders given
        outermost-first (e.g. file, enclosing message(s), field). Each holder is an
        *Options message (or None). Returns a complete feature dict."""
        result = dict(self.profile.defaults)
        for options in option_holders:
            result = self._merge(result, options)
        return result

    # -- typed accessors on a resolved feature dict ---------------------------

    @staticmethod
    def field_presence(resolved):
        return resolved["field_presence"]

    @staticmethod
    def enum_type(resolved):
        return resolved["enum_type"]

    @staticmethod
    def repeated_encoding(resolved):
        return resolved["repeated_field_encoding"]

    @staticmethod
    def message_encoding(resolved):
        return resolved["message_encoding"]

    @staticmethod
    def utf8_validation(resolved):
        return resolved["utf8_validation"]
