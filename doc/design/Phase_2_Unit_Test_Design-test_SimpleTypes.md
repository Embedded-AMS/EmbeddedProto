## 14. Phase 2 Unit Test Design: test_SimpleTypes.cpp

This section details the unit tests for partial serialization of simple/scalar types using the `Test_Simple_Types` message from `test/proto/simple_types.proto`.

### 14.1 Test Message Reference

```protobuf
message Test_Simple_Types {
  int32       a_int32     = 1;   // VARINT
  int64       a_int64     = 2;   // VARINT
  uint32      a_uint32    = 3;   // VARINT
  uint64      a_uint64    = 4;   // VARINT
  sint32      a_sint32    = 5;   // VARINT
  sint64      a_sint64    = 6;   // VARINT
  bool        a_bool      = 7;   // VARINT
  Test_Enum   a_enum      = 8;   // VARINT
  fixed64     a_fixed64   = 9;   // FIXED64 (8 bytes)
  sfixed64    a_sfixed64  = 10;  // FIXED64 (8 bytes)
  double      a_double    = 11;  // FIXED64 (8 bytes)
  fixed32     a_fixed32   = 12;  // FIXED32 (4 bytes)
  sfixed32    a_sfixed32  = 13;  // FIXED32 (4 bytes)
  float       a_float     = 14;  // FIXED32 (4 bytes)
  Nested_Enum a_nested_enum = 15; // VARINT
}
```

### 14.2 Key Concept: Atomic Fields

For scalar types (varint and fixed), the tag+value pair is **atomic**:
- If the buffer cannot hold the complete tag+value, nothing is written and `BUFFER_FULL` is returned
- The state remains at the beginning of the field (TAG phase)
- On retry with a fresh buffer, the entire tag+value is written

This is in contrast to string/bytes/nested messages where data can span multiple buffers.

### 14.3 Test Cases

---

#### 14.3.1 PartialSerialize_SingleVarintField_SufficientBuffer

**Purpose**: Verify that partial serialization of a single varint field works when buffer is sufficient.

**Field Values**:
- `a_int32 = 1`

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>` (plenty of space for tag + value = 2 bytes)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains `{0x08, 0x01}` (2 bytes)
- State phase is `COMPLETE` or ready for next field

---

#### 14.3.2 PartialSerialize_SingleFixed64Field_SufficientBuffer

**Purpose**: Verify partial serialization of a fixed64 field (9 bytes total: 1 tag + 8 data).

**Field Values**:
- `a_fixed64 = 1`

**Buffer Setup**:
- Single `WriteBufferFixedSize<10>` (sufficient for 9 bytes)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains `{0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}` (9 bytes)

---

#### 14.3.3 PartialSerialize_AllFieldsOne_LargeBuffer

**Purpose**: Verify `serialize_partial()` produces identical output to `serialize()` when buffer is large enough.

**Field Values**:
- All fields set to 1 (same as existing `serialize_one` test)

**Buffer Setup**:
- Single `WriteBufferFixedSize<100>` (sufficient for all 60 bytes)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains identical 60 bytes as existing `serialize_one` test:
  ```
  {0x08, 0x01, 0x10, 0x01, 0x18, 0x01, 0x20, 0x01, 0x28, 0x02, 
   0x30, 0x02, 0x38, 0x01, 0x40, 0x01, 
   0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x51, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
   0x59, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f, 
   0x65, 0x01, 0x00, 0x00, 0x00, 
   0x6d, 0x01, 0x00, 0x00, 0x00, 
   0x75, 0x00, 0x00, 0x80, 0x3f,
   0x78, 0x01}
  ```

---

#### 14.3.4 PartialSerialize_AllFieldsOne_TwoBuffers_CleanSplit

**Purpose**: Verify serialization can be split across two buffers at a clean field boundary.

**Field Values**:
- All fields set to 1 (60 bytes total)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<16>` (fits first 8 varint fields = 16 bytes)
- Buffer B: `WriteBufferFixedSize<50>` (fits remaining fields)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains first 16 bytes: `{0x08, 0x01, 0x10, 0x01, 0x18, 0x01, 0x20, 0x01, 0x28, 0x02, 0x30, 0x02, 0x38, 0x01, 0x40, 0x01}`
- Second call with Buffer B returns `NO_ERRORS`
- Buffer B contains remaining 44 bytes

---

#### 14.3.5 PartialSerialize_Fixed64_BufferTooSmall_Rollback

**Purpose**: Verify that when a fixed64 field cannot fit entirely, nothing is written (atomic behavior).

**Field Values**:
- `a_fixed64 = 1` (requires 9 bytes: tag 0x49 + 8 data bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<5>` (too small for 9 bytes)
- Buffer B: `WriteBufferFixedSize<10>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback - nothing written)
- Second call returns `NO_ERRORS`
- Buffer B contains all 9 bytes: `{0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}`

---

#### 14.3.6 PartialSerialize_LargeVarint_BufferTooSmall_Rollback

**Purpose**: Verify that a large varint (up to 10 bytes) triggers rollback when buffer is insufficient.

**Field Values**:
- `a_uint64 = UINT64_MAX` (requires 11 bytes: tag 1 byte + value 10 bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<6>` (too small for 11 bytes)
- Buffer B: `WriteBufferFixedSize<15>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback)
- Second call returns `NO_ERRORS`
- Buffer B contains 11 bytes for field 4: `{0x20, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}`

---

#### 14.3.7 PartialSerialize_TwoVarintFields_SplitBetweenFields

**Purpose**: Verify that split happens cleanly between two varint fields.

**Field Values**:
- `a_int32 = 1` (2 bytes)
- `a_int64 = 1` (2 bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (exactly fits first field)
- Buffer B: `WriteBufferFixedSize<5>` (fits second field)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (first field complete)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x10, 0x01}` (second field complete)

---

#### 14.3.8 PartialSerialize_TwoVarintFields_SecondDoesNotFit

**Purpose**: Verify rollback when first field fits but second field does not.

**Field Values**:
- `a_int32 = 1` (2 bytes)
- `a_int64 = 1` (2 bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<3>` (fits first field, but only 1 byte left - not enough for second)
- Buffer B: `WriteBufferFixedSize<5>` (fits second field)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x08, 0x01}` (first field only, second rolled back)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x10, 0x01}` (second field)

---

#### 14.3.9 PartialSerialize_LoopSmallBuffers

**Purpose**: Verify serialization completes correctly when using many small buffers in a loop.

**Field Values**:
- All fields set to 1 (60 bytes total)

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<12>` buffers
- Master buffer: `std::array<uint8_t, 100>` to collect all data

**Pseudo-code**:
```cpp
Test_Simple_Types msg;
// Set all fields to 1...

std::array<uint8_t, 100> collected_data;
uint32_t total_bytes = 0;
Test_Simple_Types::StateStack state;

EmbeddedProto::Error result = EmbeddedProto::Error::BUFFER_FULL;
while(EmbeddedProto::Error::BUFFER_FULL == result) 
{
  WriteBufferFixedSize<12> small_buffer;
  result = msg.serialize_partial(small_buffer, state.root());
  
  // Copy to collected buffer
  memcpy(&collected_data[total_bytes], small_buffer.data(), small_buffer.get_size());
  total_bytes += small_buffer.get_size();
}

EXPECT_EQ(EmbeddedProto::Error::NO_ERRORS, result);
EXPECT_EQ(60, total_bytes);
// Compare collected_data with expected 60 bytes
```

**Expected Results**:
- Loop iterates multiple times with `BUFFER_FULL`
- Final iteration returns `NO_ERRORS`
- Collected data equals expected 60 bytes

---

#### 14.3.10 PartialSerialize_AllFieldsMax_LoopSmallBuffers

**Purpose**: Verify partial serialization works with maximum field values (100 bytes total).

**Field Values**:
- All fields set to max values (same as existing `serialize_max` test)

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<20>` buffers

**Expected Results**:
- Multiple iterations required
- Final result is `NO_ERRORS`
- Collected data equals expected 100 bytes from `serialize_max` test

---

#### 14.3.11 PartialSerialize_EmptyMessage

**Purpose**: Verify partial serialization of an empty message (no fields set).

**Field Values**:
- No fields set (default values)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains 0 bytes
- No calls to `BUFFER_FULL`

---

#### 14.3.12 PartialSerialize_SingleBool_MinimumBuffer

**Purpose**: Verify minimum buffer size (2 bytes) is sufficient for bool field.

**Field Values**:
- `a_bool = true` (2 bytes: tag + value)

**Buffer Setup**:
- `WriteBufferFixedSize<2>` (exactly 2 bytes)

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains `{0x38, 0x01}`

---

#### 14.3.13 PartialSerialize_Fixed32_ExactFit

**Purpose**: Verify fixed32 field (5 bytes) with exactly fitting buffer.

**Field Values**:
- `a_fixed32 = 1` (5 bytes: tag 0x65 + 4 data bytes)

**Buffer Setup**:
- `WriteBufferFixedSize<5>` (exactly 5 bytes)

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains `{0x65, 0x01, 0x00, 0x00, 0x00}`

---

#### 14.3.14 PartialSerialize_Fixed32_BufferOneByteTooSmall

**Purpose**: Verify fixed32 field with buffer one byte too small triggers rollback.

**Field Values**:
- `a_fixed32 = 1` (5 bytes required)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<4>` (1 byte too small)
- Buffer B: `WriteBufferFixedSize<10>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback)
- Second call returns `NO_ERRORS`
- Buffer B contains all 5 bytes

---

#### 14.3.15 PartialSerialize_StateReset_SerializeTwice

**Purpose**: Verify state can be reset and reused for another serialization.

**Field Values**:
- `a_int32 = 42`

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- First `serialize_partial()` returns `NO_ERRORS`, buffer has data
- Reset state: `state.reset()`
- Clear buffer: `buffer.clear()`
- Second `serialize_partial()` returns `NO_ERRORS`
- Buffer contains identical data as first call

---

#### 14.3.16 PartialSerialize_Error_ZeroByteBuffer

**Purpose**: Verify behavior when buffer has zero capacity.

**Field Values**:
- `a_int32 = 1`

**Buffer Setup**:
- `WriteBufferFixedSize<0>` or mock buffer with 0 available

**Expected Results**:
- Returns `BUFFER_FULL`
- State remains in TAG phase for field 1
- State is valid for retry with larger buffer

---

#### 14.3.17 PartialSerialize_MixedFields_VarintThenFixed

**Purpose**: Verify transition from varint field to fixed field across buffer boundary.

**Field Values**:
- `a_bool = true` (field 7, 2 bytes)
- `a_fixed64 = 1` (field 9, 9 bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (fits bool only)
- Buffer B: `WriteBufferFixedSize<15>` (fits fixed64)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains `{0x38, 0x01}` (bool field)
- Second call returns `NO_ERRORS`
- Buffer B contains `{0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}` (fixed64 field)

---

#### 14.3.18 PartialSerialize_EnumField_LargeValue

**Purpose**: Verify enum with large value (multi-byte varint).

**Field Values**:
- `a_enum = Test_Enum::TWOBILLION` (2000000000, 6 bytes: tag 1 + value 5)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains `{0x40, 0x80, 0xA8, 0xD6, 0xB9, 0x07}` (6 bytes)

---

#### 14.3.19 PartialSerialize_NegativeInt32_Rollback

**Purpose**: Verify rollback with negative int32 (10 bytes due to sign extension).

**Field Values**:
- `a_int32 = -1` (requires 6 bytes when encoded: tag + 5 varint bytes for negative)

Note: In protobuf, negative int32 values are sign-extended to 64-bit, requiring up to 10 bytes for the value.

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<5>` (too small)
- Buffer B: `WriteBufferFixedSize<15>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback)
- Second call returns `NO_ERRORS`

---

#### 14.3.20 PartialSerialize_ConsecutiveSmallBuffers_VerifyProgress

**Purpose**: Verify that state correctly tracks progress through multiple varint fields.

**Field Values**:
- `a_int32 = 1` (2 bytes)
- `a_int64 = 1` (2 bytes)
- `a_uint32 = 1` (2 bytes)
- `a_uint64 = 1` (2 bytes)

**Buffer Setup**:
- Four consecutive `WriteBufferFixedSize<2>` buffers (each holds exactly one field)

**Expected Results**:
- Call 1: returns `BUFFER_FULL`, buffer has `{0x08, 0x01}`
- Call 2: returns `BUFFER_FULL`, buffer has `{0x10, 0x01}`
- Call 3: returns `BUFFER_FULL`, buffer has `{0x18, 0x01}`
- Call 4: returns `NO_ERRORS`, buffer has `{0x20, 0x01}`

---

### 14.4 Test Implementation Notes

1. **State Setup**: Each test should create a `Test_Simple_Types::StateStack` (or `MessageStateStack<1>` if not generated) and use `state.root()`.

2. **Buffer Management**: Between partial calls, clear the buffer with `buffer.clear()` or use a fresh buffer.

3. **Verification**: 
   - Compare buffer contents byte-by-byte with expected data
   - Verify return codes (`NO_ERRORS`, `BUFFER_FULL`)
   - For loop tests, verify total byte count matches expected serialized size

4. **Expected Data**: Reuse expected byte arrays from existing `serialize_one`, `serialize_max`, `serialize_min` tests where applicable.

5. **Conditional Compilation**: Tests may need to be wrapped in `#ifdef PARTIAL_SERIALIZATION_ENABLED` or similar preprocessor guard.

---

---