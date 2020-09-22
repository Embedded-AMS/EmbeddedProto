#
# Copyright (C) 2020 Embedded AMS B.V. - All Rights Reserved
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
#   Johan Huizingalaan 763a
#   1066 VH, Amsterdam
#   the Netherlands
#

from google.protobuf.descriptor_pb2 import FieldDescriptorProto
import copy
import jinja2


# This class is the base class for any kind of field used in protobuf messages.
class Field:
    def __init__(self, proto_descriptor, parent_msg, template_filename, oneof=None):
        # A reference to the FieldDescriptorProto object which defines this field.
        self.descriptor = proto_descriptor

        # A reference to the parent message in which this field is defined.
        self.parent = parent_msg

        # If this field is part of a oneof this is the reference to it.
        self.oneof = oneof

        self.name = self.descriptor.name
        self.variable_name = self.name + "_"
        self.variable_id_name = self.name.upper()
        self.variable_id = self.descriptor.number
        self.template_file = template_filename


    @staticmethod
    # This function create the appropriate field object for a variable defined in the message.
    # The descriptor and parent message parameters are required parameters, all field need them to be created. The oneof
    # parameter is only required for fields which are part of a oneof. The parameter is the reference to the oneof
    # object.
    # The last parameter is not to be used manually. It is set when we are already in a FieldNested class.
    def factory(proto_descriptor, parent_msg, oneof=None, already_nested=False):
        if (FieldDescriptorProto.LABEL_REPEATED == proto_descriptor.label) and not already_nested:
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
        return self.variable_name

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

    def render(self, filename, jinja_environment):
        template = jinja_environment.get_template(filename)
        try:
            rendered_str = template.render(field=self, environment=jinja_environment)

        except jinja2.UndefinedError as e:
            print("UndefinedError exception: " + str(e))
        except jinja2.TemplateRuntimeError as e:
            print("TemplateRuntimeError exception: " + str(e))
        except jinja2.TemplateAssertionError as e:
            print("TemplateAssertionError exception: " + str(e))
        except jinja2.TemplateSyntaxError as e:
            print("TemplateSyntaxError exception: " + str(e))
        except jinja2.TemplateError as e:
            print("TemplateError exception: " + str(e))
        except Exception as e:
            print("Template renderer exception: " + str(e))
        else:
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

    # A dictionary to convert the protobuf wire type into a C++ type.
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

    def get_default_value(self):
        return self.type_to_default_value[self.descriptor.type]

    def render_get_set(self, jinja_env):
        return self.render("FieldBasic_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldBasic_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        return self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class defines a string field
class FieldString(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldString.h", oneof)

        # This is the name given to the template parameter for the length.
        self.template_param_str = self.variable_name + "LENGTH"

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
        return "::EmbeddedProto::FieldString"

    def get_short_type(self):
        return "FieldString"

    def get_default_value(self):
        return "''"

    def get_template_parameters(self):
        return [{"name": self.template_param_str, "type": "uint32_t"}]

    def register_template_parameters(self):
        self.parent.scope.register_template_parameters(self)
        return True

    def render_get_set(self, jinja_env):
        return self.render("FieldString_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldRepeated_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        return self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class defines a bytes array field
class FieldBytes(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldBytes.h", oneof)

        # This is the name given to the template parameter for the length.
        self.template_param_str = self.variable_name + "LENGTH"

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
        return "::EmbeddedProto::FieldBytes"

    def get_short_type(self):
        return "FieldBytes"

    def get_default_value(self):
        return "0U"

    def get_template_parameters(self):
        return [{"name": self.template_param_str, "type": "uint32_t"}]

    def register_template_parameters(self):
        self.parent.register_child_with_template(self)
        return True

    def render_get_set(self, jinja_env):
        return self.render("FieldBytes_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldRepeated_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        return self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)

# -----------------------------------------------------------------------------


# This class is used to wrap around any enum used as a field.
class FieldEnum(Field):
    def __init__(self, proto_descriptor, parent_msg, oneof=None):
        super().__init__(proto_descriptor, parent_msg, "FieldEnum.h", oneof)

        # Reserve a member variable for the reference to the enum definition used for this field.
        self.definition = None

    def get_wire_type_str(self):
        return "VARINT"

    def get_type(self):
        if not self.definition:
            # When the actual definition is unknown use the protobuf type.
            type = self.descriptor.type_name if "." != self.descriptor.type_name[0] else self.descriptor.type_name[1:]
            type = type.replace(".", "::")
        else:
            type = "TODO"
        return type

    def get_short_type(self):
        return self.get_type().split("::")[-1]

    def get_default_value(self):
        return "static_cast<" + self.get_type() + ">(0)"

    def match_field_with_definitions(self, all_types_definitions):
        found = False
        my_type = self.get_type()
        for enum_defs in all_types_definitions["enums"]:
            other_scope = enum_defs.scope.get_scope_str()
            if my_type == other_scope:
                found = True
                break

        if not found:
            raise Exception("Unable to match enum type for: " + self.name)

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
            tmp["name"] = self.variable_name + tmp["name"]
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
            raise Exception("Unable to match enum type for: " + self.name)

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

    # Get the scope relevant compared to the scope this field is used in.
    def get_reduced_scope(self):
        return self.get_scope()

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
        self.template_param_str = self.variable_name + "LENGTH"

    def get_wire_type_str(self):
        return "LENGTH_DELIMITED"

    def get_type(self):
        return "::EmbeddedProto::RepeatedFieldFixedSize<" + self.actual_type.get_type() + ", " + \
               self.template_param_str + ">"

    def get_short_type(self):
        return "::EmbeddedProto::RepeatedFieldFixedSize<" + self.actual_type.get_short_type() + ", " + \
               self.template_param_str + ">"

    # As this is a repeated field we need a function to get the type we are repeating.
    def get_base_type(self):
        return self.actual_type.get_type()

    def get_template_parameters(self):
        result = [{"name": self.template_param_str, "type": "uint32_t"}]
        result.extend(self.actual_type.get_template_parameters())
        return result

    def match_field_with_definitions(self, all_types_definitions):
        self.actual_type.match_field_with_definitions(all_types_definitions)

    def register_template_parameters(self):
        self.parent.register_child_with_template(self)
        return True

    def render_get_set(self, jinja_env):
        return self.render("FieldRepeated_GetSet.h", jinja_environment=jinja_env)

    def render_serialize(self, jinja_env):
        return self.render("FieldRepeated_Serialize.h", jinja_environment=jinja_env)

    def render_deserialize(self, jinja_env):
        return self.render("FieldBasic_Deserialize.h", jinja_environment=jinja_env)
