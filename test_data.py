import build.python.simple_types_pb2 as st

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