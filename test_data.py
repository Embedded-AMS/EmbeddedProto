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

from sys import path
path.append('./build/python/')

import simple_types_pb2 as st
import nested_message_pb2 as nm
import repeated_fields_pb2 as rf
import oneof_fields_pb2 as of
import file_to_include_pb2 as fti
import include_other_files_pb2 as iof
from subfolder import file_to_include_from_subfolder_pb2 as ftis
import string_bytes_pb2 as sb


def test_simple_types():
    # A test function used to generate encoded data to test the implementation of the wireformatter
    # and header template.

    msg = st.Test_Simple_Types()
    # msg.a_int32 = -2147483648
    # msg.a_int64 = -9223372036854775808
    # msg.a_uint32 = 0
    # msg.a_uint64 = 0
    # msg.a_sint32 = -2147483648
    # msg.a_sint64 = -9223372036854775808
    # msg.a_bool = 0
    # msg.a_enum = 0
    # msg.a_fixed64 = 0
    # msg.a_sfixed64 = -9223372036854775808
    # msg.a_double = 0
    # msg.a_fixed32 = 0
    # msg.a_sfixed32 = -2147483648
    # msg.a_float = 0
    # msg.a_nested_enum = st.Test_Simple_Types.NE_A

    # msg.a_int32 = 1
    # msg.a_int64 = 1
    # msg.a_uint32 = 1
    # msg.a_uint64 = 1
    # msg.a_sint32 = 1
    # msg.a_sint64 = 1
    # msg.a_bool = 1
    # msg.a_enum = 1
    # msg.a_fixed64 = 1
    # msg.a_sfixed64 = 1
    # msg.a_double = 1
    # msg.a_fixed32 = 1
    # msg.a_sfixed32 = 1
    # msg.a_float = 1
    msg.a_nested_enum = st.Test_Simple_Types.NE_B

    #msg.a_double = pow(2, -1022)
    #msg.a_float = pow(2, -126)

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
      str += "0x{:02x}, ".format(x)

    print(str)
    print()

    x = bytearray([0x08, 0x80, 0x80, 0x80, 0x80, 0x08,
                 0x10, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01,
                 0x28, 0xFF, 0xFF, 0xFF, 0xFF, 0x0F,])
    msg2 = st.Test_Simple_Types()
    msg2.ParseFromString(x)

    print(msg2)


def test_nested_message():
    msg = nm.message_b()

    #msg.u = 1.0
    #msg.v = 1
    #msg.nested_a.x.append(1)
    #msg.nested_a.y = 1.0
    #msg.nested_a.z = 1

    msg.u = 1.7976931348623157e+308         # Max double 1.7976931348623157e+308
    msg.v = pow(2, 31) - 1                  # Max int32
    msg.nested_a.x.append(pow(2, 31) - 1)   # Max int32
    msg.nested_a.y = 3.40282347e+38         # Max float
    msg.nested_a.z = 9223372036854775807    # Max sint64

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
      str += "0x{:02x}, ".format(x)

    print(str)
    print()

    x = bytearray([0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
                   0x3f, 0x1a, 0x09, 0x08, 0x01, 0x15, 0x00, 0x00, 0x80, 0x3f, 0x18, 0x02])

    msg2 = nm.message_b()
    msg2.ParseFromString(x)

    print(msg2)


def test_repeated_fields():
    msg = rf.repeated_fields()

    #msg.x = 0
    msg.y.append(0)
    msg.y.append(1)
    msg.y.append(0)
    #msg.z = 0

    #msg.y.append(pow(2, 32) - 1)
    #msg.y.append(pow(2, 32) - 1)
    #msg.y.append(pow(2, 32) - 1)

    #msg.x = 1
    #msg.y.append(1)
    #msg.y.append(1)
    #msg.y.append(1)
    #msg.z = 1

    #msg.x = pow(2, 32) - 1
    #msg.y.append(pow(2, 32) - 1)
    #msg.y.append(pow(2, 32) - 1)
    #msg.y.append(pow(2, 32) - 1)
    #msg.z = pow(2, 32) - 1


    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


def test_repeated_message():
    msg = rf.repeated_message()

    msg.a = 0
    for i in range(3):
        nmsg = msg.b.add()
        nmsg.u = 0
        nmsg.v = 0
    msg.c = 0

    msg.b[1].u = 1
    msg.b[1].v = 1

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


def test_string():
    msg = sb.text()

    msg.txt = "Foo bar"

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


def test_bytes():
    msg = sb.raw_bytes()

    msg.b = b'\x01\x02\x03\x00'

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


def test_repeated_string_bytes():
    msg = sb.repeated_string_bytes()

    msg.array_of_txt.append("Foo bar 1")
    msg.array_of_txt.append("")
    msg.array_of_txt.append("Foo bar 3")

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


def test_oneof_fields():
    msg = of.message_oneof()

    msg.msg_DEF.varD = 1
    msg.msg_DEF.varE = 22
    msg.msg_DEF.varF = 333

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


def test_included_proto():
    msg = iof.IncludedMessages()

    msg.state = fti.StateA

    msg.msg.a = 1
    msg.msg.b = 1.0

    msg.rf.x = 1
    msg.rf.y.append(1)
    msg.rf.y.append(1)
    msg.rf.y.append(1)
    msg.rf.z = 1

    msg.sub_msg.val = 1
    msg.sub_msg.array.append(1)
    msg.sub_msg.array.append(1)
    msg.sub_msg.ne = ftis.other_folder_msg.NE_B

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
        str += "0x{:02x}, ".format(x)

    print(str)
    print()


#test_simple_types()
#test_repeated_fields()
#test_repeated_message()
#test_string()
#test_bytes()
#test_repeated_string_bytes()
#test_nested_message()
#test_oneof_fields()
test_included_proto()

