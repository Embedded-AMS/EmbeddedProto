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

# Unit tests for EmbeddedProto/field_options.py, the reader of the external field options file. These cover the file
# format itself: what is accepted, what is rejected and how an entry resolves to a field. Driving the generator with
# an options file is covered in test_options_file_generator.py.

import json
import os
import sys
import tempfile
import unittest

_REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
if _REPO_ROOT not in sys.path:
    sys.path.insert(0, _REPO_ROOT)

from EmbeddedProto import field_options  # noqa: E402


class _Base(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()

    def tearDown(self):
        self._tmp.cleanup()

    # Write an options file and return its path. The content is either an object to encode or literal text, the
    # latter to write something that is not valid JSON.
    def write(self, content, name="board.options.json"):
        path = os.path.join(self._tmp.name, name)
        with open(path, "w", encoding="utf-8") as handle:
            handle.write(content if isinstance(content, str) else json.dumps(content))
        return path


class Resolve(_Base):

    def test_field_in_a_package_resolves(self):
        path = self.write({"pkg": {"Msg": {"field": {"maxLength": 12}}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 12}, options.resolve(["pkg", "Msg"], "field"))

    def test_nested_message_resolves_at_its_nested_position(self):
        path = self.write({"pkg": {"Outer": {"Inner": {"field": {"maxLength": 3}}}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 3}, options.resolve(["pkg", "Outer", "Inner"], "field"))
        # The same field name directly in the outer message is a different entry.
        self.assertEqual({}, options.resolve(["pkg", "Outer"], "field"))

    def test_dotted_package_nests_per_part(self):
        path = self.write({"foo": {"bar": {"Msg": {"field": {"maxLength": 5}}}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 5}, options.resolve(["foo", "bar", "Msg"], "field"))

    def test_a_package_may_be_written_as_a_dotted_path(self):
        # A dot can only mean scope nesting, no package part, message or field name may contain one, so both
        # spellings say the same thing.
        path = self.write({"foo.bar": {"Msg": {"field": {"maxLength": 5}}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 5}, options.resolve(["foo", "bar", "Msg"], "field"))

    def test_a_message_may_be_written_as_a_dotted_path(self):
        path = self.write({"pkg": {"Outer.Inner": {"field": {"maxLength": 6}}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 6}, options.resolve(["pkg", "Outer", "Inner"], "field"))

    def test_the_whole_path_may_be_written_dotted(self):
        path = self.write({"pkg.Msg.field": {"maxLength": 9}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 9}, options.resolve(["pkg", "Msg"], "field"))

    def test_both_spellings_merge_into_one_scope(self):
        path = self.write({"foo.bar": {"Msg": {"a": {"maxLength": 1}}},
                           "foo": {"bar": {"Msg": {"b": {"maxLength": 2}}}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 1}, options.resolve(["foo", "bar", "Msg"], "a"))
        self.assertEqual({"maxLength": 2}, options.resolve(["foo", "bar", "Msg"], "b"))

    def test_a_field_without_an_entry_resolves_to_nothing(self):
        path = self.write({"pkg": {"Msg": {"field": {"maxLength": 12}}}})
        options = field_options.load([path])
        self.assertEqual({}, options.resolve(["pkg", "Msg"], "other"))
        self.assertEqual({}, options.resolve(["pkg", "Other"], "field"))

    def test_all_four_options_round_trip(self):
        path = self.write({"Msg": {"a": {"maxLength": 8, "nestedMaxLength": 4},
                                   "b": {"customStorage": True},
                                   "c": {"callbackStorage": True}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 8, "nestedMaxLength": 4}, options.resolve(["Msg"], "a"))
        self.assertEqual({"customStorage": True}, options.resolve(["Msg"], "b"))
        self.assertEqual({"callbackStorage": True}, options.resolve(["Msg"], "c"))

    def test_resolved_options_become_an_options_message(self):
        path = self.write({"Msg": {"a": {"maxLength": 8, "callbackStorage": True}}})
        options = field_options.load([path]).to_options(["Msg"], "a")
        self.assertEqual(8, options.maxLength)
        self.assertTrue(options.callbackStorage)
        self.assertFalse(options.customStorage)

    def test_a_field_without_an_entry_has_no_options_message(self):
        path = self.write({"Msg": {"a": {"maxLength": 8}}})
        self.assertIsNone(field_options.load([path]).to_options(["Msg"], "b"))

    def test_a_field_named_like_a_yaml_boolean_is_an_ordinary_key(self):
        # JSON keys are always strings. In YAML 1.1 the bare keys on, no and true are booleans, which would collide
        # with these perfectly legal field names. This is why the file is JSON.
        path = self.write({"Msg": {"on": {"maxLength": 1}, "no": {"maxLength": 2}, "true": {"maxLength": 3}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 1}, options.resolve(["Msg"], "on"))
        self.assertEqual({"maxLength": 2}, options.resolve(["Msg"], "no"))
        self.assertEqual({"maxLength": 3}, options.resolve(["Msg"], "true"))

    def test_a_field_named_after_an_option_resolves_normally(self):
        # Names are taken from the proto definitions, never guessed from their spelling, so a field named maxLength
        # is addressed like any other field.
        path = self.write({"Msg": {"maxLength": {"maxLength": 7}}})
        options = field_options.load([path])
        self.assertEqual({"maxLength": 7}, options.resolve(["Msg"], "maxLength"))

    def test_no_options_file_resolves_to_nothing(self):
        options = field_options.load([])
        self.assertFalse(options)
        self.assertEqual({}, options.resolve(["Msg"], "field"))


class MultipleFiles(_Base):

    def test_the_later_file_wins_on_the_same_option(self):
        base = self.write({"Msg": {"field": {"maxLength": 10}}}, "base.options.json")
        board = self.write({"Msg": {"field": {"maxLength": 20}}}, "board.options.json")
        options = field_options.load([base, board])
        self.assertEqual({"maxLength": 20}, options.resolve(["Msg"], "field"))

    def test_files_merge_where_they_do_not_overlap(self):
        base = self.write({"Msg": {"a": {"maxLength": 10}}}, "base.options.json")
        board = self.write({"Msg": {"b": {"maxLength": 20}}}, "board.options.json")
        options = field_options.load([base, board])
        self.assertEqual({"maxLength": 10}, options.resolve(["Msg"], "a"))
        self.assertEqual({"maxLength": 20}, options.resolve(["Msg"], "b"))

    def test_an_option_of_the_same_field_is_kept_when_the_other_is_overridden(self):
        base = self.write({"Msg": {"a": {"maxLength": 10, "nestedMaxLength": 4}}}, "base.options.json")
        board = self.write({"Msg": {"a": {"maxLength": 20}}}, "board.options.json")
        options = field_options.load([base, board])
        self.assertEqual({"maxLength": 20, "nestedMaxLength": 4}, options.resolve(["Msg"], "a"))


class Rejected(_Base):

    def test_a_missing_file_is_an_error(self):
        missing = os.path.join(self._tmp.name, "absent.options.json")
        with self.assertRaises(Exception) as context:
            field_options.load([missing])
        self.assertIn(missing, str(context.exception))

    def test_malformed_json_names_the_position(self):
        path = self.write('{"Msg": {"field": {"maxLength": 4,}}}')
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        message = str(context.exception)
        self.assertIn(path, message)
        # The line and column of the offending character, not a traceback.
        self.assertIn(":1:", message)

    def test_a_root_that_is_not_an_object_is_an_error(self):
        path = self.write('[1, 2]')
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("expected an object", str(context.exception))

    def test_a_name_given_a_number_reads_as_an_unknown_option(self):
        # A number is an option value and an object is a name, so this is read as the option "field" on Msg. Saying
        # so is more use than reporting the shape, it names the key the user has to look at either way.
        path = self.write({"Msg": {"field": 12}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("unknown option field", str(context.exception))

    def test_mixing_options_and_names_in_one_object_is_an_error(self):
        path = self.write({"Msg": {"maxLength": 4, "field": {"maxLength": 8}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        message = str(context.exception)
        self.assertIn("Msg", message)
        self.assertIn("maxLength", message)
        self.assertIn("field", message)

    def test_a_text_value_for_a_length_is_an_error(self):
        path = self.write({"Msg": {"field": {"maxLength": "12"}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("whole number", str(context.exception))

    def test_a_boolean_for_a_length_is_an_error(self):
        # A bool is an int in Python, so this has to be rejected on purpose or it would size a buffer to one.
        path = self.write({"Msg": {"field": {"maxLength": True}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("whole number", str(context.exception))

    def test_a_negative_length_is_an_error(self):
        path = self.write({"Msg": {"field": {"maxLength": -1}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("positive", str(context.exception))

    def test_an_empty_part_in_a_dotted_path_is_an_error(self):
        path = self.write({"foo..bar": {"Msg": {"field": {"maxLength": 4}}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("empty part", str(context.exception))

    def test_a_trailing_dot_is_an_error(self):
        path = self.write({"foo.": {"Msg": {"field": {"maxLength": 4}}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("empty part", str(context.exception))

    def test_a_number_for_a_flag_is_an_error(self):
        path = self.write({"Msg": {"field": {"customStorage": 1}}})
        with self.assertRaises(Exception) as context:
            field_options.load([path])
        self.assertIn("true or false", str(context.exception))


class ValidateAgainstSchema(_Base):
    # The schema mirrors the proto definitions of one protoc run: a message maps to its children, a field to None.
    SCHEMA = {"pkg": {"Msg": {"field": None, "Inner": {"nested": None}}}}

    def validate(self, content):
        field_options.load([self.write(content)]).validate_against(self.SCHEMA)

    def test_a_matching_file_passes(self):
        self.validate({"pkg": {"Msg": {"field": {"maxLength": 4},
                                       "Inner": {"nested": {"maxLength": 8}}}}})

    def test_a_misspelled_field_is_an_error(self):
        with self.assertRaises(Exception) as context:
            self.validate({"pkg": {"Msg": {"feild": {"maxLength": 4}}}})
        self.assertIn("pkg.Msg.feild", str(context.exception))

    def test_a_misspelled_message_is_an_error(self):
        with self.assertRaises(Exception) as context:
            self.validate({"pkg": {"Msgg": {"field": {"maxLength": 4}}}})
        self.assertIn("pkg.Msgg", str(context.exception))

    def test_an_unknown_option_on_a_field_is_an_error(self):
        with self.assertRaises(Exception) as context:
            self.validate({"pkg": {"Msg": {"field": {"maxLenght": 4}}}})
        self.assertIn("maxLenght", str(context.exception))

    def test_an_option_on_a_message_is_an_error(self):
        # Rejected today, which is what keeps the format free to grow scope level defaults later without changing
        # the meaning of any file that is valid now.
        with self.assertRaises(Exception) as context:
            self.validate({"pkg": {"Msg": {"maxLength": 4, "field": {"maxLength": 8}}}})
        self.assertIn("Msg", str(context.exception))

    def test_a_schema_absent_from_this_run_is_skipped(self):
        # One options file may hold the options of every schema in a product while a single protoc run compiles only
        # a part of it. Entries for a package that is not in the run at all are not typos.
        self.validate({"other": {"Message": {"field": {"maxLength": 4}}}})


if __name__ == "__main__":
    unittest.main()
