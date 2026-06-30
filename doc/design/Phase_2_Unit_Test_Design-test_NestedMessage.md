# Phase 2 Unit Test Design: test_NestedMessage.cpp

This document details the unit tests for partial serialization of nested messages using the test messages from `test/proto/nested_message.proto`.

## 1. Test Message Reference

```protobuf
message message_a 
{
  repeated int32 x   = 1;
  float y   = 2;
  sint64 z  = 3;
}

message message_b 
{
  double u  = 1;
  message_a nested_a = 2;
  int32 v   = 3;
}

message message_c 
{
  message message_d
  {
    repeated uint32 d = 1;
  }

  message_b nested_b = 1;
  message_d nested_d = 2;
  message_e.message_g nested_g = 3;
}
```

## 2. Key Concepts for Nested Messages

### 2.1 Wire Format
Nested messages use LENGTH_DELIMITED wire type (2):
```
[tag][size][nested_message_bytes...]
```

### 2.2 State Machine for Nested Messages

**Serialization:**
1. **TAG phase**: Write tag (field_id << 3 | 2)
2. **SIZE phase**: Calculate nested message size, write size varint
3. **DATA phase**: Delegate to nested message's `serialize_partial` with child state

**Deserialization:**
1. **TAG phase**: Read tag, extract field_id
2. **SIZE phase**: Read size varint, store in `bytes_remaining`
3. **DATA phase**: Use `ReadBufferSection` to limit child's view, delegate to nested message

### 2.3 State Fields Used
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: Field number of nested message
- `size_value`: Total size of nested message
- `bytes_remaining`: Bytes left to read/write in nested message
- `child`: Pointer to nested message's state

## 3. Test Cases

### 3.1 Partial Serialize - Single Nested Message, Sufficient Buffer

**Test Name**: `PartialSerialize_SingleNestedMessage_SufficientBuffer`

**Purpose**: Verify that partial serialization of a message with a single nested message works when buffer is sufficient.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Single `WriteBufferFixedSize<50>` (sufficient for entire message)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains complete serialized message (23 bytes)
- State phase is `COMPLETE`

**Expected Bytes**:
```
{0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, // u
 0x12, 0x0A, // tag and size of nested_a
 0x0A, 0x01, 0x01, // x
 0x15, 0x00, 0x00, 0x80, 0x3F, // y
 0x18, 0x02, // z
 0x18, 0x01} // v
```

### 3.2 Partial Serialize - Nested Message Split at Tag

**Test Name**: `PartialSerialize_NestedMessage_SplitAtTag`

**Purpose**: Verify that serialization can be split between parent fields and nested message tag.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<9>` (fits `u` field only)
- Buffer B: `WriteBufferFixedSize<20>` (fits remaining message)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F}` (u field)
- Second call returns `NO_ERRORS`
- Buffer B contains remaining 14 bytes starting with `{0x12, 0x0A}` (nested_a tag and size)

### 3.3 Partial Serialize - Nested Message Split at Size

**Test Name**: `PartialSerialize_NestedMessage_SplitAtSize`

**Purpose**: Verify that serialization can be split between nested message tag and size.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<10>` (fits `u` + nested_a tag only)
- Buffer B: `WriteBufferFixedSize<20>` (fits remaining message)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x12}`
- Second call returns `NO_ERRORS`
- Buffer B contains remaining 13 bytes starting with `{0x0A}` (nested_a size)

### 3.4 Partial Serialize - Nested Message Split During Data

**Test Name**: `PartialSerialize_NestedMessage_SplitDuringData`

**Purpose**: Verify that serialization can be split during nested message data.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<15>` (fits `u` + nested_a tag+size + part of nested_a data)
- Buffer B: `WriteBufferFixedSize<15>` (fits remaining nested_a data + v field)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x12, 0x0A, 0x0A, 0x01, 0x01, 0x15}`
- Second call returns `NO_ERRORS`
- Buffer B contains remaining 8 bytes: `{0x00, 0x00, 0x80, 0x3F, 0x18, 0x02, 0x18, 0x01}`

### 3.5 Partial Serialize - Deeply Nested Message

**Test Name**: `PartialSerialize_DeeplyNestedMessage`

**Purpose**: Verify partial serialization with multiple levels of nesting.

**Message Setup**:
- `message_c` with:
  - `nested_b` with `u = 1.0`, `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`, `v = 1`
  - `nested_d` with `d = [1, 2]`
  - `nested_g` with `g = 1`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<20>` buffers

**Expected Results**:
- Multiple calls with `BUFFER_FULL`
- Final call returns `NO_ERRORS`
- Collected data equals complete serialized message

### 3.6 Partial Serialize - Nested Message with Large Array

**Test Name**: `PartialSerialize_NestedMessage_LargeArray`

**Purpose**: Verify partial serialization when nested message contains large repeated field.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1, 2, ..., 127]` (127 elements), `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<20>` (fits `u` + nested_a tag+size + partial x array)
- Buffer B: `WriteBufferFixedSize<100>` (fits remaining x array + y + z)
- Buffer C: `WriteBufferFixedSize<10>` (fits v field)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Second call returns `BUFFER_FULL`
- Third call returns `NO_ERRORS`
- All 127 elements correctly serialized

### 3.7 Partial Serialize - Multiple Nested Messages

**Test Name**: `PartialSerialize_MultipleNestedMessages`

**Purpose**: Verify partial serialization with multiple nested messages at same level.

**Message Setup**:
- `message_c` with:
  - `nested_b` with `u = 1.0`, `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`, `v = 1`
  - `nested_d` with `d = [1, 2]`
  - `nested_g` with `g = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<30>` (fits nested_b partially)
- Buffer B: `WriteBufferFixedSize<20>` (fits remaining nested_b + nested_d)
- Buffer C: `WriteBufferFixedSize<10>` (fits nested_g)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Second call returns `BUFFER_FULL`
- Third call returns `NO_ERRORS`
- All nested messages correctly serialized

### 3.8 Partial Serialize - Nested Message Rollback

**Test Name**: `PartialSerialize_NestedMessage_Rollback`

**Purpose**: Verify that when nested message cannot fit entirely, nothing is written (atomic behavior).

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<10>` (fits `u` but not nested_a tag)
- Buffer B: `WriteBufferFixedSize<30>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains only `u` field (9 bytes)
- Second call returns `NO_ERRORS`
- Buffer B contains complete remaining message (14 bytes)

### 3.9 Partial Serialize - Empty Nested Message

**Test Name**: `PartialSerialize_EmptyNestedMessage`

**Purpose**: Verify partial serialization when nested message has no fields set.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` (no fields set)
  - `v = 1`

**Buffer Setup**:
- `WriteBufferFixedSize<20>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains `{0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F, 0x18, 0x01}`
- No bytes for empty nested message

### 3.10 Partial Serialize - State Reuse

**Test Name**: `PartialSerialize_StateReuse`

**Purpose**: Verify that state can be reset and reused for another serialization.

**Message Setup**:
- First: `message_b` with `u = 1.0`, `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`, `v = 1`
- Second: `message_b` with `u = 2.0`, `nested_a` with `x = [2]`, `y = 2.0`, `z = 2`, `v = 2`

**Buffer Setup**:
- `WriteBufferFixedSize<30>`

**Expected Results**:
- First `serialize_partial()` returns `NO_ERRORS`
- Reset state: `state.reset()`
- Clear buffer: `buffer.clear()`
- Second `serialize_partial()` returns `NO_ERRORS`
- Buffer contains second message data

### 3.11 Partial Serialize - Loop Small Buffers

**Test Name**: `PartialSerialize_LoopSmallBuffers`

**Purpose**: Verify serialization completes correctly when using many small buffers in a loop.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<8>` buffers
- Master buffer: `std::array<uint8_t, 50>` to collect all data

**Pseudo-code**:
```cpp
message_b msg;
// Set all fields...

std::array<uint8_t, 50> collected_data;
uint32_t total_bytes = 0;
message_b::StateStack state;

Error result = Error::BUFFER_FULL;
while(Error::BUFFER_FULL == result) 
{
  WriteBufferFixedSize<8> small_buffer;
  result = msg.serialize_partial(small_buffer, state.root());
  
  // Copy to collected buffer
  memcpy(&collected_data[total_bytes], small_buffer.data(), small_buffer.get_size());
  total_bytes += small_buffer.get_size();
}

EXPECT_EQ(Error::NO_ERRORS, result);
EXPECT_EQ(23, total_bytes);
// Compare collected_data with expected 23 bytes
```

**Expected Results**:
- Loop iterates 3 times with `BUFFER_FULL`
- Final iteration returns `NO_ERRORS`
- Collected data equals expected 23 bytes

### 3.12 Partial Serialize - Maximum Values

**Test Name**: `PartialSerialize_MaximumValues`

**Purpose**: Verify partial serialization with maximum field values.

**Message Setup**:
- `message_b` with:
  - `u = std::numeric_limits<double>::max()`
  - `nested_a` with `x = [std::numeric_limits<int32_t>::max()]`, `y = std::numeric_limits<float>::max()`, `z = std::numeric_limits<int64_t>::max()`
  - `v = std::numeric_limits<int32_t>::max()`

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<25>` buffers

**Expected Results**:
- Multiple iterations with `BUFFER_FULL`
- Final result is `NO_ERRORS`
- Collected data equals expected bytes from existing `serialize_max` test

### 3.13 Partial Serialize - Consecutive Small Buffers

**Test Name**: `PartialSerialize_ConsecutiveSmallBuffers`

**Purpose**: Verify that state correctly tracks progress through nested message fields.

**Message Setup**:
- `message_b` with:
  - `u = 1.0`
  - `nested_a` with `x = [1]`, `y = 1.0`, `z = 1`
  - `v = 1`

**Buffer Setup**:
- Four consecutive `WriteBufferFixedSize<6>` buffers

**Expected Results**:
- Call 1: returns `BUFFER_FULL`, buffer has `{0x09, 0x00, 0x00, 0x00, 0x00, 0x00}` (partial u)
- Call 2: returns `BUFFER_FULL`, buffer has `{0xF0, 0x3F, 0x12, 0x0A, 0x0A, 0x01}` (rest of u + nested_a tag+size + partial x)
- Call 3: returns `BUFFER_FULL`, buffer has `{0x01, 0x15, 0x00, 0x00, 0x80, 0x3F}` (rest of x + y)
- Call 4: returns `NO_ERRORS`, buffer has `{0x18, 0x02, 0x18, 0x01}` (z + v)

## 4. Partial Deserialization Test Cases

### 4.1 Partial Deserialize - Clean Split at Tag

**Test Name**: `PartialDeserialize_CleanSplitAtTag`

**Purpose**: Verify deserialization can be split after the tag of nested message.

**Buffer Setup**:
- Buffer 1: Contains `u` field + nested_a tag
- Buffer 2: Contains nested_a size + nested_a data + v field

**Expected Results**:
- First call returns `END_OF_BUFFER`
- Second call returns `NO_ERRORS`
- All fields correctly deserialized

### 4.2 Partial Deserialize - Clean Split at Size

**Test Name**: `PartialDeserialize_CleanSplitAtSize`

**Purpose**: Verify deserialization can be split after the size of nested message.

**Buffer Setup**:
- Buffer 1: Contains `u` field + nested_a tag + size
- Buffer 2: Contains nested_a data + v field

**Expected Results**:
- First call returns `END_OF_BUFFER`
- Second call returns `NO_ERRORS`
- All fields correctly deserialized

### 4.3 Partial Deserialize - Split During Size

**Test Name**: `PartialDeserialize_SplitDuringSize`

**Purpose**: Verify deserialization can be split during multi-byte size varint.

**Message Setup**:
- `message_b` with large `nested_a` (size > 127 bytes)

**Buffer Setup**:
- Buffer 1: Contains `u` + nested_a tag + partial size bytes
- Buffer 2: Contains remaining size bytes + nested_a data + v field

**Expected Results**:
- First call returns `END_OF_BUFFER`
- Second call returns `NO_ERRORS`
- All fields correctly deserialized

### 4.4 Partial Deserialize - Split During Nested Data

**Test Name**: `PartialDeserialize_SplitDuringNestedData`

**Purpose**: Verify deserialization can be split during nested message data.

**Buffer Setup**:
- Buffer 1: Contains `u` + nested_a tag+size + partial nested_a data
- Buffer 2: Contains remaining nested_a data + v field

**Expected Results**:
- First call returns `END_OF_BUFFER`
- Second call returns `NO_ERRORS`
- All fields correctly deserialized

### 4.5 Partial Deserialize - Multiple Nested Levels

**Test Name**: `PartialDeserialize_MultipleNestedLevels`

**Purpose**: Verify deserialization with multiple levels of nesting across buffer boundaries.

**Message Setup**:
- `message_c` with nested_b, nested_d, and nested_g

**Buffer Setup**:
- Multiple buffers splitting at various points

**Expected Results**:
- All nested messages correctly deserialized
- State correctly tracks depth

## 5. Test Implementation Notes

### 5.1 State Setup
Each test should create the appropriate state stack:
- `message_b`: `message_b::StateStack` (depth 2)
- `message_c`: `message_c::StateStack` (depth 3)

### 5.2 Buffer Management
Between partial calls:
- Clear the buffer with `buffer.clear()`
- Or use a fresh buffer instance

### 5.3 Verification
- Compare buffer contents byte-by-byte with expected data
- Verify return codes (`NO_ERRORS`, `BUFFER_FULL`, `END_OF_BUFFER`)
- For loop tests, verify total byte count matches expected serialized size
- Verify deserialized field values match original values

### 5.4 Expected Data
Reuse expected byte arrays from existing tests where applicable:
- `serialize_one`
- `serialize_max`
- `serialize_nested_in_nested_max`

### 5.5 Conditional Compilation
Tests should be wrapped in appropriate preprocessor guards:
```cpp
#ifdef PARTIAL_SERIALIZATION_ENABLED
// Partial serialization tests
#endif

#ifdef PARTIAL_SERIALIZATION_ENABLED
// Partial deserialization tests
#endif
```

## 6. Edge Cases to Consider

### 6.1 Empty Messages
- Parent message with empty nested message
- Empty parent message with empty nested message

### 6.2 Maximum Nesting Depth
- Test with maximum supported nesting depth
- Verify error handling when depth exceeded

### 6.3 Very Large Nested Messages
- Nested messages with large repeated fields
- Nested messages with large string/bytes fields

### 6.4 Error Conditions
- Invalid wire types in nested messages
- Size mismatches
- Unexpected end of buffer

### 6.5 State Corruption
- Test behavior with corrupted state
- Verify state reset functionality

## 7. Integration with Existing Tests

The new partial serialization tests should:
- Coexist with existing full serialization tests
- Use similar naming conventions
- Follow the same code style
- Be disabled by default (using preprocessor guards)
- Be enabled when partial serialization feature is available

## 8. Test Coverage Goals

The test suite should achieve:
- 100% code coverage of partial serialization/deserialization methods
- All state transitions tested
- All error conditions tested
- Various buffer sizes tested
- Various nesting depths tested
- Both successful and error paths tested
