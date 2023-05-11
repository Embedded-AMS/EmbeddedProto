#
# Copyright (C) 2020-2023 Embedded AMS B.V. - All Rights Reserved
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
# <https://EmbeddedProto.com/license/>.
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
import copy


# This class is the base class for any kind of field used in protobuf messages.
class Field:
    def __init__(self, proto_descriptor, parent_msg, template_filename, oneof=None):
        # A reference to the FieldDescriptorProto object which defines this field.
        self.descriptor = proto_descriptor

        # A reference to the parent message in which this field is defined.
        self.parent = parent_msg

        # Is this field optional, so do we need to track the presence of the field.
        self.optional = self.descriptor.proto3_optional

        # If this field is part of an oneof this is the reference to it.
        self.oneof = oneof

        self.name = self.descriptor.name
        self.variable_name = self.name + "_"
        self.variable_id_name = self.name.upper()
        self.variable_id = self.descriptor.number
        self.template_file = template_filename

        self.of_type_enum = FieldDescriptorProto.TYPE_ENUM == proto_descriptor.type

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

# -----------------------------------------------------------------------------


# This class is used to define any type of basic field.
class FieldBasic(Field):
    # A dictionary to convert the wire type into a default value.
    type_to_default_value = {FieldDescriptorProto.TYPE_DOUBLE:   "0.0",
                             FieldDescriptorProto.TYPE_FLOAT:    "0.0",
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
        super().__init__(proto_descriptor, parent_msg, "FieldBasic.h", oneof)

    def get_wire_type_str(self):
        return self.type_to_wire_type[self.descriptor.type]

    def get_type(self):
        return self.type_to_cpp_type[self.descriptor.type]

    def get_short_type(self):
        return self.get_type().split("::")[-1]

    def get_cstdint_type(self):
        return self.type_to_cstdint[self.descriptor.type]

    def get_default_value(self):
        return self.type_to_default_value[self.descriptor.type]

    def render_get_set(self, jinja_env):
        return self.render("FieldBasic_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldBasic_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        str = self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)
        return str.rstrip()

# -----------------------------------------------------------------------------


# A base class for both the String and Bytes type field
class BaseStringBytes(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldString.h", oneof)

        # This is the name given to the template parameter for the length.
        self.template_param_str = self.parent.name + "_" + self.variable_name + "LENGTH"

        # Find options we know and use in this type of field.
        self.MaxLength = None
        try:
            import embedded_proto_options_pb2
        except Exception as e:
            pass
        else:
            if self.descriptor.options.HasExtension(embedded_proto_options_pb2.options):
                self.MaxLength = self.descriptor.options.Extensions[embedded_proto_options_pb2.options].maxLength

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_template_parameters(self):
        result = []
        # When we do not have a maximum length specified add the length as a template param.
        if not self.MaxLength:
            result.append({"name": self.template_param_str, "type": "uint32_t"})
        return result

    def register_template_parameters(self):
        # If we do not have a max length defined register the template parameter.
        if not self.MaxLength:
            self.parent.register_child_with_template(self)
        return True

    def render_serialize(self, jinja_env):
        return self.render("FieldRepeated_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        str = self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)
        return str.rstrip()

# -----------------------------------------------------------------------------


# This class defines a string field
class FieldString(BaseStringBytes):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, oneof)

    def get_type(self):
        str_type = "::EmbeddedProto::FieldString<"
        if self.MaxLength:
            str_type += str(self.MaxLength) + ">"
        else:
            str_type += self.template_param_str + ">"
        return str_type

    def get_short_type(self):
        return "FieldString"

    def render_get_set(self, jinja_env):
        return self.render("FieldString_GetSet.h", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class defines a bytes array field
class FieldBytes(BaseStringBytes):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, oneof)

    def get_type(self):
        str_type = "::EmbeddedProto::FieldBytes<"
        if self.MaxLength:
            str_type += str(self.MaxLength) + ">"
        else:
            str_type += self.template_param_str + ">"
        return str_type

    def get_short_type(self):
        return "FieldBytes"

    def render_get_set(self, jinja_env):
        return self.render("FieldBytes_GetSet.h", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class is used to wrap around any enum used as a field.
class FieldEnum(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldEnum.h", oneof)

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

    def get_type(self):
        return "EmbeddedProto::enumeration<" + self.get_type_as_defined() + ">"

    def get_short_type(self):
        return "EmbeddedProto::enumeration<" + self.get_type_as_defined().split("::")[-1] + ">"

    def get_default_value(self):
        return "static_cast<" + self.get_type_as_defined() + ">(0)"

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

    def render_get_set(self, jinja_env):
        return self.render("FieldEnum_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldEnum_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        return self.render("FieldEnum_Deserialize.h", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class is used to wrap around any type of message used as a field.
class FieldMessage(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldMsg.h", oneof)

        # Reserve a member variable for the reference to the message definition used for this field.
        self.definition = None

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
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

            tmpl_param = self.get_template_parameters()
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
        # Get the template names used by the definition.
        templates = copy.deepcopy(self.definition.get_templates())
        # Next add our variable name to make them unique.
        for tmp in templates:
            tmp["name"] = self.parent.name + "_" + self.variable_name + tmp["name"]
        return templates

    def match_field_with_definitions(self, all_types_definitions):
        found = False
        my_type = self.get_type()
        for msg_defs in all_types_definitions["messages"]:
            other_scope = msg_defs.scope.get_scope_str()
            if my_type == other_scope:
                self.definition = msg_defs
                found = True
                break

        if not found:
            raise Exception("Unable to find the definition of this message: " + self.name)

    def register_template_parameters(self):
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
        return self.render("FieldMsg_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldMsg_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        return self.render("FieldMsg_Deserialize.h", jinja_environment=jinja_env)

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
        try:
            import embedded_proto_options_pb2
        except Exception as e:
            pass
        else:
            if self.descriptor.options.HasExtension(embedded_proto_options_pb2.options):
                self.MaxLength = self.descriptor.options.Extensions[embedded_proto_options_pb2.options].maxLength

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
        type_str = "::EmbeddedProto::RepeatedFieldFixedSize<" + self.actual_type.get_type() + ", "
        if self.MaxLength:
            type_str += str(self.MaxLength) + ">"
        else:
            type_str += self.template_param_str + ">"
        return type_str

    def get_short_type(self):
        type_str = "::EmbeddedProto::RepeatedFieldFixedSize<" + self.actual_type.get_type() + ", "
        if self.MaxLength:
            type_str += str(self.MaxLength) + ">"
        else:
            type_str += self.template_param_str + ">"
        return type_str

    # As this is a repeated field we need a function to get the type we are repeating.
    def get_base_type(self):
        return self.actual_type.get_type()

    def get_template_parameters(self):
        result = []
        # When we do not have a maximum length specified add the length as a template param.
        if not self.MaxLength:
            result.append({"name": self.template_param_str, "type": "uint32_t"})
        result.extend(self.actual_type.get_template_parameters())
        return result

    def match_field_with_definitions(self, all_types_definitions):
        self.actual_type.match_field_with_definitions(all_types_definitions)

    def register_template_parameters(self):
        # If we do not have a max length defined register the template parameter.
        if not self.MaxLength:
            self.parent.register_child_with_template(self)
        return True

    def render_get_set(self, jinja_env):
        return self.render("FieldRepeated_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldRepeated_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        str = self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)
        return str.rstrip()

# -----------------------------------------------------------------------------


# This class represents a field we can not include because it causes a recursive inclusion.
class FieldErrorRecursive(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldRepeated.h", oneof)

        self.descriptor.type_name = "FieldErrorRecursive"

    def get_type(self):
        return "//"

    def render_get_set(self, jinja_env):
        return self.render("FieldErrorRecursive_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return ""

    def render_deserialize(self, jinja_env):
        return ""

