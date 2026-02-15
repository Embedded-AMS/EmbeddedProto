## 15. Phase 2 Unit Test Design: test_string_bytes.cpp

## 15. Phase 2 Unit Test Design: test_string_bytes.cpp

This section details the unit tests for partial serialization of string and bytes fields using messages from `test/proto/string_bytes.proto`.

### 15.1 Test Messages Reference

```protobuf
message text {
  string txt = 1;  // LENGTH_DELIMITED (wire type 2)
}

message raw_bytes {
  bytes b = 1;     // LENGTH_DELIMITED (wire type 2)
}

message string_or_bytes {
  oneof s_or_b {
    string txt = 1;  // LENGTH_DELIMITED
    bytes b = 2;     // LENGTH_DELIMITED
  }
  text nested_text = 3;
  raw_bytes nested_bytes = 4;
}

message repeated_string_bytes {
  repeated string array_of_txt = 1;    // Non-packed LENGTH_DELIMITED
  repeated bytes array_of_bytes = 2;   // Non-packed LENGTH_DELIMITED
  text nested_text = 3;
  raw_bytes nested_bytes = 4;
}
```

### 15.2 Key Concept: Length-Delimited Fields

For string/bytes fields, the wire format is `[tag][size][bytes...]`:
- **TAG phase**: Tag is atomic (must fit completely or rollback)
- **SIZE phase**: Size varint is atomic (must fit completely or rollback to before tag)
- **DATA phase**: Data bytes can be written **incrementally** across multiple buffers

This is the key difference from scalar types: string/bytes data can span multiple buffers while maintaining state via `bytes_remaining`.

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: Field number (1 for txt/b)
- `size_value`: Total size of string/bytes data
- `bytes_remaining`: Bytes left to write in DATA phase

### 15.3 Test Cases

---

#### 15.3.1 PartialSerialize_String_ShortText_SufficientBuffer

**Purpose**: Verify partial serialization of a short string field with sufficient buffer.

**Message**: `text<10>`

**Field Values**:
- `txt = "Foo bar"` (7 characters)

**Buffer Setup**:
- Single `WriteBufferFixedSize<20>` (plenty of space for 9 bytes: tag + size + data)

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS` on first call
- Buffer contains 9 bytes: `{0x0a, 0x07, 'F', 'o', 'o', ' ', 'b', 'a', 'r'}`
  - `0x0a` = tag (field 1, wire type 2)
  - `0x07` = size (7 bytes)
  - Remaining = "Foo bar" ASCII

---

#### 15.3.2 PartialSerialize_String_EmptyString

**Purpose**: Verify empty string is not serialized (default value behavior).

**Message**: `text<10>`

**Field Values**:
- `txt = ""` (empty string, default value)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- `serialize_partial()` returns `NO_ERRORS`
- Buffer contains 0 bytes (empty strings are not serialized per proto3 rules)

---

#### 15.3.3 PartialSerialize_String_SingleChar

**Purpose**: Verify minimum non-empty string serialization.

**Message**: `text<10>`

**Field Values**:
- `txt = "A"` (1 character)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains 3 bytes: `{0x0a, 0x01, 0x41}`
  - `0x0a` = tag
  - `0x01` = size (1 byte)
  - `0x41` = 'A'

---

#### 15.3.4 PartialSerialize_String_BufferTooSmallForTag

**Purpose**: Verify rollback when buffer cannot hold even the tag.

**Message**: `text<10>`

**Field Values**:
- `txt = "Foo bar"`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<0>` (zero capacity)
- Buffer B: `WriteBufferFixedSize<20>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes
- Second call returns `NO_ERRORS`
- Buffer B contains all 9 bytes

---

#### 15.3.5 PartialSerialize_String_BufferOnlyFitsTag

**Purpose**: Verify rollback when tag fits but size varint does not.

**Message**: `text<10>`

**Field Values**:
- `txt = "Foo bar"` (7 chars, size fits in 1 byte)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<1>` (fits tag only, not size)
- Buffer B: `WriteBufferFixedSize<20>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback - tag+size must be atomic)
- Second call returns `NO_ERRORS`
- Buffer B contains all 9 bytes

---

#### 15.3.6 PartialSerialize_String_SplitInData

**Purpose**: Verify string data can span multiple buffers (key partial serialization test).

**Message**: `text<10>`

**Field Values**:
- `txt = "Foo bar"` (7 characters, total 9 bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<5>` (fits tag + size + 3 data bytes)
- Buffer B: `WriteBufferFixedSize<10>` (fits remaining 4 data bytes)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 5 bytes: `{0x0a, 0x07, 'F', 'o', 'o'}`
- State: `phase = DATA`, `bytes_remaining = 4`
- Second call returns `NO_ERRORS`
- Buffer B contains 4 bytes: `{' ', 'b', 'a', 'r'}`

---

#### 15.3.7 PartialSerialize_String_SplitAfterTagSize

**Purpose**: Verify split can occur exactly after tag+size, before any data.

**Message**: `text<10>`

**Field Values**:
- `txt = "Foo bar"` (7 characters)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (fits exactly tag + size)
- Buffer B: `WriteBufferFixedSize<10>` (fits all data)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 2 bytes: `{0x0a, 0x07}`
- State: `phase = DATA`, `bytes_remaining = 7`
- Second call returns `NO_ERRORS`
- Buffer B contains 7 bytes: `{'F', 'o', 'o', ' ', 'b', 'a', 'r'}`

---

#### 15.3.8 PartialSerialize_String_DataOneByteAtATime

**Purpose**: Verify string can be serialized one byte at a time in DATA phase.

**Message**: `text<10>`

**Field Values**:
- `txt = "ABC"` (3 characters, total 5 bytes)

**Buffer Setup**:
- Buffer 1: `WriteBufferFixedSize<2>` (tag + size)
- Buffers 2-4: `WriteBufferFixedSize<1>` each (one data byte each)

**Pseudo-code**:
```cpp
text<10> msg;
msg.mutable_txt() = "ABC";
text<10>::StateStack state;

std::array<uint8_t, 10> collected;
uint32_t total = 0;

// First buffer: tag + size
WriteBufferFixedSize<2> buf1;
auto result = msg.serialize_partial(buf1, state.root());
EXPECT_EQ(BUFFER_FULL, result);
memcpy(&collected[total], buf1.get_data(), buf1.get_size());
total += buf1.get_size();

// Remaining bytes one at a time
for(int i = 0; i < 3; ++i)
{
  WriteBufferFixedSize<1> buf;
  result = msg.serialize_partial(buf, state.root());
  memcpy(&collected[total], buf.get_data(), buf.get_size());
  total += buf.get_size();
}

EXPECT_EQ(NO_ERRORS, result);
EXPECT_EQ(5, total);
```

**Expected Results**:
- Buffers 1-3 return `BUFFER_FULL`
- Buffer 4 returns `NO_ERRORS`
- Collected data: `{0x0a, 0x03, 'A', 'B', 'C'}`

---

#### 15.3.9 PartialSerialize_String_LargeString_MultipleBuffers

**Purpose**: Verify large string serialization across many small buffers.

**Message**: `text<140>`

**Field Values**:
- `txt = "Foo bar "` repeated 20 times (140 characters)

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<20>` buffers
- Collect all output

**Expected Results**:
- Total serialized size: 1 (tag) + 2 (size varint for 140) + 140 (data) = 143 bytes
- Size varint for 140: `{0x8c, 0x01}` (140 = 0x8C in 7-bit encoding)
- Multiple iterations required
- Final result is `NO_ERRORS`
- Collected data starts with `{0x0a, 0x8c, 0x01, 'F', 'o', 'o', ...}`

---

#### 15.3.10 PartialSerialize_String_SizeVarintTwoBytes

**Purpose**: Verify correct handling when size requires 2-byte varint (>127 chars).

**Message**: `text<140>`

**Field Values**:
- `txt` = 140 characters (size requires 2+bytes: `{0x8c, 0x01}`)

**Buffer Setup**:
- `WriteBufferFixedSize<200>` (large enough for all)

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer starts with: `{0x0a, 0x8c, 0x01, ...data...}`
- Total size: 143 bytes

---

#### 15.3.11 PartialSerialize_String_SizeVarintTwoBytes_SplitInSize

**Purpose**: Verify rollback when buffer can hold tag but not complete size varint.

**Message**: `text<140>`

**Field Values**:
- `txt` = 140 characters (size requires 2 bytes)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<2>` (fits tag + 1 byte of size, but size needs 2)
- Buffer B: `WriteBufferFixedSize<200>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback - tag+size must be atomic)
- Second call returns `NO_ERRORS`
- Buffer B contains all 143 bytes

---

#### 15.3.12 PartialSerialize_Bytes_SufficientBuffer

**Purpose**: Verify partial serialization of bytes field with sufficient buffer.

**Message**: `raw_bytes<10>`

**Field Values**:
- `b = {0x01, 0x02, 0x03, 0x00}` (4 bytes)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains 6 bytes: `{0x0a, 0x04, 0x01, 0x02, 0x03, 0x00}`

---

#### 15.3.13 PartialSerialize_Bytes_EmptyBytes

**Purpose**: Verify empty bytes field is not serialized.

**Message**: `raw_bytes<10>`

**Field Values**:
- `b` = empty (0 bytes)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains 0 bytes

---

#### 15.3.14 PartialSerialize_Bytes_SplitInData

**Purpose**: Verify bytes data can span multiple buffers.

**Message**: `raw_bytes<10>`

**Field Values**:
- `b = {0x01, 0x02, 0x03, 0x00}` (4 bytes, total 6 bytes wire format)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<4>` (fits tag + size + 2 data bytes)
- Buffer B: `WriteBufferFixedSize<10>` (fits remaining 2 data bytes)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 4 bytes: `{0x0a, 0x04, 0x01, 0x02}`
- Second call returns `NO_ERRORS`
- Buffer B contains 2 bytes: `{0x03, 0x00}`

---

#### 15.3.15 PartialSerialize_Bytes_BufferTooSmallForTagSize

**Purpose**: Verify rollback when tag+size cannot fit.

**Message**: `raw_bytes<10>`

**Field Values**:
- `b = {0x01, 0x02, 0x03, 0x00}`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<1>` (tag fits, size doesn't)
- Buffer B: `WriteBufferFixedSize<10>` (sufficient)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 0 bytes (rollback)
- Second call returns `NO_ERRORS`
- Buffer B contains all 6 bytes

---

#### 15.3.16 PartialSerialize_Oneof_String_SufficientBuffer

**Purpose**: Verify oneof string field serialization.

**Message**: `string_or_bytes<3, 3, 10, 10>`

**Field Values**:
- `txt = "Foo bar"` (oneof selected)

**Buffer Setup**:
- `WriteBufferFixedSize<20>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains 9 bytes: `{0x0a, 0x07, 'F', 'o', 'o', ' ', 'b', 'a', 'r'}`
- Tag `0x0a` = field 1 (txt), wire type 2

---

#### 15.3.17 PartialSerialize_Oneof_Bytes_SufficientBuffer

**Purpose**: Verify oneof bytes field serialization.

**Message**: `string_or_bytes<3, 3, 10, 10>`

**Field Values**:
- `b = {0x01, 0x02, 0x03, 0x00}` (oneof selected)

**Buffer Setup**:
- `WriteBufferFixedSize<20>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains 6 bytes: `{0x12, 0x04, 0x01, 0x02, 0x03, 0x00}`
- Tag `0x12` = field 2 (b), wire type 2

---

#### 15.3.18 PartialSerialize_Oneof_String_SplitInData

**Purpose**: Verify oneof string can span multiple buffers.

**Message**: `string_or_bytes<3, 3, 10, 10>`

**Field Values**:
- `txt = "Foo bar"`

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<5>`
- Buffer B: `WriteBufferFixedSize<10>`

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 5 bytes: `{0x0a, 0x07, 'F', 'o', 'o'}`
- Second call returns `NO_ERRORS`
- Buffer B contains 4 bytes: `{' ', 'b', 'a', 'r'}`

---

#### 15.3.19 PartialSerialize_RepeatedString_ThreeStrings_LargeBuffer

**Purpose**: Verify repeated string field serialization with sufficient buffer.

**Message**: `repeated_string_bytes<3, 15, 3, 15, 3, 3>`

**Field Values**:
- `array_of_txt[0] = "Foo bar 1"` (9 chars)
- `array_of_txt[1] = ""` (empty)
- `array_of_txt[2] = "Foo bar 3"` (9 chars)

**Buffer Setup**:
- `WriteBufferFixedSize<50>`

**Expected Results**:
- Returns `NO_ERRORS`
- Expected wire format (24 bytes):
  - String 1: `{0x0a, 0x09, "Foo bar 1"}` (11 bytes)
  - String 2: `{0x0a, 0x00}` (2 bytes - empty string with tag)
  - String 3: `{0x0a, 0x09, "Foo bar 3"}` (11 bytes)

---

#### 15.3.20 PartialSerialize_RepeatedString_SplitBetweenElements

**Purpose**: Verify split occurs cleanly between repeated string elements.

**Message**: `repeated_string_bytes<3, 15, 3, 15, 3, 3>`

**Field Values**:
- `array_of_txt[0] = "Foo bar 1"` (11 bytes wire)
- `array_of_txt[1] = "Foo bar 2"` (11 bytes wire)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<11>` (exactly fits first element)
- Buffer B: `WriteBufferFixedSize<20>` (fits second element)

**Expected Results**:
- First call returns `BUFFER_FULL`
- Buffer A contains 11 bytes: `{0x0a, 0x09, "Foo bar 1"}`
- Second call returns `NO_ERRORS`
- Buffer B contains 11 bytes: `{0x0a, 0x09, "Foo bar 2"}`

---

#### 15.3.21 PartialSerialize_RepeatedString_SplitWithinElement

**Purpose**: Verify split can occur within a repeated string element's data.

**Message**: `repeated_string_bytes<3, 15, 3, 15, 3, 3>`

**Field Values**:
- `array_of_txt[0] = "Foo bar 1"` (9 chars)
- `array_of_txt[1] = "Foo bar 2"` (9 chars)

**Buffer Setup**:
- Buffer A: `WriteBufferFixedSize<6>` (tag + size + 4 chars of first string)
- Buffer B: `WriteBufferFixedSize<10>` (remaining 5 chars + partial second)
- Buffer C: `WriteBufferFixedSize<20>` (rest)

**Expected Results**:
- First call: `BUFFER_FULL`, Buffer A has `{0x0a, 0x09, 'F', 'o', 'o', ' '}`
- Second call: `BUFFER_FULL`, Buffer B continues
- Final call: `NO_ERRORS`
- Total collected = 22 bytes

---

#### 15.3.22 PartialSerialize_RepeatedBytes_ThreeArrays_LargeBuffer

**Purpose**: Verify repeated bytes field serialization.

**Message**: `repeated_string_bytes<3, 15, 3, 15, 3, 3>`

**Field Values**:
- `array_of_bytes[0] = {0x01, 0x02}` (2 bytes)
- `array_of_bytes[1] = {0x03, 0x04, 0x05}` (3 bytes)
- `array_of_bytes[2] = {0x06}` (1 byte)

**Buffer Setup**:
- `WriteBufferFixedSize<20>`

**Expected Results**:
- Returns `NO_ERRORS`
- Expected wire format:
  - `{0x12, 0x02, 0x01, 0x02}` (4 bytes)
  - `{0x12, 0x03, 0x03, 0x04, 0x05}` (5 bytes)
  - `{0x12, 0x01, 0x06}` (3 bytes)
- Total: 12 bytes

---

#### 15.3.23 PartialSerialize_String_StateReset_SerializeTwice

**Purpose**: Verify state can be reset and reused for string fields.

**Message**: `text<10>`

**Field Values**:
- `txt = "Test"`

**Buffer Setup**:
- Two `WriteBufferFixedSize<10>` buffers

**Expected Results**:
- First `serialize_partial()` returns `NO_ERRORS`
- Reset state and clear buffer
- Second `serialize_partial()` returns `NO_ERRORS`
- Both buffers contain identical data: `{0x0a, 0x04, 'T', 'e', 's', 't'}`

---

#### 15.3.24 PartialSerialize_String_VerifyBytesRemainingTracking

**Purpose**: Verify `bytes_remaining` in state correctly tracks progress.

**Message**: `text<20>`

**Field Values**:
- `txt = "1234567890"` (10 characters)

**Buffer Setup**:
- Buffer 1: `WriteBufferFixedSize<2>` (tag + size only)
- Buffer 2: `WriteBufferFixedSize<4>` (4 data bytes)
- Buffer 3: `WriteBufferFixedSize<10>` (remaining 6 data bytes)

**Verification Points**:
- After Buffer 1: `state.phase == DATA`, `state.bytes_remaining == 10`
- After Buffer 2: `state.phase == DATA`, `state.bytes_remaining == 6`
- After Buffer 3: `state.phase == COMPLETE` or next field, `state.bytes_remaining == 0`

**Expected Results**:
- Buffer 1: `{0x0a, 0x0a}` (tag + size=10), returns `BUFFER_FULL`
- Buffer 2: `{'1', '2', '3', '4'}`, returns `BUFFER_FULL`
- Buffer 3: `{'5', '6', '7', '8', '9', '0'}`, returns `NO_ERRORS`

---

#### 15.3.25 PartialSerialize_String_LoopSmallBuffers

**Purpose**: Verify string serialization completes correctly with many small buffers.

**Message**: `text<100>`

**Field Values**:
- `txt = "The quick brown fox jumps over the lazy dog"` (43 characters)

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<8>` buffers
- Collect all output

**Pseudo-code**:
```cpp
text<100> msg;
msg.mutable_txt() = "The quick brown fox jumps over the lazy dog";
text<100>::StateStack state;

std::array<uint8_t, 100> collected;
uint32_t total = 0;

EmbeddedProto::Error result = EmbeddedProto::Error::BUFFER_FULL;
while(EmbeddedProto::Error::BUFFER_FULL == result)
{
  WriteBufferFixedSize<8> buf;
  result = msg.serialize_partial(buf, state.root());
  memcpy(&collected[total], buf.get_data(), buf.get_size());
  total += buf.get_size();
}

EXPECT_EQ(NO_ERRORS, result);
EXPECT_EQ(45, total);  // tag(1) + size(1) + data(43)
```

**Expected Results**:
- Multiple iterations with `BUFFER_FULL`
- Final returns `NO_ERRORS`
- Total bytes: 45 (1 tag + 1 size + 43 data)

---

#### 15.3.26 PartialSerialize_Bytes_WithZeroBytes

**Purpose**: Verify bytes field containing zero values serializes correctly.

**Message**: `raw_bytes<10>`

**Field Values**:
- `b = {0x00, 0x00, 0x00}` (3 zero bytes)

**Buffer Setup**:
- `WriteBufferFixedSize<10>`

**Expected Results**:
- Returns `NO_ERRORS`
- Buffer contains 5 bytes: `{0x0a, 0x03, 0x00, 0x00, 0x00}`

---

#### 15.3.27 PartialSerialize_String_MaxLength_SplitMultipleTimes

**Purpose**: Verify string at maximum template length serializes correctly.

**Message**: `text<10>`

**Field Values**:
- `txt = "1234567890"` (10 characters, at max length)

**Buffer Setup**:
- Loop with `WriteBufferFixedSize<3>` buffers

**Expected Results**:
- Multiple iterations required
- Total: 12 bytes (tag + size + 10 data)
- Final returns `NO_ERRORS`

---

### 15.4 Test Implementation Notes

1. **State Setup**: Use `text<N>::StateStack` or `raw_bytes<N>::StateStack` for messages. These have `STATE_DEPTH = 1` since they contain no nested messages.

2. **Buffer Management**: Between partial calls:
   - For loop tests: create fresh buffer each iteration
   - Collect data into master array for verification

3. **Key Verifications for String/Bytes**:
   - Verify `bytes_remaining` decrements correctly
   - Verify data continuity across buffers
   - Verify tag+size atomic behavior (rollback if incomplete)
   - Verify data can split at any byte boundary

4. **Expected Data Calculation**:
   - Tag: `(field_number << 3) | 2` where wire type 2 = LENGTH_DELIMITED
   - Size: varint encoding of string/bytes length
   - Data: raw bytes

5. **Difference from Scalar Tests**:
   - Scalars: tag+value atomic (all or nothing)
   - String/Bytes: tag+size atomic, but data can span buffers

6. **Conditional Compilation**: Tests should be wrapped in:
   ```cpp
   #ifdef PARTIAL_SERIALIZATION_ENABLED
   // tests here
   #endif
   ```

---

