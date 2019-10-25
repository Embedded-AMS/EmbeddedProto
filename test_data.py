import build.python.simple_types_pb2 as st
import build.python.nested_message_pb2 as nm
import build.python.repeated_fields_pb2 as rf

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

    msg.a_double = pow(2, -1022)
    msg.a_float = pow(2, -126)

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

    # msg.u = 1.0
    # msg.v = 1.0
    # msg.nested_a.x = 1
    # msg.nested_a.y = 1.0
    # msg.nested_a.z = 1

    msg.u = 0 #pow(2, 1023)
    msg.v = 0 #pow(2, 1023)
    msg.nested_a.x = pow(2, 31) - 1
    msg.nested_a.y = 0 #1.0
    msg.nested_a.z = 0 #1

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

    y = msg.y.append(1)
    y = msg.y.append(1)
    y = msg.y.append(255)

    str = ""
    msg_str = msg.SerializeToString()
    print(len(msg_str))
    print(msg_str)
    for x in msg_str:
      str += "0x{:02x}, ".format(x)

    print(str)
    print()


test_repeated_fields()