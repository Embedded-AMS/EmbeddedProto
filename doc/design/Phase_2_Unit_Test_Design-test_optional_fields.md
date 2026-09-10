# Phase 2 Unit Test Design: test_optional_fields.cpp

This document details the unit tests for partial serialization of optional fields using the `optional_fields` message from `test/proto/optional_fields.proto`.

## 1. Test Message Reference

```protobuf
message optional_fields 
{
  int32 a = 1;
  optional int32 b = 2;
  float x = 3;  
  optional float y = 4;
  optional position pos = 5;
  optional states state = 6;
  optional bytes bytes_array = 7;
  optional string str = 8;
}

message position 
{
  double xpos = 1;
  double ypos = 2;
  double zpos = 3;
}

enum states
{
  A = 0;
  B = 1;
  C = 2;
}
```

## 2. Key Concepts for Optional Fields

### 2.1 Presence Tracking

Optional fields in protobuf v3 are fields that may or may not be set. In EmbeddedProto, this is tracked through presence flags:
- `has_b()`, `has_y()`, etc. indicate if a field is set
- `clear_b()`, `clear_y()`, etc. clear the presence flag
- Setting a value automatically sets the presence flag

### 2.2 Serialization Behavior

For optional fields:
- If a field is NOT set (presence flag false), it is NOT serialized
- If a field IS set (presence flag true), it IS serialized, even if the value is the default
- This differs from protobuf v3 semantics where default values are not serialized

### 2.3 Partial Serialization Considerations

For partial serialization of optional fields:
- Each field is serialized independently based on its presence flag
- The state machine must track which field is being serialized
- Fields are serialized in field number order (1, 2, 3, ...)
- The presence of optional fields affects the total serialized size

## 3. Test Cases

### 3.1 Basic Presence Tests (Non-Partial)

These tests verify the basic presence behavior before testing partial serialization.

#### 3.1.1 OptionalFields_Presence_NotSet

**Purpose**: Verify that optional fields not set are not serialized.

**Setup**:
- Create `optional_fields<5,10>` message
- Don't set any optional fields
- Set required field `a = 1`

**Expected Results**:
- `has_b()` returns `false`
- `has_y()` returns `false`
- `has_pos()` returns `false`
- `has_state()` returns `false`
- `has_bytes_array()` returns `false`
- `has_str()` returns `false`
- Serialization contains only field `a` (tag 0x08, value 0x01)

#### 3.1.2 OptionalFields_Presence_SetToDefault

**Purpose**: Verify that setting optional fields to default values sets presence flag.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `b = 0` (default for int32)
- Set `y = 0.0f` (default for float)
- Set `state = states::A` (default enum value)

**Expected Results**:
- All `has_*` methods return `true`
- Serialization includes all set fields with their default values

#### 3.1.3 OptionalFields_Presence_Clear

**Purpose**: Verify that clearing optional fields removes them from serialization.

**Setup**:
- Create `optional_fields<5,10>` message
- Set all optional fields to non-default values
- Clear specific fields using `clear_b()`, `clear_y()`, etc.

**Expected Results**:
- Cleared fields return `false` for `has_*`
- Serialization excludes cleared fields
- Other fields remain in serialization

### 3.2 Partial Serialization Tests

#### 3.2.1 PartialSerialize_NoOptionalFields_SufficientBuffer

**Purpose**: Verify partial serialization when no optional fields are set.

**Setup**:
- Create `optional_fields<5,10>` message
- Set only required field `a = 42`
- Don't set any optional fields
- Use `WriteBufferFixedSize<10>` (sufficient for 2 bytes)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains `{0x08, 0x2A}` (field 1, value 42)
- State phase is `COMPLETE`

#### 3.2.2 PartialSerialize_OneOptionalField_SufficientBuffer

**Purpose**: Verify partial serialization with one optional field set.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a = 1` and `b = 42`
- Use `WriteBufferFixedSize<20>` (sufficient for both fields)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains `{0x08, 0x01, 0x10, 0x2A}` (field 1 and 2)
- State phase is `COMPLETE`

#### 3.2.3 PartialSerialize_AllOptionalFields_SufficientBuffer

**Purpose**: Verify partial serialization with all optional fields set.

**Setup**:
- Create `optional_fields<5,10>` message
- Set all fields: `a=1`, `b=42`, `x=1.0f`, `y=2.0f`, `pos` with values, `state=B`, `bytes_array` with data, `str="test"`
- Use `WriteBufferFixedSize<100>` (sufficient for all fields)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains all fields in order
- State phase is `COMPLETE`

#### 3.2.4 PartialSerialize_SplitBetweenRequiredAndOptional

**Purpose**: Verify split between required field and first optional field.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a = 1` and `b = 42`
- Buffer A: `WriteBufferFixedSize<2>` (fits field 1 only)
- Buffer B: `WriteBufferFixedSize<10>` (fits field 2)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (field 1)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x10, 0x2A}` (field 2)

#### 3.2.5 PartialSerialize_SplitBetweenOptionalFields

**Purpose**: Verify split between two optional fields.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `b = 42` and `y = 3.14f`
- Buffer A: `WriteBufferFixedSize<2>` (fits field 2 only)
- Buffer B: `WriteBufferFixedSize<5>` (fits field 4)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x10, 0x2A}` (field 2)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x25, 0xC3, 0xF5, 0x48, 0x40}` (field 4, float 3.14)

#### 3.2.6 PartialSerialize_OptionalFieldNotSet_SkipInState

**Purpose**: Verify that the state machine skips optional fields that are not set.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a = 1`, `b = 42`, and `y = 3.14f`
- Don't set `x` (field 3)
- Use `WriteBufferFixedSize<20>`

**Expected Results**:
- Serialization contains fields 1, 2, and 4 (skips field 3)
- State transitions: field 1 → field 2 → field 4 (skips field 3)
- Returns `NO_ERRORS` on first call

#### 3.2.7 PartialSerialize_NestedMessageOptional_SufficientBuffer

**Purpose**: Verify partial serialization of optional nested message field.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a = 1` and `pos` with `xpos=1.0`, `ypos=2.0`, `zpos=3.0`
- Use `WriteBufferFixedSize<50>` (sufficient for field 1 + nested message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains field 1 tag+value and field 5 tag+size+nested_data
- State phase is `COMPLETE`

#### 3.2.8 PartialSerialize_NestedMessageOptional_SplitTagAndSize

**Purpose**: Verify split between nested message tag and size.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `pos` with some values
- Buffer A: `WriteBufferFixedSize<1>` (fits only tag byte)
- Buffer B: `WriteBufferFixedSize<20>` (fits size + nested data)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x2A}` (tag for field 5)
- Second call returns `NO_ERRORS`
- Buffer B contains size + nested message data

#### 3.2.9 PartialSerialize_NestedMessageOptional_SplitSizeAndData

**Purpose**: Verify split between nested message size and data.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `pos` with values requiring 24 bytes (3 doubles)
- Buffer A: `WriteBufferFixedSize<2>` (fits tag + 1 byte of size)
- Buffer B: `WriteBufferFixedSize<30>` (fits remaining size + data)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x2A, 0x18}` (tag + first byte of size)
- Second call returns `NO_ERRORS`
- Buffer B contains remaining size bytes + nested data

#### 3.2.10 PartialSerialize_StringOptional_SufficientBuffer

**Purpose**: Verify partial serialization of optional string field.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `str = "Hello World"`
- Use `WriteBufferFixedSize<20>` (sufficient for tag + size + string)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains `{0x42, 0x0B, 0x48, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x57, 0x6F, 0x72, 0x6C, 0x64}`
- State phase is `COMPLETE`

#### 3.2.11 PartialSerialize_StringOptional_SplitTagAndSize

**Purpose**: Verify split between string tag and size.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `str = "test"`
- Buffer A: `WriteBufferFixedSize<1>` (fits only tag)
- Buffer B: `WriteBufferFixedSize<10>` (fits size + string)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x42}`
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x04, 0x74, 0x65, 0x73, 0x74}`

#### 3.2.12 PartialSerialize_StringOptional_SplitSizeAndData

**Purpose**: Verify split between string size and data.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `str = "Hello"`
- Buffer A: `WriteBufferFixedSize<2>` (fits tag + size)
- Buffer B: `WriteBufferFixedSize<10>` (fits string data)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x42, 0x05}`
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x48, 0x65, 0x6C, 0x6C, 0x6F}`

#### 3.2.13 PartialSerialize_BytesOptional_SufficientBuffer

**Purpose**: Verify partial serialization of optional bytes field.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `bytes_array` with 4 bytes: `{0x01, 0x02, 0x03, 0x04}`
- Use `WriteBufferFixedSize<10>` (sufficient)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains `{0x3A, 0x04, 0x01, 0x02, 0x03, 0x04}`
- State phase is `COMPLETE`

#### 3.2.14 PartialSerialize_EnumOptional_SufficientBuffer

**Purpose**: Verify partial serialization of optional enum field.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `state = states::C` (value 2)
- Use `WriteBufferFixedSize<5>` (sufficient)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains `{0x30, 0x02}` (field 6, enum value 2)
- State phase is `COMPLETE`

#### 3.2.15 PartialSerialize_MixedFieldTypes_LoopSmallBuffers

**Purpose**: Verify serialization of mixed field types using multiple small buffers.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a=1`, `b=42`, `y=3.14f`, `state=B`, `str="test"`
- Loop with `WriteBufferFixedSize<8>` buffers
- Collect data in master buffer

**Pseudo-code**:
```cpp
optional_fields<5,10> msg;
msg.set_a(1);
msg.set_b(42);
msg.set_y(3.14f);
msg.set_state(states::B);
msg.set_str("test", 4);

std::array<uint8_t, 50> collected_data;
uint32_t total_bytes = 0;
optional_fields<5,10>::StateStack state;

EmbeddedProto::Error result = EmbeddedProto::Error::BUFFER_FULL;
while(EmbeddedProto::Error::BUFFER_FULL == result) 
{
  WriteBufferFixedSize<8> small_buffer;
  result = msg.serialize_partial(small_buffer, state.root());
  
  // Copy to collected buffer
  memcpy(&collected_data[total_bytes], small_buffer.data(), small_buffer.get_size());
  total_bytes += small_buffer.get_size();
}

EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, result);
// Verify collected_data contains expected bytes
```

**Expected Results**:
- Multiple iterations with `BUFFER_FULL`
- Final iteration returns `NO_ERRORS`
- Collected data equals expected serialized bytes

#### 3.2.16 PartialSerialize_StatePersistence_AcrossCalls

**Purpose**: Verify that state correctly persists across partial serialization calls.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a=1`, `b=42`, `y=3.14f`
- Use `WriteBufferFixedSize<4>` (fits first field, partial second)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer contains `{0x08, 0x01, 0x10, 0x2A}` (both fields fit)
- State shows field 4 (y) as next field
- Second call with fresh buffer returns `NO_ERRORS`
- Second buffer contains field 4 data

#### 3.2.17 PartialSerialize_StateReset_Reuse

**Purpose**: Verify that state can be reset and reused for another message.

**Setup**:
- Create two `optional_fields<5,10>` messages
- Message 1: set `a=1`, `b=42`
- Message 2: set `a=2`, `b=99`
- Use same state stack for both

**Expected Results**:
- Serialize message 1, verify correct output
- Reset state: `state.reset()`
- Clear buffer
- Serialize message 2, verify correct output (different from message 1)

#### 3.2.18 PartialSerialize_ErrorHandling_BufferTooSmall

**Purpose**: Verify error handling when buffer is too small for any field.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a=1`
- Use `WriteBufferFixedSize<1>` (too small for 2-byte field)

**Expected Results**:
- `serialize_partial()` returns `BUFFER_FULL`
- Buffer contains 0 bytes (rollback)
- State remains valid for retry
- Second call with larger buffer succeeds

#### 3.2.19 PartialSerialize_EmptyOptionalFields

**Purpose**: Verify serialization when optional fields are set but empty.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a=1`
- Set `str` to empty string: `msg.set_str("", 0)`
- Set `bytes_array` to empty: `msg.mutable_bytes_array().clear()`
- Set `pos` but clear it: `msg.mutable_pos().clear()`

**Expected Results**:
- Presence flags are true for all set fields
- Serialization includes all fields with zero/empty values
- Empty string: `{0x42, 0x00}`
- Empty bytes: `{0x3A, 0x00}`
- Empty nested: `{0x2A, 0x00}`

#### 3.2.20 PartialSerialize_LargeString_SplitAcrossBuffers

**Purpose**: Verify large string split across multiple buffers.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `str` to 50-character string
- Use `WriteBufferFixedSize<10>` buffers

**Expected Results**:
- First call: tag + size in buffer, returns `BUFFER_FULL`
- Subsequent calls: partial string data, returns `BUFFER_FULL`
- Final call: remaining string data, returns `NO_ERRORS`
- All string data correctly reassembled

### 3.3 Edge Cases

#### 3.3.1 PartialSerialize_MaximumInt32Value

**Purpose**: Verify serialization of maximum int32 value (requires 5 bytes for varint).

**Setup**:
- Create `optional_fields<5,10>` message
- Set `b = INT32_MAX`
- Use `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains correct 5-byte varint encoding

#### 3.3.2 PartialSerialize_NegativeInt32Value

**Purpose**: Verify serialization of negative int32 value.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `b = -42`
- Use `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains correct zig-zag encoded varint

#### 3.3.3 PartialSerialize_FloatSpecialValues

**Purpose**: Verify serialization of special float values.

**Setup**:
- Create `optional_fields<5,10>` message
- Test with `y = 0.0f`, `y = -0.0f`, `y = INFINITY`, `y = NAN`
- Use `WriteBufferFixedSize<10>`

**Expected Results**:
- All special values serialize correctly
- Returns `NO_ERRORS` for each

#### 3.3.4 PartialSerialize_ZeroByteBuffer

**Purpose**: Verify behavior with zero-capacity buffer.

**Setup**:
- Create `optional_fields<5,10>` message
- Set `a=1`
- Use `WriteBufferFixedSize<0>`

**Expected Results**:
- Returns `BUFFER_FULL`
- State remains valid
- No data written to buffer

## 4. Test Implementation Notes

### 4.1 State Setup

Each test should create an `optional_fields<5,10>::StateStack` and use `state.root()`.

### 4.2 Buffer Management

Between partial calls, clear the buffer with `buffer.clear()` or use a fresh buffer instance.

### 4.3 Verification

- Compare buffer contents byte-by-byte with expected data
- Verify return codes (`NO_ERRORS`, `BUFFER_FULL`)
- For loop tests, verify total byte count matches expected serialized size
- Verify presence flags before and after serialization

### 4.4 Expected Data

Calculate expected byte sequences based on protobuf wire format:
- Tag: `(field_number << 3) | wire_type`
- Varint: variable-length encoding
- Fixed32/64: little-endian byte order
- Length-delimited: tag + size (varint) + data

### 4.5 Conditional Compilation

Tests should be wrapped in appropriate preprocessor guards if partial serialization is feature-flagged.

## 5. Test Organization

Group tests by complexity:
1. Basic presence tests (non-partial)
2. Simple partial serialization (single fields)
3. Multi-field partial serialization
4. Nested message partial serialization
5. String/bytes partial serialization
6. Edge cases and error handling
7. State management tests

Start with simple cases and build up to complex scenarios involving multiple buffers and field types.