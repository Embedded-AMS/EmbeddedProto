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

"""Read the external field options file(s).

Embedded Proto's per field options (``maxLength``, ``nestedMaxLength``,
``customStorage`` and ``callbackStorage``) are normally written inline in the
``.proto``. That file is a shared, cross language contract though, while these
options are specific to one target. This module reads the same options from an
external JSON file instead, so a field can be sized without touching a schema
you do not own.

The file is keyed by scope, mirroring the proto: package parts, message names,
nested message names and finally the field name. A scope may also be written as a
dotted path, ``"foo.telemetry"`` says the same as ``"foo": {"telemetry": ...}``.
Options live only in the object belonging to a field, nothing is inherited::

    {
      "SensorPackage": {
        "SensorFrame": {
          "samples": { "maxLength": 128 }
        }
      }
    }

Because the file is keyed by package and message it has no relation to the path
of any ``.proto``. One file can hold the options for every schema in a build and
may live anywhere, which is what makes configuring a third party schema possible.

JSON keeps the whole thing in the standard library, like ``configparser`` does
for ``config.py``. It also guarantees that keys are strings; in YAML the bare
keys ``on``, ``no`` and ``off`` are booleans, which would collide with the legal
field names of the same spelling.
"""

import json
import os
import sys

from EmbeddedProto import embedded_proto_options_pb2

# The options that may appear in an options file and the JSON type each accepts.
# These are the fields of the Options message in embedded_proto_options.proto.
UNSIGNED = "unsigned"
BOOLEAN = "boolean"
OPTION_TYPES = {"maxLength": UNSIGNED,
                "nestedMaxLength": UNSIGNED,
                "customStorage": BOOLEAN,
                "callbackStorage": BOOLEAN,
                "keyMaxLength": UNSIGNED,
                "valueMaxLength": UNSIGNED}


def warn(message):
    """Write a warning to stderr. Protoc passes plugin stderr on to the user."""
    sys.stderr.write("EmbeddedProto options file: " + message + "\n")


# Format one scope path for use in an error message.
def path_str(path):
    return ".".join(path) if path else "<root>"


# Check the value of a single option and return it. Booleans are a subclass of int in Python, so an unsigned option
# must reject them explicitly or "maxLength": true would silently size a buffer to one.
def check_option_value(name, value, path, source):
    location = source + ": " + path_str(path) + "." + name + ": "
    if UNSIGNED == OPTION_TYPES[name]:
        if isinstance(value, bool) or not isinstance(value, int):
            raise Exception(location + "expected a whole number, got " + json.dumps(value) + ".")
        if 0 > value:
            raise Exception(location + "expected a positive number, got " + json.dumps(value) + ".")
    elif not isinstance(value, bool):
        raise Exception(location + "expected true or false, got " + json.dumps(value) + ".")
    return value


# Walk one parsed file and check everything that can be checked without the proto definitions: an object holds
# either options or names, never both, and every option value has the right type. Whether a name exists in the schema,
# and whether an option sits on a field rather than on a message, needs the descriptors and is checked in
# validate_against().
def check_structure(node, path, source):
    if not isinstance(node, dict):
        raise Exception(source + ": " + path_str(path) + ": expected an object, got "
                        + json.dumps(node) + ".")

    # The value tells an option from a name: an option is a number or a flag, a message or field is an object. Going
    # by the value rather than by the key is what lets a field be named after an option, "maxLength" included.
    name_keys = [key for key, value in node.items() if isinstance(value, dict)]
    option_keys = [key for key in node if key not in name_keys]

    # An object either describes one field, holding its options, or it is a scope holding messages and fields.
    if name_keys and option_keys:
        raise Exception(source + ": " + path_str(path) + ": mixes the option(s) "
                        + ", ".join(sorted(option_keys)) + " with the name(s) "
                        + ", ".join(sorted(name_keys)) + ". Options belong to a field, "
                        "a message or package holds no options of its own.")

    for name in option_keys:
        if name not in OPTION_TYPES:
            raise Exception(source + ": " + path_str(path) + ": unknown option " + name + ". Known options are "
                            + ", ".join(sorted(OPTION_TYPES)) + ".")
        check_option_value(name, node[name], path, source)

    for name in name_keys:
        check_structure(node[name], path + [name], source)


# Merge the tree of one file into the tree read so far. Values from the later file win, per option, so a project can
# keep a shared base file and a board specific overlay.
def merge_tree(target, addition):
    for key, value in addition.items():
        if isinstance(value, dict) and isinstance(target.get(key), dict):
            merge_tree(target[key], value)
        else:
            target[key] = value
    return target


# Expand every key holding a dotted path into nested objects. A dot can only mean scope nesting: protoc allows no
# dot in a package part, a message name or a field name, so the two spellings mean the same thing. A file may mix
# them, "foo.telemetry" and "foo": { "telemetry": ... } end up in one and the same subtree.
def expand_dotted_keys(node, path, source):
    if not isinstance(node, dict):
        return node

    result = {}
    for key, value in node.items():
        parts = key.split(".")
        if any(not part for part in parts):
            raise Exception(source + ": " + path_str(path + [key])
                            + ": a dotted path may not have an empty part.")

        # Rebuild the key as nested objects, innermost first.
        branch = expand_dotted_keys(value, path + [key], source)
        for part in reversed(parts):
            branch = {part: branch}
        merge_tree(result, branch)

    return result


class OptionsFile:
    """The merged content of zero or more field options files."""

    def __init__(self, tree=None, sources=None):
        # The scope tree as read from the file(s), keys mirroring the proto.
        self.tree = tree if tree is not None else {}
        # The paths the tree was read from, in the order given, for error messages.
        self.sources = list(sources) if sources else []

    # An options file that holds nothing behaves exactly like no options file at all.
    def __bool__(self):
        return bool(self.tree)

    def describe_sources(self):
        return ", ".join(self.sources) if self.sources else "<none>"

    def resolve(self, scope_path, field_name):
        """Return ``{option: value}`` for one field, empty when nothing is set.

        The path is walked with names taken from the proto definitions, never
        guessed from their spelling, so a field named ``maxLength`` resolves like
        any other.
        """
        node = self.tree
        for name in list(scope_path) + [field_name]:
            if not isinstance(node, dict) or name not in node:
                return {}
            node = node[name]
        if not isinstance(node, dict):
            return {}
        return {key: value for key, value in node.items() if key in OPTION_TYPES}

    def to_options(self, scope_path, field_name):
        """The resolved options of one field as an Options message, or None."""
        values = self.resolve(scope_path, field_name)
        if not values:
            return None
        options = embedded_proto_options_pb2.Options()
        for name, value in values.items():
            setattr(options, name, value)
        return options

    def validate_against(self, schema):
        """Report entries that match nothing in the schema of this build.

        A misspelled field name silently sizing nothing is exactly the wrong
        size build this feature must not cause, so it is an error. One file may
        however hold the options for schemas that this particular protoc run does
        not compile. Entries below a top level name that is absent from the run
        are therefore skipped, while everything under a name that is present is
        checked in full.
        """
        for name, node in self.tree.items():
            if name in schema:
                self.validate_node(node, schema[name], [name])

    # Walk one subtree of the file next to the matching part of the schema. The schema maps a message name to its own
    # children and a field name to None, mirroring the proto definitions.
    def validate_node(self, node, schema_node, path):
        source = self.describe_sources()

        # A field: the object below it may hold options and nothing else.
        if schema_node is None:
            unknown = sorted(key for key in node if key not in OPTION_TYPES)
            if unknown:
                raise Exception(source + ": " + path_str(path) + ": unknown option(s) "
                                + ", ".join(unknown) + ". Known options are "
                                + ", ".join(sorted(OPTION_TYPES)) + ".")
            return

        # A message or package: it holds messages and fields, never options.
        for key, value in node.items():
            if key not in schema_node:
                if key in OPTION_TYPES:
                    raise Exception(source + ": " + path_str(path) + ": the option " + key
                                    + " is set on a message or package. Options are set per "
                                    "field, in the object of the field itself.")
                raise Exception(source + ": " + path_str(path + [key])
                                + " does not exist in the proto definitions.")
            self.validate_node(value, schema_node[key], path + [key])


def load(paths):
    """Read and merge the given options files into one OptionsFile.

    Every path was named explicitly by the user, so a missing or unreadable file
    is an error: silently continuing would build the wrong buffer sizes.
    """
    tree = {}
    sources = []
    for path in paths:
        if not os.path.isfile(path):
            raise Exception("The options file " + path + " does not exist.")
        try:
            with open(path, "r", encoding="utf-8") as handle:
                content = json.load(handle)
        except json.JSONDecodeError as error:
            raise Exception(path + ":" + str(error.lineno) + ":" + str(error.colno) + ": "
                            + error.msg + ".")
        except OSError as error:
            raise Exception("The options file " + path + " could not be read: "
                            + error.strerror + ".")

        content = expand_dotted_keys(content, [], path)
        check_structure(content, [], path)
        merge_tree(tree, content)
        sources.append(path)

    return OptionsFile(tree, sources)


def schema_from_descriptors(proto_files):
    """Build the scope tree of the proto definitions in this protoc run.

    A message maps to its own children, a field maps to None. The result mirrors
    the shape of an options file, so the two can be walked side by side.
    """
    schema = {}
    for proto_file in proto_files:
        node = schema
        for part in proto_file.package.split(".") if proto_file.package else []:
            node = node.setdefault(part, {})
        for message in proto_file.message_type:
            add_message_to_schema(message, node)
    return schema


# Add one message and everything nested in it to the schema tree.
def add_message_to_schema(message, node):
    entry = node.setdefault(message.name, {})
    for field in message.field:
        entry[field.name] = None
    for nested in message.nested_type:
        # Map entries are synthesised by protoc and hold no user options.
        if not nested.options.map_entry:
            add_message_to_schema(nested, entry)
