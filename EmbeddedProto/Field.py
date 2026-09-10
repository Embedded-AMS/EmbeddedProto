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

from google.protobuf.descriptor_pb2 import FieldDescriptorProto
from . import embedded_proto_options_pb2
from . import field_options
from .Features import FieldPresence
import copy
import sys


# This class is the base class for any kind of field used in protobuf messages.
class Field:
    def __init__(self, proto_descriptor, parent_msg, template_filename, oneof=None):
        # A reference to the FieldDescriptorProto object which defines this field.
        self.descriptor = proto_descriptor

        # A reference to the parent message in which this field is defined.
        self.parent = parent_msg

        # Editions feature resolution. The parent message holds the resolver and
        # its own resolved feature set; this field merges its own explicit
        # overrides on top to obtain the resolved feature set for this field. The
        # resolved values are consumed by the individual feature tickets; ED-1 only
        # makes them available.
        self.feature_resolver = getattr(parent_msg, "feature_resolver", None)
        if self.feature_resolver is not None:
            self.resolved_features = self.feature_resolver.merge(
                parent_msg.features, self.descriptor.options, self.descriptor.name)
        else:
            self.resolved_features = None

        # Determine field presence from the resolved editions feature.
        #   EXPLICIT        -> track presence with a bit and generate has_*()
        #                      (reuse the existing proto3_optional infrastructure).
        #   IMPLICIT        -> proto3 behavior (serialize only when != default).
        #   LEGACY_REQUIRED -> always serialize, no presence bit, no absence check.
        # Repeated fields and real oneof members never carry a presence bit
        # (presence comes from the repetition / the oneof discriminator) so they
        # keep the proto3 behavior regardless of the resolved feature.
        self.always_serialize = False
        is_repeated = (FieldDescriptorProto.LABEL_REPEATED == self.descriptor.label)
        in_real_oneof = oneof is not None
        if (self.resolved_features is not None) and (not is_repeated) and (not in_real_oneof):
            presence = self.resolved_features["field_presence"]
            if FieldPresence.LEGACY_REQUIRED == presence:
                self.optional = self.descriptor.proto3_optional
                self.always_serialize = True
            else:
                self.optional = (FieldPresence.EXPLICIT == presence) or self.descriptor.proto3_optional
        else:
            self.optional = self.descriptor.proto3_optional

        # If this field is part of an oneof this is the reference to it.
        self.oneof = oneof

        self.name = self.descriptor.name
        self.variable_name = self.name + "_"
        self.variable_id_name = self.name.upper()
        self.variable_id = self.descriptor.number
        self.template_file = template_filename

        self.of_type_enum = FieldDescriptorProto.TYPE_ENUM == proto_descriptor.type

        # The Embedded Proto options of this field, written inline in the .proto, supplied by an external options
        # file, or both. Resolved once here so every option read below uses one and the same effective set.
        self.embedded_proto_options = self.resolve_embedded_proto_options()

        # Whether the user wants to supply the storage type for this field via a template parameter.
        self.custom_storage = False
        # Whether the field streams its elements through user callbacks (the callbackStorage option).
        self.callback_storage = False
        if self.embedded_proto_options is not None:
            self.custom_storage = self.embedded_proto_options.customStorage
            self.callback_storage = self.embedded_proto_options.callbackStorage

        # Remember whether this is a repeated field; it selects the callback storage kind
        # (repeated scalar/enum -> RepeatedFieldCallback, singular bytes/string -> BytesStringCallback).
        self.is_repeated = is_repeated

        # callbackStorage supports repeated scalar/enum fields and singular bytes/string fields. Reject
        # every other placement with a generator error so unsupported configurations never emit wrong code.
        if self.callback_storage:
            self.reject_unsupported_callback_storage(is_repeated, in_real_oneof)

    # Combine the options written inline in the .proto with those from an external options file. An entry in the
    # options file wins over the same option inline: the .proto is a shared contract and may not be yours to edit,
    # which is the reason the file exists. Returns None when neither source sets anything.
    def resolve_embedded_proto_options(self):
        inline = None
        if self.descriptor.options.HasExtension(embedded_proto_options_pb2.options):
            inline = self.descriptor.options.Extensions[embedded_proto_options_pb2.options]

        # The options file reaches a field through its parent message, the same way the feature resolver does.
        options_file = getattr(self.parent, "options_file", None)
        if not options_file:
            return inline

        # The scope of the parent message, the package parts and message names leading up to this field.
        scope = self.parent.scope.get_list_of_scope_str()
        from_file = options_file.resolve(scope, self.descriptor.name)
        if not from_file:
            return inline

        result = embedded_proto_options_pb2.Options()
        if inline is not None:
            result.CopyFrom(inline)

        for name, value in from_file.items():
            # The Options message has no field presence, an option still at its default was never set inline. Report
            # an override rather than let the .proto and the options file disagree silently.
            previous = getattr(result, name)
            if previous:
                field_options.warn(options_file.describe_sources() + ": " + ".".join(scope) + "."
                                   + self.descriptor.name + ": " + name + " = " + str(value)
                                   + " overrides " + str(previous) + " set in the proto file.")
            setattr(result, name, value)

        return result

    # Returns true when the user supplies the storage type for this field (the customStorage option).
    def has_custom_storage(self):
        return self.custom_storage

    # Returns true when the field streams its elements through user callbacks (the callbackStorage option).
    def has_callback_storage(self):
        return self.callback_storage

    # Returns true when a callbackStorage field serializes EXPANDED (one tag per element). This holds for
    # repeated scalar/enum callback storage; a singular bytes/string callback stays LEN framed instead.
    def callback_is_expanded(self):
        return False

    # Returns true when a callbackStorage field streams a repeated message (through MessageCallback).
    def callback_is_message(self):
        return False

    # Returns true when a callbackStorage field streams its elements length delimited instead of as
    # DELIMITED groups. Only a map does; its entries have to stay readable by a standard protoc peer.
    def callback_is_len_expanded(self):
        return False

    # Raise a generator error when callbackStorage is set on a field kind that is not yet supported.
    def reject_unsupported_callback_storage(self, is_repeated, in_real_oneof):
        # A map streams its entries length delimited instead of as DELIMITED groups, so the rules
        # below do not apply to it. This is checked on the descriptor rather than on the class
        # because a repeated field builds its element from that very same descriptor, which would
        # otherwise run these checks a second time for the entry.
        if Field.find_map_entry_descriptor(self.descriptor, self.parent) is not None:
            return

        location = self.parent.name + "." + self.descriptor.name
        is_message = FieldDescriptorProto.TYPE_MESSAGE == self.descriptor.type
        is_string_or_bytes = self.descriptor.type in (FieldDescriptorProto.TYPE_STRING,
                                                      FieldDescriptorProto.TYPE_BYTES)
        if in_real_oneof:
            raise Exception(location + ": callbackStorage cannot be used on oneof members.")
        elif is_repeated:
            if is_message:
                # Repeated message streaming uses MessageCallback, which frames each element
                # DELIMITED (a group carries no size prefix, so there is no size pre-pass). Require
                # the field to be DELIMITED-encoded; a LEN element would need the pre-pass a stream
                # cannot provide.
                from .Features import MessageEncoding
                is_delimited = (self.resolved_features is not None) and \
                    (MessageEncoding.DELIMITED == self.resolved_features["message_encoding"])
                if not is_delimited:
                    raise Exception(location + ": callbackStorage on a repeated message field requires "
                                    "features.message_encoding = DELIMITED.")
            elif is_string_or_bytes:
                # Repeated string/bytes streaming is not built yet.
                raise Exception(location + ": callbackStorage on repeated string or bytes fields is not "
                                "yet supported; only repeated scalar, enum and message fields are.")
        elif is_string_or_bytes:
            # Singular bytes/string streams through BytesStringCallback. Presence-on-bind for an
            # explicit-presence (optional) field is a later extension, so reject it for now.
            if self.optional:
                raise Exception(location + ": callbackStorage on an explicit-presence (optional) bytes "
                                "or string field is not yet supported; use an implicit-presence field.")
        elif is_message:
            raise Exception(location + ": callbackStorage on message fields is not yet supported.")
        else:
            raise Exception(location + ": callbackStorage on a singular scalar field is not supported; "
                            "use a repeated scalar/enum field or a bytes/string field.")

    # The name of the C++ template parameter exposing the user supplied storage type.
    def get_storage_type_param_str(self):
        return self.parent.name + "_" + self.variable_name + "STORAGE"

    # The C++ base class a customStorage type must derive from. Returns an empty string for field kinds that do not
    # support customStorage. Used to emit a static_assert in the generated code.
    def get_storage_base_type(self):
        return ""

    @staticmethod
    # Return the descriptor of the synthetic entry message when this field is a map, None otherwise.
    #
    # Protoc rewrites "map<K,V> foo = 1;" into a repeated message field whose element is a nested
    # message flagged map_entry, holding the key as field one and the value as field two. That entry
    # is always nested directly in the message declaring the map, so it can be found by name among
    # the raw nested types of the parent. The raw descriptors are used on purpose: this runs from
    # MessageDefinition.__init__ while the fields are built, before the type definitions are matched,
    # so the generated entry class can not be resolved through the usual type lookup yet.
    def find_map_entry_descriptor(proto_descriptor, parent_msg):
        result = None
        if (FieldDescriptorProto.LABEL_REPEATED == proto_descriptor.label) and \
                (FieldDescriptorProto.TYPE_MESSAGE == proto_descriptor.type):
            entry_name = proto_descriptor.type_name.rsplit(".", 1)[-1]
            for nested in parent_msg.descriptor.nested_type:
                if (nested.name == entry_name) and nested.options.map_entry:
                    result = nested
        return result

    @staticmethod
    # This function create the appropriate field object for a variable defined in the message.
    # The descriptor and parent message parameters are required parameters, all field need them to be created. The oneof
    # parameter is only required for fields which are part of a oneof. The parameter is the reference to the oneof
    # object.
    # The last parameter is not to be used manually. It is set when we are already in a FieldNested class.
    def factory(proto_descriptor, parent_msg, oneof=None, already_nested=False):

        # If this field has the same type as the parent we got a recursive inclusion. We can not solve the template
        # parameters in this case. This field is thus replaced by a dummy with a warning. Toposort is used to find more
        # complex recursive inclusions which we can not solve like this.
        parent_msg_type = "." + parent_msg.scope.get_scope_str().replace("::", ".")
        if proto_descriptor.type_name == parent_msg_type:
            result = FieldErrorRecursive(proto_descriptor, parent_msg, oneof)
        # Now continue constructing the normal fields.
        elif (FieldDescriptorProto.LABEL_REPEATED == proto_descriptor.label) and not already_nested:
            # A map is stored and encoded as a repeated message field of entries, so it has to be
            # recognised before the plain repeated field is constructed.
            if Field.find_map_entry_descriptor(proto_descriptor, parent_msg) is not None:
                result = FieldMap(proto_descriptor, parent_msg, oneof)
            else:
                result = FieldRepeated(proto_descriptor, parent_msg, oneof)
        elif FieldDescriptorProto.TYPE_MESSAGE == proto_descriptor.type:
            result = FieldMessage(proto_descriptor, parent_msg, oneof)
        elif FieldDescriptorProto.TYPE_ENUM == proto_descriptor.type:
            result = FieldEnum(proto_descriptor, parent_msg, oneof)
        elif FieldDescriptorProto.TYPE_STRING == proto_descriptor.type:
            result = FieldString(proto_descriptor, parent_msg, oneof)
        elif FieldDescriptorProto.TYPE_BYTES == proto_descriptor.type:
            result = FieldBytes(proto_descriptor, parent_msg, oneof)
        else:
            result = FieldBasic(proto_descriptor, parent_msg, oneof)
        return result

    def get_wire_type_str(self):
        return ""

    def get_type(self):
        return ""

    def get_short_type(self):
        return ""

    def get_default_value(self):
        return ""

    # Returns True when the field carries a custom (editions) default value, e.g.
    # int32 x = 1 [default = 42];. Custom defaults are only legal on fields with
    # explicit presence; protoc rejects them on implicit-presence fields.
    def has_default(self):
        return self.descriptor.HasField("default_value")

    def get_name(self):
        return self.name

    def get_variable_name(self):
        var_name = ""
        if self.oneof:
            var_name = self.oneof.get_variable_name() + "."
        var_name += self.variable_name
        return var_name

    def get_variable_id_name(self):
        return self.variable_id_name

    # Returns a list with a dictionaries for each template parameter this field had. The dictionary holds the parameter
    # name and its type.
    def get_template_parameters(self):
        # For the field that do not have any templates return an empty list.
        return []

    def match_field_with_definitions(self, all_types_definitions):
        pass

    def register_template_parameters(self):
        return True

    # Returns true if in oneof.init the new& function needs to be call to initialize already allocated memory.
    def oneof_allocation_required(self):
        return type(self) is not FieldEnum

    def get_oneof_name(self):
        return self.oneof.get_name()

    def get_which_oneof(self):
        return self.oneof.get_which_oneof()

    # Get the scope relevant compared to the scope this field is used in.
    def get_reduced_scope(self):
        parent_scope = self.parent.scope.get()
        def_scope = self.definition.scope.get()
        start_index = 0
        for ds, ps in zip(def_scope[:-1], parent_scope):
            if ds == ps:
                start_index += 1
            else:
                break
        reduced_scope = def_scope[start_index:]
        return reduced_scope

    def render(self, filename, jinja_environment):
        template = jinja_environment.get_template(filename)
        rendered_str = template.render(field=self, environment=jinja_environment)
        return rendered_str

    # Returns True if this field type uses serialize_len() instead of serialize_with_id()
    def uses_serialize_len(self):
        return False

    # Returns the C++ expression for the size parameter of serialize_len()
    # Only used when uses_serialize_len() returns True
    def get_size_expression(self):
        return ""

    # Returns True if this field is a nested message type (not string/bytes)
    def is_message_type(self):
        return False

    # Returns True when this message field uses DELIMITED (group) message encoding
    # (editions message_encoding feature). Only message fields can be delimited.
    def is_delimited(self):
        return False

    def render_serialize(self, jinja_env):
        return self.render("Field_Serialize.h.jinja2", jinja_environment=jinja_env)

    def render_serialize_partial(self, jinja_env):
        return self.render("Field_SerializePartial.h.jinja2", jinja_environment=jinja_env)

    def render_deserialize_partial(self, jinja_env):
        return self.render("Field_DeserializePartial.h.jinja2", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class is used to define any type of basic field.
class FieldBasic(Field):
    # A dictionary to convert the wire type into a default value.
    type_to_default_value = {FieldDescriptorProto.TYPE_DOUBLE:   "0.0",
                             FieldDescriptorProto.TYPE_FLOAT:    "0.0f",
                             FieldDescriptorProto.TYPE_INT64:    "0",
                             FieldDescriptorProto.TYPE_UINT64:   "0U",
                             FieldDescriptorProto.TYPE_INT32:    "0",
                             FieldDescriptorProto.TYPE_FIXED64:  "0U",
                             FieldDescriptorProto.TYPE_FIXED32:  "0U",
                             FieldDescriptorProto.TYPE_BOOL:     "false",
                             FieldDescriptorProto.TYPE_UINT32:   "0U",
                             FieldDescriptorProto.TYPE_SFIXED32: "0",
                             FieldDescriptorProto.TYPE_SFIXED64: "0",
                             FieldDescriptorProto.TYPE_SINT32:   "0",
                             FieldDescriptorProto.TYPE_SINT64:   "0"}

    # A dictionary to convert the protobuf wire type into an Embedded Proto C++ type.
    type_to_cpp_type = {FieldDescriptorProto.TYPE_DOUBLE:   "EmbeddedProto::doublefixed",
                        FieldDescriptorProto.TYPE_FLOAT:    "EmbeddedProto::floatfixed",
                        FieldDescriptorProto.TYPE_INT64:    "EmbeddedProto::int64",
                        FieldDescriptorProto.TYPE_UINT64:   "EmbeddedProto::uint64",
                        FieldDescriptorProto.TYPE_INT32:    "EmbeddedProto::int32",
                        FieldDescriptorProto.TYPE_FIXED64:  "EmbeddedProto::fixed64",
                        FieldDescriptorProto.TYPE_FIXED32:  "EmbeddedProto::fixed32",
                        FieldDescriptorProto.TYPE_BOOL:     "EmbeddedProto::boolean",
                        FieldDescriptorProto.TYPE_UINT32:   "EmbeddedProto::uint32",
                        FieldDescriptorProto.TYPE_SFIXED32: "EmbeddedProto::sfixed32",
                        FieldDescriptorProto.TYPE_SFIXED64: "EmbeddedProto::sfixed64",
                        FieldDescriptorProto.TYPE_SINT32:   "EmbeddedProto::sint32",
                        FieldDescriptorProto.TYPE_SINT64:   "EmbeddedProto::sint64"}

    # A dictionary to convert the protobuf wire type into a C++ type.
    type_to_cstdint = {FieldDescriptorProto.TYPE_DOUBLE:   "double",
                       FieldDescriptorProto.TYPE_FLOAT:    "float",
                       FieldDescriptorProto.TYPE_INT64:    "int64_t",
                       FieldDescriptorProto.TYPE_UINT64:   "uint64_t",
                       FieldDescriptorProto.TYPE_INT32:    "int32_t",
                       FieldDescriptorProto.TYPE_FIXED64:  "uint64_t",
                       FieldDescriptorProto.TYPE_FIXED32:  "uint32_t",
                       FieldDescriptorProto.TYPE_BOOL:     "bool",
                       FieldDescriptorProto.TYPE_UINT32:   "uint32_t",
                       FieldDescriptorProto.TYPE_SFIXED32: "int32_t",
                       FieldDescriptorProto.TYPE_SFIXED64: "int64_t",
                       FieldDescriptorProto.TYPE_SINT32:   "int32_t",
                       FieldDescriptorProto.TYPE_SINT64:   "int64_t"}

    # A dictionary to convert the wire type number into a wire type string.
    type_to_wire_type = {FieldDescriptorProto.TYPE_INT32:    "VARINT",
                         FieldDescriptorProto.TYPE_INT64:    "VARINT",
                         FieldDescriptorProto.TYPE_UINT32:   "VARINT",
                         FieldDescriptorProto.TYPE_UINT64:   "VARINT",
                         FieldDescriptorProto.TYPE_SINT32:   "VARINT",
                         FieldDescriptorProto.TYPE_SINT64:   "VARINT",
                         FieldDescriptorProto.TYPE_BOOL:     "VARINT",
                         FieldDescriptorProto.TYPE_FIXED64:  "FIXED64",
                         FieldDescriptorProto.TYPE_SFIXED64: "FIXED64",
                         FieldDescriptorProto.TYPE_DOUBLE:   "FIXED64",
                         FieldDescriptorProto.TYPE_FIXED32:  "FIXED32",
                         FieldDescriptorProto.TYPE_FLOAT:    "FIXED32",
                         FieldDescriptorProto.TYPE_SFIXED32: "FIXED32"}

    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldBasic.h.jinja2", oneof)

    def get_wire_type_str(self):
        return self.type_to_wire_type[self.descriptor.type]

    def get_type(self):
        return self.type_to_cpp_type[self.descriptor.type]

    def get_short_type(self):
        return self.get_type().split("::")[-1]

    def get_cstdint_type(self):
        return self.type_to_cstdint[self.descriptor.type]

    # A suffix to make a numeric literal the correct C++ type for the field's
    # storage type. Only used when emitting a custom (editions) default value; the
    # zero-default path keeps its historical formatting unchanged.
    type_to_literal_suffix = {FieldDescriptorProto.TYPE_INT64:    "LL",
                              FieldDescriptorProto.TYPE_SINT64:   "LL",
                              FieldDescriptorProto.TYPE_SFIXED64: "LL",
                              FieldDescriptorProto.TYPE_UINT64:   "ULL",
                              FieldDescriptorProto.TYPE_FIXED64:  "ULL",
                              FieldDescriptorProto.TYPE_UINT32:   "U",
                              FieldDescriptorProto.TYPE_FIXED32:  "U",
                              FieldDescriptorProto.TYPE_INT32:    "",
                              FieldDescriptorProto.TYPE_SINT32:   "",
                              FieldDescriptorProto.TYPE_SFIXED32: ""}

    def get_default_value(self):
        # Without a custom default keep the historical zero literal unchanged so
        # proto3 generation is byte-for-byte identical.
        if not self.has_default():
            return self.type_to_default_value[self.descriptor.type]
        return self._format_default_literal(self.descriptor.default_value)

    def _format_default_literal(self, value):
        field_type = self.descriptor.type
        if FieldDescriptorProto.TYPE_BOOL == field_type:
            return value  # protobuf gives "true" / "false"
        if field_type in (FieldDescriptorProto.TYPE_FLOAT, FieldDescriptorProto.TYPE_DOUBLE):
            return self._format_floating_literal(value, field_type)
        # Integer types: append the suffix matching the storage type.
        return value + self.type_to_literal_suffix[field_type]

    @staticmethod
    def _format_floating_literal(value, field_type):
        is_float = (FieldDescriptorProto.TYPE_FLOAT == field_type)
        cpp_type = "float" if is_float else "double"
        # protobuf encodes non-finite defaults as inf / -inf / nan.
        if "inf" == value:
            return "std::numeric_limits<" + cpp_type + ">::infinity()"
        if "-inf" == value:
            return "-std::numeric_limits<" + cpp_type + ">::infinity()"
        if "nan" == value:
            return "std::numeric_limits<" + cpp_type + ">::quiet_NaN()"
        # Ensure a valid C++ floating point literal (e.g. "2" -> "2.0").
        if not any(ch in value for ch in (".", "e", "E")):
            value += ".0"
        return value + "F" if is_float else value

    def render_get_set(self, jinja_env):
        return self.render("FieldBasic_GetSet.h.jinja2", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        str = self.render("FieldBasic_Deserialize.h.jinja2", jinja_environment=jinja_env)
        return str.rstrip()

# -----------------------------------------------------------------------------


# A base class for both the String and Bytes type field
class BaseStringBytes(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldString.h.jinja2", oneof)

        # Custom default values for string and bytes fields are not supported: the
        # fixed-size storage has no literal constructor to initialise from. Warn so
        # the silently-ignored default is not mistaken for working behavior.
        if self.has_default():
            print(parent_msg.name + "." + self.name + ": Warning: custom default values "
                  "for string and bytes fields are not supported and will be ignored.",
                  file=sys.stderr)

        # This is the name given to the template parameter for the length.
        self.template_param_str = self.parent.name + "_" + self.variable_name + "LENGTH"

        # Find options we know and use in this type of field.
        self.MaxLength = None

        if self.embedded_proto_options is not None:
            options = self.embedded_proto_options

            # Determine which maxLength to use based on context
            # If we're in a repeated field, use nestedMaxLength, if not present create a C++ template parameter.
            # If we're not in a repeated field, use maxLength, if not present create a C++ template parameter.
            # Check if this field is part of a repeated field by looking at the parent's descriptor
            is_in_repeated = proto_descriptor.label == FieldDescriptorProto.LABEL_REPEATED

            if is_in_repeated:
                if options.nestedMaxLength:
                    self.MaxLength = options.nestedMaxLength
                else:
                    self.MaxLength = None
            else:
                # For non-repeated fields, just use maxLength
                self.MaxLength = options.maxLength

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_template_parameters(self):
        result = []
        # A callbackStorage field is emitted as a concrete BytesStringCallback, so it exposes no
        # template parameter (neither a storage type nor a length).
        if self.has_callback_storage():
            pass
        # When the user supplies the storage type, expose a single plain type parameter without a default. The user
        # must supply a type derived from ::EmbeddedProto::internal::BaseStringBytes. The maximum length is ignored.
        elif self.has_custom_storage():
            result.append({"name": self.get_storage_type_param_str(), "type": "class"})

        # When no maximum length is specified, expose the length as a template parameter.
        elif not self.MaxLength:
            result.append({"name": self.template_param_str, "type": "uint32_t"})

        return result

    def register_template_parameters(self):
        # A callbackStorage field is a concrete type with no template parameter to register.
        if self.has_callback_storage():
            return True
        # A user supplied storage type contributes a storage type parameter; an unspecified maximum length contributes
        # a length parameter. In either case this field has a template parameter to register with the parent.
        if self.has_custom_storage() or not self.MaxLength:
            self.parent.register_child_with_template(self)
        return True

    def get_storage_base_type(self):
        return "::EmbeddedProto::internal::BaseStringBytes"

    def render_deserialize(self, jinja_env):
        str = self.render("FieldBasic_Deserialize.h.jinja2", jinja_environment=jinja_env)
        return str.rstrip()

    def uses_serialize_len(self):
        return True

    def get_size_expression(self):
        return self.get_variable_name() + ".get_length()"

# -----------------------------------------------------------------------------


# This class defines a string field
class FieldString(BaseStringBytes):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, oneof)

    def get_type(self):
        # A callbackStorage field streams through a concrete BytesStringCallback for char elements.
        if self.has_callback_storage():
            return "::EmbeddedProto::BytesStringCallback<char>"

        # When the user supplies the storage type, use the plain template parameter as the field type.
        if self.has_custom_storage():
            return self.get_storage_type_param_str()

        str_type = "::EmbeddedProto::FieldString<"
        if self.MaxLength:
            str_type += str(self.MaxLength) + ">"
        else:
            str_type += self.template_param_str + ">"
        return str_type

    def get_short_type(self):
        return "FieldString"

    def render_get_set(self, jinja_env):
        return self.render("FieldString_GetSet.h.jinja2", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class defines a bytes array field
class FieldBytes(BaseStringBytes):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, oneof)

    def get_type(self):
        # A callbackStorage field streams through a concrete BytesStringCallback for uint8_t elements.
        if self.has_callback_storage():
            return "::EmbeddedProto::BytesStringCallback<uint8_t>"

        # When the user supplies the storage type, use the plain template parameter as the field type.
        if self.has_custom_storage():
            return self.get_storage_type_param_str()

        str_type = "::EmbeddedProto::FieldBytes<"
        if self.MaxLength:
            str_type += str(self.MaxLength) + ">"
        else:
            str_type += self.template_param_str + ">"
        return str_type

    def get_short_type(self):
        return "FieldBytes"

    def render_get_set(self, jinja_env):
        return self.render("FieldBytes_GetSet.h.jinja2", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class is used to wrap around any enum used as a field.
class FieldEnum(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldEnum.h.jinja2", oneof)

        # Reserve a member variable for the reference to the enum definition used for this field.
        self.definition = None

    def get_wire_type_str(self):
        return "VARINT"

    def get_type_as_defined(self):
        if not self.definition:
            # When the actual definition is unknown use the protobuf type.
            type_name = self.descriptor.type_name if "." != self.descriptor.type_name[0] else self.descriptor.type_name[1:]
            type_name = type_name.replace(".", "::")
        else:
            scopes = self.get_reduced_scope()
            type_name = ""
            for scope in scopes:
                if scope["templates"]:
                    raise Exception("You are trying to use a field with the type: \"" + self.descriptor.type_name +
                                    "\". It is defined in different scope as where you are using it. But the scope of "
                                    "definition includes template parameters for repeated, string or byte fields. It "
                                    "is there for not possible to define the field where you are using it as we do not "
                                    "know the template value. Try defining the field in the main scope or the one you "
                                    "are using it in.")

                type_name += scope["name"] + "::"
            # Remove the last ::
            type_name = type_name[:-2]

        return type_name

    def get_max_enum_value(self):
        # Return the maximum value used by any of the enum items as a string. This is used to calculate the maximum serialized size.
        max_number = self.definition.descriptor.value[0].number
        for value in self.definition.descriptor.value[1:]:
            if max_number < value.number:
                max_number = value.number
        return str(max_number)
    def get_type(self):
        return "EmbeddedProto::enumeration<" + self.get_type_as_defined() + ", EmbeddedProto::WireFormatter::VarintSize(" + self.get_max_enum_value() + ")>"

    def get_short_type(self):
        return "EmbeddedProto::enumeration<" + self.get_type_as_defined().split("::")[-1] + ", EmbeddedProto::WireFormatter::VarintSize(" + self.get_max_enum_value() + ")>"

    def get_cstdint_type(self):
        # For enums, use the underlying type (uint32_t for the serialized form)
        return "uint32_t"

    def get_default_value(self):
        # A custom (editions) default is the qualified enumerator, e.g.
        # Color::BLUE.
        if self.has_default():
            return self.get_type_as_defined() + "::" + self.descriptor.default_value
        # A CLOSED enum defaults to its first declared enumerator, which may be
        # non-zero (and a raw zero is not necessarily a member of the enum). OPEN
        # enums keep the historical zero-initialised default.
        if self.is_closed() and (self.definition is not None):
            first_enumerator = self.definition.descriptor.value[0].name
            return self.get_type_as_defined() + "::" + first_enumerator
        return "static_cast<" + self.get_type_as_defined() + ">(0)"

    # True when clearing this enum field must assign an explicit default value
    # rather than a plain .clear() (which resets to zero): a custom default or a
    # CLOSED enum whose default is its first (possibly non-zero) enumerator.
    def assign_default_on_clear(self):
        return self.has_default() or self.is_closed()

    def match_field_with_definitions(self, all_types_definitions):
        found = False
        my_type = self.get_type_as_defined()
        for enum_defs in all_types_definitions["enums"]:
            other_scope = enum_defs.scope.get_scope_str()
            if my_type == other_scope:
                self.definition = enum_defs
                found = True
                break

        if not found:
            raise Exception("Unable to find the definition of this enum: " + self.name)

    # Whether the referenced enum is CLOSED (editions enum_type feature). Closedness
    # is a property of the enum definition's scope, not of the field, so it is read
    # from the resolved enum definition.
    def is_closed(self):
        from .Features import EnumType
        if (self.definition is not None) and (self.definition.features is not None):
            return EnumType.CLOSED == self.definition.features["enum_type"]
        return False

    def render_get_set(self, jinja_env):
        return self.render("FieldEnum_GetSet.h.jinja2", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        rendered = self.render("FieldEnum_Deserialize.h.jinja2", jinja_environment=jinja_env)
        return rendered.rstrip()

# -----------------------------------------------------------------------------


# This class is used to wrap around any type of message used as a field.
class FieldMessage(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldMsg.h.jinja2", oneof)

        # Reserve a member variable for the reference to the message definition used for this field.
        self.definition = None

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
        # When the user supplies the storage type, use the plain template parameter as the field type.
        if self.has_custom_storage():
            return self.get_storage_type_param_str()

        return self.get_message_type()

    # Returns the actual nested message C++ type, independent of the customStorage option. Used to resolve the message
    # definition and as the documented base type for the customStorage static_assert.
    def get_message_type(self):
        if not self.definition:
            # When the actual definition is unknown use the protobuf type.
            type_name = self.descriptor.type_name if "." != self.descriptor.type_name[0] else self.descriptor.type_name[1:]
            type_name = type_name.replace(".", "::")
        else:
            scopes = self.get_reduced_scope()
            type_name = ""
            for scope in scopes:
                type_name += scope["name"] + "::"
            # Remove the last ::
            type_name = type_name[:-2]

            tmpl_param = self.get_message_template_parameters()
            if tmpl_param:
                type_name += "<"
                for param in tmpl_param:
                    type_name += param["name"] + ", "
                type_name = type_name[:-2] + ">"

        return type_name

    def get_short_type(self):
        return self.get_type().split("::")[-1]

    def get_default_value(self):
        # Just call the default constructor.
        return ""

    def get_template_parameters(self):
        # When the user supplies the storage type, expose a single plain type parameter without a default. The user
        # must supply a type derived from ::EmbeddedProto::MessageInterface. The nested message template parameters are
        # not exposed because the user owns the complete type.
        if self.has_custom_storage():
            return [{"name": self.get_storage_type_param_str(), "type": "class"}]

        return self.get_message_template_parameters()

    def get_message_template_parameters(self):
        # Get the template parameters from the nested message definition. A deep copy is made because we will rename
        # them to be unique within the parent message scope.
        templates = copy.deepcopy(self.definition.get_templates())

        # Prefix each parameter name with the parent message name and field variable name to avoid collisions when
        # the same message type is used in multiple fields. Track the old-to-new mapping for updating defaults.
        rename_map = {}
        for tmp in templates:
            old_name = tmp["name"]
            new_name = self.parent.name + "_" + self.variable_name + old_name
            rename_map[old_name] = new_name
            tmp["name"] = new_name

        # Default values may reference other template parameter names (e.g. a repeated storage type parameter whose
        # default is a fixed-size type). These references must be updated to use the renamed parameter names.
        for tmp in templates:
            if "default" in tmp:
                default_value = tmp["default"]
                for old_name, new_name in rename_map.items():
                    default_value = default_value.replace(old_name, new_name)
                tmp["default"] = default_value

        return templates

    def match_field_with_definitions(self, all_types_definitions):
        found = False
        # Use the real message type (not the customStorage parameter) to resolve the definition.
        my_type = self.get_message_type()
        for msg_defs in all_types_definitions["messages"]:
            other_scope = msg_defs.scope.get_scope_str()
            if my_type == other_scope:
                self.definition = msg_defs
                found = True
                break

        if not found:
            if self.descriptor.type_name.startswith(".google.protobuf."):
                raise Exception("The field " + self.name + " uses the type " + self.descriptor.type_name + ". No code "
                                "is generated for google/protobuf/descriptor.proto, that file may only be imported to "
                                "declare custom options. Please do not use its types as a field type.")
            raise Exception("Unable to find the definition of this message: " + self.name)

    def register_template_parameters(self):
        # When the user supplies the storage type, this field contributes a single plain storage type parameter. The
        # user owns the complete type so the nested message parameters are not propagated.
        if self.has_custom_storage():
            self.parent.register_child_with_template(self)
            return True

        if self.definition.all_parameters_registered:
            if self.definition.contains_template_parameters:
                self.parent.register_child_with_template(self)
            return True
        else:
            return False

    # Get the whole scope of the definition of this field.
    def get_scope(self):
        return self.definition.scope.get()

    def render_get_set(self, jinja_env):
        return self.render("FieldMsg_GetSet.h.jinja2", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        rendered = self.render("FieldMsg_Deserialize.h.jinja2", jinja_environment=jinja_env)
        return rendered.rstrip()

    def uses_serialize_len(self):
        return True

    def get_size_expression(self):
        return self.get_variable_name() + ".serialized_size()"

    def is_message_type(self):
        """Returns True if this field is a nested message type (not string/bytes)."""
        return True

    def is_delimited(self):
        # Honor the resolved editions message_encoding feature for this field.
        from .Features import MessageEncoding
        if self.resolved_features is not None:
            return MessageEncoding.DELIMITED == self.resolved_features["message_encoding"]
        return False

    def get_storage_base_type(self):
        return "::EmbeddedProto::MessageInterface"

# -----------------------------------------------------------------------------


# This class wraps around any other type of field which is repeated.
class FieldRepeated(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldRepeated.h", oneof)

        # To make use of the field object actual type create one of their objects.
        self.actual_type = Field.factory(proto_descriptor, parent_msg, oneof, already_nested=True)

        # This is the name given to the template parameter for the length.
        self.template_param_str = self.parent.name + "_" + self.variable_name + "REP_LENGTH"

        # Find options we know and use in this type of field.
        self.MaxLength = None
        if self.embedded_proto_options is not None:
            self.MaxLength = self.embedded_proto_options.maxLength

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
        # When callbackStorage is set, emit a concrete streaming storage type parameterised on the
        # element type. Unlike customStorage this is not a template parameter; the element type is
        # known at generation time and no maximum length applies.
        if self.has_callback_storage():
            # A repeated message streams through MessageCallback (DELIMITED groups); scalar/enum
            # elements stream through RepeatedFieldCallback (EXPANDED tag+value per element).
            if self.callback_is_message():
                return "::EmbeddedProto::MessageCallback<" + self.actual_type.get_type() + ">"
            return "::EmbeddedProto::RepeatedFieldCallback<" + self.actual_type.get_type() + ">"

        # When the user supplies the storage type, use the plain template parameter as the field type. No size is
        # appended; a user supplied storage type takes precedence over a maximum length.
        if self.has_custom_storage():
            return self.get_storage_type_param_str()

        # Otherwise use the built-in fixed-size storage. The size is hard coded when a maximum length is set or exposed
        # as a template parameter when it is not.
        type_str = "::EmbeddedProto::RepeatedFieldFixedSize<" + self.actual_type.get_type() + ", "
        if self.MaxLength:
            type_str += str(self.MaxLength) + ">"
        else:
            type_str += self.template_param_str + ">"
        return type_str

    def get_short_type(self):
        # Delegates to get_type() because the storage type may be a template parameter whose name cannot be shortened.
        return self.get_type()

    # As this is a repeated field we need a function to get the type we are repeating.
    def get_base_type(self):
        return self.actual_type.get_type()

    def get_template_parameters(self):
        result = []

        # A callbackStorage field has a concrete, fully specified storage type; it exposes no template
        # parameters (no length, no element parameters).
        if self.has_callback_storage():
            return result

        # When the user supplies the storage type, expose a single plain type parameter without a default. The user
        # must supply a type derived from ::EmbeddedProto::RepeatedField. The maximum length is ignored.
        if self.has_custom_storage():
            result.append({"name": self.get_storage_type_param_str(), "type": "class"})
            return result

        # When no maximum length is set, expose the array length as a C++ template parameter so the user specifies it
        # at compile time.
        if not self.MaxLength:
            result.append({"name": self.template_param_str, "type": "uint32_t"})

        # Include any template parameters required by the element type (e.g. nested message templates).
        result.extend(self.actual_type.get_template_parameters())

        return result

    def match_field_with_definitions(self, all_types_definitions):
        self.actual_type.match_field_with_definitions(all_types_definitions)

    def register_template_parameters(self):
        result = True

        # A callbackStorage field has a concrete storage type and contributes no template parameters.
        if self.has_callback_storage():
            return result

        # When the user supplies the storage type, this field contributes a single plain storage type parameter. The
        # user owns the complete type so the element parameters are not propagated.
        if self.has_custom_storage():
            self.parent.register_child_with_template(self)
            return result

        # Check for the special case where a string or bytes field is nested in the repeated field. Such an element
        # contributes its own length template parameter when no nested maximum length is defined.
        string_or_bytes_field = (FieldDescriptorProto.TYPE_STRING == self.actual_type.descriptor.type) or \
            (FieldDescriptorProto.TYPE_BYTES == self.actual_type.descriptor.type)

        # When the array size or a nested string/bytes length is not fixed, register the template parameter(s).
        if not self.MaxLength or (string_or_bytes_field and not self.actual_type.MaxLength):
            self.parent.register_child_with_template(self)

        # If the array element is a message type it may have its own template parameters (e.g. nested repeated fields
        # or string lengths) that also need to be registered up the chain.
        elif FieldDescriptorProto.TYPE_MESSAGE == self.actual_type.descriptor.type:
            result = self.actual_type.register_template_parameters()

        return result

    def get_storage_base_type(self):
        return "::EmbeddedProto::RepeatedField<" + self.actual_type.get_type() + ">"

    def render_get_set(self, jinja_env):
        return self.render("FieldRepeated_GetSet.h.jinja2", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        # A repeated message callback arrives as a START_GROUP framed element; route it through the
        # message template (which dispatches on the group wire type to deserialize()) rather than the
        # basic deserialize_check_type path, which is LEN-only for a message and would reject a group.
        if self.callback_is_message():
            str = self.render("FieldMsg_Deserialize.h.jinja2", jinja_environment=jinja_env)
            return str.rstrip()
        else:
            str = self.render("FieldBasic_Deserialize.h.jinja2", jinja_environment=jinja_env)
            return str.rstrip()

    def uses_serialize_len(self):
        return True

    def get_size_expression(self):
        # Repeated fields use serialized_size_packed() for packed mode
        # Unpacked mode is handled separately in the template
        return self.get_variable_name() + ".serialized_size_packed()"

    # Repeated scalar/enum/message callback storage serializes EXPANDED (one tag or group per element).
    def callback_is_expanded(self):
        return self.has_callback_storage()

    # True when this is a repeated message field streamed through MessageCallback.
    def callback_is_message(self):
        return self.has_callback_storage() and \
            (FieldDescriptorProto.TYPE_MESSAGE == self.actual_type.descriptor.type)

    # A repeated message callback streams each element as a DELIMITED group; every other repeated
    # field keeps the length-prefixed / expanded encoding selected elsewhere.
    def is_delimited(self):
        if self.callback_is_message():
            return self.actual_type.is_delimited()
        return False

    def is_packed(self):
        # A callback field streams one element at a time and cannot run the length
        # prefix / size pass a packed block needs, so it is always serialized EXPANDED
        # (one tag per element).
        if self.has_callback_storage():
            return False

        # Message, string and bytes elements are length-delimited and can never be
        # packed, regardless of the resolved feature.
        if self.element_is_length_delimited():
            return False

        # For packable scalar / enum element types honor the resolved editions
        # repeated_field_encoding feature. proto3 maps to the proto3 profile whose
        # default is PACKED, preserving the historical behavior.
        if self.resolved_features is not None:
            from .Features import RepeatedFieldEncoding
            return RepeatedFieldEncoding.PACKED == self.resolved_features["repeated_field_encoding"]

        return True

    # True when the repeated element type is serialized as a length-delimited field
    # (message, string, bytes). Such elements always use the expanded (one
    # tag+length per element) form.
    def element_is_length_delimited(self):
        return self.actual_type.descriptor.type in (FieldDescriptorProto.TYPE_MESSAGE,
                                                     FieldDescriptorProto.TYPE_STRING,
                                                     FieldDescriptorProto.TYPE_BYTES)

# -----------------------------------------------------------------------------


# This class wraps a protobuf map field.
#
# Protoc rewrites "map<K,V> foo = 1;" into a repeated message field whose element is a synthetic
# entry message holding the key as field one and the value as field two. On the wire the two are
# identical, so this class reuses the storage and the whole (de)serialization of a repeated message
# field and only adds the map shaped accessors on top of it.
class FieldMap(FieldRepeated):

    # Protobuf fixes the field numbers of the entry message.
    KEY_FIELD_NUMBER = 1
    VALUE_FIELD_NUMBER = 2

    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        # Resolved before the base constructor runs because the callback storage validation it
        # performs already has to know that this field is a map.
        self.entry_descriptor = Field.find_map_entry_descriptor(proto_descriptor, parent_msg)

        super().__init__(proto_descriptor, parent_msg, oneof)

        # The generated entry class. A message definition creates its nested definitions before its
        # fields, so the entry is already available while this field is constructed.
        self.entry_definition = None
        for nested in parent_msg.nested_msg_definitions:
            if nested.descriptor is self.entry_descriptor:
                self.entry_definition = nested

        if self.entry_definition is None:
            raise Exception(self.get_location() + ": unable to find the generated map entry class.")

        self.apply_key_value_max_length()

    # The message and field name, used to point the user at the offending field in an error message.
    def get_location(self):
        return self.parent.name + "." + self.descriptor.name

    # Return the field of the entry class with the given number. The key and the value are looked up
    # by their protobuf field number rather than by their position in the entry.
    def get_entry_field(self, number):
        result = None
        for field in self.entry_definition.fields:
            if number == field.variable_id:
                result = field
        return result

    def get_key_field(self):
        return self.get_entry_field(FieldMap.KEY_FIELD_NUMBER)

    def get_value_field(self):
        return self.get_entry_field(FieldMap.VALUE_FIELD_NUMBER)

    # Push keyMaxLength and valueMaxLength into the string or bytes fields of the entry class.
    #
    # Without this the key and the value are singular fields of a message which carries no options of
    # its own, so each would expose a bare C++ template parameter and the user would have to size the
    # map through the generated entry type instead of through the map field itself.
    def apply_key_value_max_length(self):
        if self.embedded_proto_options is None:
            return

        options = self.embedded_proto_options
        if options.keyMaxLength:
            key_field = self.get_key_field()
            if not isinstance(key_field, BaseStringBytes):
                raise Exception(self.get_location() + ": keyMaxLength is only valid on a map with a "
                                "string key.")
            key_field.MaxLength = options.keyMaxLength

        if options.valueMaxLength:
            value_field = self.get_value_field()
            if not isinstance(value_field, BaseStringBytes):
                raise Exception(self.get_location() + ": valueMaxLength is only valid on a map with "
                                "a string or bytes value.")
            value_field.MaxLength = options.valueMaxLength

    # A map entry is length delimited on the wire, whether the entries are stored resident or
    # streamed through callbacks. Streaming does not switch the framing to a group the way it does
    # for a plain repeated message field, so the bytes stay readable by a standard protoc peer.
    def is_delimited(self):
        return False

    # True when this map streams its entries through user callbacks. The generated code then emits
    # each entry length delimited from a transient, which needs a serialize call of its own.
    def callback_is_len_expanded(self):
        return self.has_callback_storage()

    # Entries are length delimited in both storage modes, so deserialization always follows the same
    # path as any other repeated message field, never the group path a message callback uses.
    def render_deserialize(self, jinja_env):
        return self.render("FieldBasic_Deserialize.h.jinja2", jinja_environment=jinja_env).rstrip()

    def render_get_set(self, jinja_env):
        return self.render("FieldMap_GetSet.h.jinja2", jinja_environment=jinja_env)

    # ---- Types used by the generated map API --------------------------------------------------

    # The C++ type of one entry, the element type of the underlying repeated field.
    def get_entry_type(self):
        return self.actual_type.get_type()

    @staticmethod
    def field_is_string(field):
        return FieldDescriptorProto.TYPE_STRING == field.descriptor.type

    @staticmethod
    def field_is_bytes(field):
        return FieldDescriptorProto.TYPE_BYTES == field.descriptor.type

    @staticmethod
    def field_is_message(field):
        return FieldDescriptorProto.TYPE_MESSAGE == field.descriptor.type

    def key_is_string(self):
        return FieldMap.field_is_string(self.get_key_field())

    def value_is_string(self):
        return FieldMap.field_is_string(self.get_value_field())

    def value_is_bytes(self):
        return FieldMap.field_is_bytes(self.get_value_field())

    def value_is_message(self):
        return FieldMap.field_is_message(self.get_value_field())

    def value_is_enum(self):
        return self.get_value_field().of_type_enum

    # True when the value can be returned by value from a lookup, with a sensible default when the
    # key is absent. Messages and bytes have no such literal default and are read through the
    # Error returning overload instead.
    def value_is_returned_by_value(self):
        return not (self.value_is_message() or self.value_is_bytes())

    # The C++ type a key is passed as. A string key is taken as a plain c style string, which is what
    # a user reaches for, and compares directly against the stored key through FieldString.
    def get_key_param_type(self):
        if self.key_is_string():
            return "const char*"
        return self.get_key_field().get_cstdint_type()

    # The C++ type a value is passed as when writing an entry.
    def get_value_param_type(self):
        value_field = self.get_value_field()
        if self.value_is_string():
            return "const char*"
        if self.value_is_bytes() or self.value_is_message():
            return "const " + value_field.get_type() + "&"
        if self.value_is_enum():
            return value_field.get_type_as_defined()
        return value_field.get_cstdint_type()

    # The C++ type a lookup returns for the value kinds that have a literal default.
    def get_value_return_type(self):
        value_field = self.get_value_field()
        if self.value_is_string():
            return "const char*"
        if self.value_is_enum():
            return value_field.get_type_as_defined()
        return value_field.get_cstdint_type()

    # The value handed back when a key is not present in the map. Scalars and enums reuse the default
    # the generator emits for the field itself, which also covers a CLOSED enum whose first
    # enumerator is not zero.
    def get_value_absent_default(self):
        if self.value_is_string():
            return '""'
        return self.get_value_field().get_default_value()

    # The C++ type the Error returning lookup writes the value into.
    def get_value_out_type(self):
        value_field = self.get_value_field()
        if self.value_is_string() or self.value_is_bytes() or self.value_is_message():
            return value_field.get_type()
        if self.value_is_enum():
            return value_field.get_type_as_defined()
        return value_field.get_cstdint_type()

    # The statement writing the key of a freshly added entry, given the names of the entry variable
    # and of the key parameter. A string is assigned through its field object, every other key kind
    # has a plain setter taking the value.
    def get_key_assign_statement(self, entry, key):
        if self.key_is_string():
            return entry + ".mutable_key().set(" + key + ");"
        return entry + ".set_key(" + key + ");"

    # The statement writing the value of an entry, see get_key_assign_statement.
    def get_value_assign_statement(self, entry, value):
        if self.value_is_string():
            return entry + ".mutable_value().set(" + value + ");"
        return entry + ".set_value(" + value + ");"

    # The expression reading the value out of an entry for the by value lookup. A string is handed
    # back as a c style string, matching get_value_return_type.
    def get_value_read_expression(self, entry):
        if self.value_is_string():
            return entry + ".get_value().get_const()"
        return entry + ".get_value()"


# -----------------------------------------------------------------------------


# This class represents a field we can not include because it causes a recursive inclusion.
class FieldErrorRecursive(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldRepeated.h.jinja2", oneof)

        self.descriptor.type_name = "FieldErrorRecursive"

    def get_type(self):
        return "//"

    def render_get_set(self, jinja_env):
        return self.render("FieldErrorRecursive_GetSet.h.jinja2", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return ""

    def render_deserialize(self, jinja_env):
        return ""
