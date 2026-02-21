# Phase 2 Unit Test Design: test_oneof_fields.cpp

This document details the unit tests for partial serialization of oneof fields using the test messages from `test/proto/oneof_fields.proto`.

## 1. Test Message Reference

```protobuf
message some_ABC
{
  int32 varA = 1;
  int32 varB = 2;
  int32 varC = 3;
}

message some_DEF
{
  int32 varD = 1;
  int32 varE = 2;
  int32 varF = 3;
}

message message_oneof 
{
  int32 a = 1;

  enum States {
    Idle = 0;
    Run = 1;
    Done = 2;
    Error = 3;
  }

  oneof xyz 
  {
    int32 x = 5;
    int32 y = 6;
    int32 z = 7;
    States state = 8;
  }

  int32 b = 10;

  oneof uvw 
  {
    float u = 15;
    float v = 16;
    float w = 17;
  }

  oneof message 
  {
    some_ABC msg_ABC = 20;
    some_DEF msg_DEF = 21;
  }
}

message nested_oneof {
  message_oneof msg_oneof = 1;
}

message string_bytes_oneof
{
  oneof sb
  {
    string name = 1;
    bytes  data = 2;
  }
}

message combined_oneof
{
  oneof combi
  {
    message_oneof msg_oneof = 1;
    string_bytes_oneof msg_sb_oneof = 2;
  }
}

message oneof_sigle {
  oneof single_data {
    int32 single_num = 1;
  }
}
```

## 2. Key Concepts for Oneof Fields

### 2.1 Wire Format
Oneof fields are serialized identically to regular fields - only one field from the oneof group is serialized at a time. The wire format depends on the selected field type:
- Scalar types: `[tag][value]`
- Nested messages: `[tag][size][nested_message_bytes...]`
- Strings/bytes: `[tag][size][data...]`

### 2.2 State Machine for Oneof Fields

**Serialization:**
For scalar oneof fields:
1. **TAG phase**: Write tag (field_id << 3 | wire_type)
2. **DATA phase**: Write value

For nested message oneof fields:
1. **TAG phase**: Write tag (field_id << 3 | 2)
2. **SIZE phase**: Calculate nested message size, write size varint
3. **DATA phase**: Delegate to nested message's `serialize_partial` with child state

### 2.3 State Fields Used
- `phase`: TAG → SIZE (for LENGTH_DELIMITED) → DATA → COMPLETE
- `field_id`: Field number of the selected oneof field
- `size_value`: Total size of nested message (for message oneof fields)
- `bytes_remaining`: Bytes left to read/write in nested message
- `child`: Pointer to nested message's state

### 2.4 Oneof-Specific Behavior
- Only the currently selected field is serialized
- Setting a new field in the oneof clears the previous selection
- Default values are serialized when explicitly set (unlike regular fields)

## 3. Test Cases - Scalar Oneof Fields

### 3.1 PartialSerialize_ScalarOneof_SufficientBuffer

**Purpose**: Verify that partial serialization of a scalar oneof field works when buffer is sufficient.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `x = 1` (from oneof xyz)
  - `b = 1`

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains complete serialized message (6 bytes)

**Expected Bytes**:
```
{0x08, 0x01,  // a
 0x50, 0x01,  // b
 0x28, 0x01}  // x
```

---

### 3.2 PartialSerialize_ScalarOneof_SplitBeforeOneof

**Purpose**: Verify that serialization can be split between regular field and oneof field.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `x = 1` (from oneof xyz)
  - `b = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (fits `a` field only)
- Buffer B: `WriteBufferFixedSize<10>` (fits remaining fields)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (a field)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x50, 0x01, 0x28, 0x01}` (b and x fields)

---

### 3.3 PartialSerialize_ScalarOneof_SplitAfterOneof

**Purpose**: Verify that serialization can be split after oneof field.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `x = 1` (from oneof xyz)
  - `b = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<4>` (fits `a` and `b` fields)
- Buffer B: `WriteBufferFixedSize<10>` (fits x field)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01, 0x50, 0x01}` (a and b fields)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x28, 0x01}` (x field)

---

### 3.4 PartialSerialize_MultipleOneofs_SufficientBuffer

**Purpose**: Verify partial serialization with multiple oneof groups.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `x = 1` (from oneof xyz)
  - `b = 1`
  - `v = 1.0` (from oneof uvw)

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains 12 bytes

**Expected Bytes**:
```
{0x08, 0x01,  // a
 0x50, 0x01,  // b
 0x28, 0x01,  // x
 0x85, 0x01, 0x00, 0x00, 0x80, 0x3f}  // v
```

---

### 3.5 PartialSerialize_MultipleOneofs_SplitBetweenOneofs

**Purpose**: Verify that serialization can be split between two oneof groups.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `x = 1` (from oneof xyz)
  - `b = 1`
  - `v = 1.0` (from oneof uvw)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<6>` (fits a, b, x fields)
- Buffer B: `WriteBufferFixedSize<10>` (fits v field)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01, 0x50, 0x01, 0x28, 0x01}` (a, b, x fields)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x85, 0x01, 0x00, 0x00, 0x80, 0x3f}` (v field)

---

### 3.6 PartialSerialize_EnumOneof_SufficientBuffer

**Purpose**: Verify partial serialization of enum field within oneof.

**Message Setup**:
- `message_oneof` with:
  - `state = States::Run` (from oneof xyz)

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x40, 0x01}` (state field)

---

### 3.7 PartialSerialize_FloatOneof_Rollback

**Purpose**: Verify rollback when float oneof field cannot fit entirely.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `v = 1.0` (from oneof uvw, requires 6 bytes: tag 2 bytes + 4 data bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<5>` (fits a but not v)
- Buffer B: `WriteBufferFixedSize<10>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (a field only, v rolled back)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x85, 0x01, 0x00, 0x00, 0x80, 0x3f}` (v field)

---

### 3.8 PartialSerialize_OneofSetToZero

**Purpose**: Verify that oneof fields serialize even when set to default value (unlike regular fields).

**Message Setup**:
- `message_oneof` with:
  - `x = 0` (explicitly set to zero)

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x28, 0x00}` (x field with value 0)

---

## 4. Test Cases - Nested Message Oneof Fields

### 4.1 PartialSerialize_NestedOneof_SufficientBuffer

**Purpose**: Verify partial serialization of nested message within oneof.

**Message Setup**:
- `message_oneof` with:
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains complete serialized message

**Expected Bytes**:
```
{0xa2, 0x01, 0x08,  // tag and size of msg_ABC
 0x08, 0x01,        // varA
 0x10, 0x16,        // varB
 0x18, 0xcd, 0x02}  // varC
```

---

### 4.2 PartialSerialize_NestedOneof_SplitAtTag

**Purpose**: Verify that serialization can be split at nested message tag.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (fits a field only)
- Buffer B: `WriteBufferFixedSize<20>` (fits remaining message)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (a field)
- Second call returns `NO_ERRORS`
- Buffer B contains nested message bytes

---

### 4.3 PartialSerialize_NestedOneof_SplitAtSize

**Purpose**: Verify that serialization can be split between nested message tag and size.

**Message Setup**:
- `message_oneof` with:
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (fits tag only)
- Buffer B: `WriteBufferFixedSize<15>` (fits remaining message)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0xa2, 0x01}` (tag bytes)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x08, 0x08, 0x01, 0x10, 0x16, 0x18, 0xcd, 0x02}` (size + nested data)

---

### 4.4 PartialSerialize_NestedOneof_SplitDuringData

**Purpose**: Verify that serialization can be split during nested message data.

**Message Setup**:
- `message_oneof` with:
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<6>` (fits tag + size + partial data)
- Buffer B: `WriteBufferFixedSize<10>` (fits remaining data)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0xa2, 0x01, 0x08, 0x08, 0x01, 0x10}` (tag + size + varA + partial varB tag)
- Second call returns `NO_ERRORS`
- Buffer B contains remaining bytes

---

### 4.5 PartialSerialize_NestedOneof_EmptyMessage

**Purpose**: Verify partial serialization when nested oneof message has no fields set.

**Message Setup**:
- `message_oneof` with:
  - `msg_ABC` (no fields set, but oneof is selected)

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0xa2, 0x01, 0x00}` (tag + size 0)

---

### 4.6 PartialSerialize_NestedOneof_Rollback

**Purpose**: Verify rollback when nested message tag cannot fit.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<3>` (fits a but only 1 byte left - not enough for tag)
- Buffer B: `WriteBufferFixedSize<20>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (a field only)
- Second call returns `NO_ERRORS`
- Buffer B contains complete nested message

---

### 4.7 PartialSerialize_AlternateNestedOneof

**Purpose**: Verify partial serialization with alternate nested message in oneof.

**Message Setup**:
- `message_oneof` with:
  - `msg_DEF` with `varD = 1`, `varE = 22`, `varF = 333`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<5>` buffers

**Expected Results**:
- Multiple calls with `BUFFER_FULL`
- Final call returns `NO_ERRORS`
- Collected data equals complete serialized message

**Expected Bytes**:
```
{0xaa, 0x01, 0x08,  // tag and size of msg_DEF
 0x08, 0x01,        // varD
 0x10, 0x16,        // varE
 0x18, 0xcd, 0x02}  // varF
```

---

## 5. Test Cases - Nested Oneof (message containing message with oneof)

### 5.1 PartialSerialize_NestedOneof_TwoLevels_SufficientBuffer

**Purpose**: Verify partial serialization of nested_oneof message (two levels of nesting).

**Message Setup**:
- `nested_oneof` with:
  - `msg_oneof` with `a = 1`, `x = 1`, `b = 1`

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains complete serialized message

**Expected Bytes**:
```
{0x0a, 0x06,        // tag and size of msg_oneof
 0x08, 0x01,        // a
 0x50, 0x01,        // b
 0x28, 0x01}        // x
```

---

### 5.2 PartialSerialize_NestedOneof_TwoLevels_SplitAtOuterTag

**Purpose**: Verify split at outer message tag.

**Message Setup**:
- `nested_oneof` with:
  - `msg_oneof` with `a = 1`, `x = 1`, `b = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<1>` (fits partial tag only)
- Buffer B: `WriteBufferFixedSize<15>` (fits remaining message)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (tag rolled back)
- Second call returns `NO_ERRORS`
- Buffer B contains complete message

---

### 5.3 PartialSerialize_NestedOneof_TwoLevels_SplitDuringInnerOneof

**Purpose**: Verify split during inner oneof field serialization.

**Message Setup**:
- `nested_oneof` with:
  - `msg_oneof` with `a = 1`, `x = 1`, `b = 1`, `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<10>` (fits outer tag+size + partial inner message)
- Buffer B: `WriteBufferFixedSize<20>` (fits remaining message)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Second call returns `NO_ERRORS`
- Collected data equals complete serialized message

---

### 5.4 PartialSerialize_NestedOneof_ThreeLevels

**Purpose**: Verify partial serialization with three levels of nesting (nested_oneof -> message_oneof -> msg_ABC).

**Message Setup**:
- `nested_oneof` with:
  - `msg_oneof` with `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<5>` buffers

**Expected Results**:
- Multiple calls with `BUFFER_FULL`
- Final call returns `NO_ERRORS`
- Collected data equals complete serialized message

---

## 6. Test Cases - String/Bytes Oneof Fields

### 6.1 PartialSerialize_StringOneof_SufficientBuffer

**Purpose**: Verify partial serialization of string field within oneof.

**Message Setup**:
- `string_bytes_oneof<20, 20>` with:
  - `name = "John Doe"`

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x0a, 0x08, 'J', 'o', 'h', 'n', ' ', 'D', 'o', 'e'}` (10 bytes)

---

### 6.2 PartialSerialize_StringOneof_SplitDuringData

**Purpose**: Verify that serialization can be split during string data.

**Message Setup**:
- `string_bytes_oneof<20, 20>` with:
  - `name = "John Doe"`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<5>` (fits tag + size + partial string)
- Buffer B: `WriteBufferFixedSize<10>` (fits remaining string)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x0a, 0x08, 'J', 'o', 'h'}` (tag + size + "Joh")
- Second call returns `NO_ERRORS`
- Buffer B contains `{'n', ' ', 'D', 'o', 'e'}` ("n Doe")

---

### 6.3 PartialSerialize_BytesOneof_SufficientBuffer

**Purpose**: Verify partial serialization of bytes field within oneof.

**Message Setup**:
- `string_bytes_oneof<20, 20>` with:
  - `data = {0x01, 0x02, 0x03, 0x04, 0x05}`

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x12, 0x05, 0x01, 0x02, 0x03, 0x04, 0x05}` (7 bytes)

---

### 6.4 PartialSerialize_BytesOneof_SplitDuringData

**Purpose**: Verify that serialization can be split during bytes data.

**Message Setup**:
- `string_bytes_oneof<20, 20>` with:
  - `data = {0x01, 0x02, 0x03, 0x04, 0x05}`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<4>` (fits tag + size + partial bytes)
- Buffer B: `WriteBufferFixedSize<10>` (fits remaining bytes)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x12, 0x05, 0x01, 0x02}` (tag + size + 2 bytes)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x03, 0x04, 0x05}` (remaining 3 bytes)

---

### 6.5 PartialSerialize_EmptyStringOneof

**Purpose**: Verify partial serialization of empty string within oneof.

**Message Setup**:
- `string_bytes_oneof<20, 20>` with:
  - `name` set but empty (mutable_name().clear())

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x0a, 0x00}` (tag + size 0)

---

## 7. Test Cases - Combined Oneof

### 7.1 PartialSerialize_CombinedOneof_NestedMessage

**Purpose**: Verify partial serialization of combined_oneof with nested message_oneof.

**Message Setup**:
- `combined_oneof` with:
  - `msg_oneof` with `a = 1`, `x = 1`, `b = 1`

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains complete serialized message

---

### 7.2 PartialSerialize_CombinedOneof_StringBytesOneof

**Purpose**: Verify partial serialization of combined_oneof with string_bytes_oneof.

**Message Setup**:
- `combined_oneof` with:
  - `msg_sb_oneof` with `name = "Test"`

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains complete serialized message

---

### 7.3 PartialSerialize_CombinedOneof_SplitDuringNestedOneof

**Purpose**: Verify split during nested oneof within combined_oneof.

**Message Setup**:
- `combined_oneof` with:
  - `msg_oneof` with `a = 1`, `x = 1`, `b = 1`, `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<5>` buffers

**Expected Results**:
- Multiple calls with `BUFFER_FULL`
- Final call returns `NO_ERRORS`
- Collected data equals complete serialized message

---

## 8. Test Cases - Loop and State Management

### 8.1 PartialSerialize_LoopSmallBuffers

**Purpose**: Verify serialization completes correctly when using many small buffers in a loop.

**Message Setup**:
- `message_oneof` with:
  - `a = 1`
  - `x = 1`
  - `b = 1`
  - `v = 1.0`
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<5>` buffers
- Master buffer: `std::array<uint8_t, 50>` to collect all data

**Pseudo-code**:
```cpp
message_oneof msg;
// Set all fields...

std::array<uint8_t, 50> collected_data;
uint32_t total_bytes = 0;
message_oneof::StateStack state;

Error result = Error::BUFFER_FULL;
uint32_t iterations = 0;
constexpr uint32_t MAX_ITERATIONS = 20;

while((Error::BUFFER_FULL == result) && (iterations < MAX_ITERATIONS))
{
  WriteBufferFixedSize<5> small_buffer;
  result = msg.serialize_partial(small_buffer, state.root());
  
  memcpy(&collected_data[total_bytes], small_buffer.get_data(), small_buffer.get_size());
  total_bytes += small_buffer.get_size();
  ++iterations;
}

EXPECT_EQ(Error::NO_ERRORS, result);
// Verify collected_data matches expected bytes
```

**Expected Results**:
- Loop iterates multiple times with `BUFFER_FULL`
- Final iteration returns `NO_ERRORS`
- Collected data equals expected serialized message

---

### 8.2 PartialSerialize_StateReset_SerializeTwice

**Purpose**: Verify that state can be reset and reused for another serialization.

**Message Setup**:
- First: `message_oneof` with `x = 1`
- Second: `message_oneof` with `y = 2`

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- First `serialize_partial()` returns `NO_ERRORS`
- Reset state: `state.reset()`
- Clear buffer: `buffer.clear()`
- Second `serialize_partial()` returns `NO_ERRORS`
- Buffer contains second message data

---

### 8.3 PartialSerialize_StateReset_DifferentOneofSelection

**Purpose**: Verify state reset works when switching between different oneof selections.

**Message Setup**:
- First: `message_oneof` with `msg_ABC` with `varA = 1`
- Second: `message_oneof` with `msg_DEF` with `varD = 2`

**Buffer Setup**:
- `WriteBufferFixedSize<20>`

**Expected Results**:
- First `serialize_partial()` returns `NO_ERRORS`
- Reset state: `state.reset()`
- Clear buffer: `buffer.clear()`
- Second `serialize_partial()` returns `NO_ERRORS`
- Buffer contains msg_DEF data (not msg_ABC)

---

## 9. Test Cases - Error Conditions

### 9.1 PartialSerialize_ZeroByteBuffer

**Purpose**: Verify behavior when buffer has zero capacity.

**Message Setup**:
- `message_oneof` with `x = 1`

**Buffer Setup**:
- `WriteBufferFixedSize<0>` or mock buffer with 0 available

**Expected Results**:
- Returns `BUFFER_FULL`
- State remains in TAG phase
- State is valid for retry with larger buffer

---

### 9.2 PartialSerialize_BufferExactlyOneByteShort

**Purpose**: Verify rollback when buffer is exactly one byte short for nested message.

**Message Setup**:
- `message_oneof` with:
  - `msg_ABC` with `varA = 1`, `varB = 22`, `varC = 333` (10 bytes total)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<9>` (1 byte short)
- Buffer B: `WriteBufferFixedSize<15>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains partial data (tag + size + partial nested data)
- Second call returns `NO_ERRORS`
- Buffer B contains remaining data

---

## 10. Test Cases - Single Oneof Message

### 10.1 PartialSerialize_SingleOneof_Set

**Purpose**: Verify partial serialization of oneof_sigle message with field set.

**Message Setup**:
- `oneof_sigle` with:
  - `single_num = 42`

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x08, 0x2a}` (tag + value 42)

---

### 10.2 PartialSerialize_SingleOneof_NotSet

**Purpose**: Verify partial serialization of oneof_sigle message with no field set.

**Message Setup**:
- `oneof_sigle` (no fields set)

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains 0 bytes (nothing serialized)

---

## 11. Test Implementation Notes

### 11.1 State Setup
Each test should create the appropriate state stack:
- `message_oneof`: `message_oneof::StateStack` (depth 2 due to nested msg_ABC/msg_DEF)
- `nested_oneof`: `nested_oneof::StateStack` (depth 3)
- `string_bytes_oneof`: `string_bytes_oneof<N,M>::StateStack` (depth 1)
- `combined_oneof`: `combined_oneof::StateStack` (depth 3)
- `oneof_sigle`: `oneof_sigle::StateStack` (depth 1)

### 11.2 Buffer Management
Between partial calls:
- Clear the buffer with `buffer.clear()`
- Or use a fresh buffer instance

### 11.3 Verification
- Compare buffer contents byte-by-byte with expected data
- Verify return codes (`NO_ERRORS`, `BUFFER_FULL`)
- For loop tests, verify total byte count matches expected serialized size
- Verify which oneof field is selected using `get_which_xyz()`, `get_which_uvw()`, `get_which_message()`

### 11.4 Expected Data
Reuse expected byte arrays from existing tests where applicable:
- `serialize_ones`
- `serialize_second_oneof`
- `serialize_oneof_msg`

### 11.5 Template Parameters
For `string_bytes_oneof`, remember to specify template parameters:
```cpp
string_bytes_oneof<20, 20> msg;  // <string_max_size, bytes_max_size>
```

## 12. Test Coverage Summary

| Category | Test Cases | Coverage |
|----------|------------|----------|
| Scalar Oneof | 3.1 - 3.8 | Basic scalar oneof serialization, splits, rollback |
| Nested Message Oneof | 4.1 - 4.7 | Nested message in oneof, splits at tag/size/data |
| Two-Level Nesting | 5.1 - 5.4 | nested_oneof message with inner oneof |
| String/Bytes Oneof | 6.1 - 6.5 | String and bytes fields in oneof |
| Combined Oneof | 7.1 - 7.3 | Oneof containing oneof messages |
| Loop/State | 8.1 - 8.3 | Small buffer loops, state reset |
| Error Conditions | 9.1 - 9.2 | Zero buffer, exact size boundary |
| Single Oneof | 10.1 - 10.2 | Simple single-field oneof |

## 13. Edge Cases to Consider

### 13.1 Oneof Selection Changes
- Test behavior when oneof selection changes between serialization calls (should not happen in normal use)

### 13.2 Maximum Nesting Depth
- Test with maximum supported nesting depth through oneof fields
- Verify error handling when depth exceeded

### 13.3 Large Nested Messages
- Nested messages with large repeated fields within oneof
- Nested messages with large string/bytes fields within oneof

### 13.4 Default Values
- Oneof fields set to default values should still serialize
- Verify difference from regular field behavior

### 13.5 State Corruption
- Test behavior with corrupted state
- Verify state reset functionality

## 14. Integration with Existing Tests

The new partial serialization tests should:
- Coexist with existing full serialization tests
- Use similar naming conventions (prefix with `PartialSerialize_`)
- Follow the same code style
- Be placed after existing tests in the file
- Use the same test fixture (`OneofField`)
