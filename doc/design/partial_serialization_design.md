# Partial (De)serialization - Detailed Design

## Document Information
- **Version**: 0.1 (Draft)
- **Status**: Under Review
- **Last Updated**: Session 3 - Detailed Design Phase
- **Related**: `doc/design/v4_high_level_specification.md` Section 4.1

---

## 1. Executive Summary

This document details the design for Partial (De)serialization in EmbeddedProto v4. This feature enables serialization and deserialization of messages in chunks using small buffers, critical for:
- CAN bus communication (8 bytes per frame)
- CAN-FD (64 bytes per frame)
- Multitasking environments (yield CPU between chunks)
- Memory-constrained systems

---

## 2. Design Principles

### 2.1 Core Principles
1. **External State** - State stored outside message objects to avoid per-message RAM overhead
2. **Compile-Time Depth** - Maximum nesting depth known at compile time via generated code
3. **Minimal Buffer Size** - Support buffers as small as 10-20 bytes
4. **Backward Compatible** - Existing `serialize()`/`deserialize()` APIs unchanged
5. **Opt-in Usage** - Users who don't use partial methods have zero overhead

### 2.2 Key Decisions from Design Sessions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| State ownership | User-allocated template | Compile-time depth, no dynamic allocation |
| State fields | Single struct with all fields | Simpler than union, minimal overhead |
| Field tracking | `field_id` (proto field number) | Consistent between serialization and deserialization |
| Buffer rollback | Update buffer only on success | No need for push/pop API |
| Error handling | No `last_error` in state | Caller tracks errors, simpler design |
| Feature flag | Always enabled | Unused code optimized out by compiler |

---

## 3. State Structure

### 3.1 Phase Enumeration

The `Phase` enum tracks the current operation within a field's serialization/deserialization:

```cpp
namespace EmbeddedProto 
{

enum class Phase : uint8_t 
{
    TAG = 0,       // Reading/writing field tag (field number + wire type)
    SIZE = 1,      // Reading/writing length prefix (for LENGTH_DELIMITED fields)
    DATA = 2,      // Reading/writing actual field data
    COMPLETE = 3   // Done with this field, ready for next
};

} // namespace EmbeddedProto
```

### 3.2 Message State Structure

A single state structure is used for both serialization and deserialization:

```cpp
namespace EmbeddedProto 
{

class MessageState 
{
  public:
    //! Current phase of field processing
    Phase phase = Phase::TAG;
    
    //! Field number from protobuf definition (1-based, from tag or next to serialize)
    uint32_t field_id = 0;
    
    //! Wire type from tag (deserialization only, but stored here for simplicity)
    WireFormatter::WireType wire_type = WireFormatter::WireType::VARINT;
    
    //! For repeated fields: index of current element (0-based)
    uint32_t element_index = 0;
    
    //! For length-delimited fields: bytes remaining to read/write
    uint32_t bytes_remaining = 0;
    
    //! For length-delimited fields: size value once calculated/read
    uint32_t size_value = 0;
    
    //! Pointer to child state for nested messages (null if leaf)
    MessageState* child = nullptr;
    
    /**
     * \brief Reset state to initial values
     */
    void reset() 
    {
      phase = Phase::TAG;
      field_id = 0;
      wire_type = WireFormatter::WireType::VARINT;
      element_index = 0;
      bytes_remaining = 0;
      size_value = 0;
      // Note: child pointer is not reset, it's set at construction
    }
    
  protected:
    MessageState() = default;
    ~MessageState() = default;
};

} // namespace EmbeddedProto
```

### 3.3 State Template with Fixed Array

The user instantiates a state template with the required depth:

```cpp
namespace EmbeddedProto 
{

template<uint32_t DEPTH>
class MessageStateStack 
{
    static_assert(DEPTH >= 1, "Depth must be at least 1");
    
  public:
    MessageStateStack() 
    {
      // Link states together: parent -> child
      // states_[0] is the root (outermost message)
      // states_[DEPTH-1] is the deepest possible nested message
      for(uint32_t i = 0; i < DEPTH - 1; ++i) 
      {
        states_[i].child = &states_[i + 1];
      }
      // states_[DEPTH-1].child remains nullptr (leaf)
    }
    
    /**
     * \brief Get the root state (for the outermost message)
     */
    MessageState& root() { return states_[0]; }
    
    /**
     * \brief Get the state at a specific depth
     * \param depth 0 = root, 1 = first nested level, etc.
     */
    MessageState& at(uint32_t depth) 
    { 
      assert(depth < DEPTH);
      return states_[depth]; 
    }
    
    /**
     * \brief Reset all states to initial values
     */
    void reset() 
    {
      for (uint32_t i = 0; i < DEPTH; ++i)
      {
        states_[i].reset();
      }
    }
    
    /**
     * \brief Get the maximum depth this stack can handle
     */
    static constexpr uint32_t max_depth() { return DEPTH; }
    
  private:
    MessageState states_[DEPTH];
};

} // namespace EmbeddedProto
```

### 3.4 Generated State Typedef

The Python code generator calculates the required depth and provides a convenient typedef:

```cpp
// Generated in the message header file
// For message OuterMsg with nesting: OuterMsg -> InnerMsg -> DeepestMsg
// Depth = 3

using SerializationState_OuterMsg = EmbeddedProto::MessageStateStack<3>;
using DeserializationState_OuterMsg = EmbeddedProto::MessageStateStack<3>;
```

---

## 4. Depth Calculation

### 4.1 Algorithm

The depth is the maximum nesting level of messages within messages:

```python
# In TypeDefinitions.py (MessageDefinition class)

def get_state_depth(self):
    """
    Calculate the required state depth for this message.
    
    Returns:
        int: Minimum 1, plus maximum depth of nested message fields.
    
    Examples:
        - message Simple { int32 a = 1; } -> depth = 1
        - message A { B b = 1; } where B has no nested -> depth = 2
        - message A { B b = 1; } where B { C c = 1; } -> depth = 3
    """
    max_nested_depth = 0
    
    for field in self.fields:
        if isinstance(field, FieldMessage):
            nested_depth = field.definition.get_state_depth()
            if nested_depth > max_nested_depth:
                max_nested_depth = nested_depth
    
    return 1 + max_nested_depth  # +1 for this message itself
```

### 4.2 Examples

| Message Structure | Depth |
|-------------------|-------|
| `Simple { int32 a = 1; }` | 1 |
| `A { B b = 1; }` where `B { int32 x = 1; }` | 2 |
| `A { B b = 1; }` where `B { C c = 1; }` and `C { int32 x = 1; }` | 3 |
| `A { B b1 = 1; C c = 2; }` where B depth=2, C depth=1 | 3 (max of B, C) |

---

## 5. Serialization State Machine

### 5.1 Overview

Serialization follows a state machine per field:

```
┌─────────────────────────────────────────────────────────────────┐
│                         TAG Phase                               │
│  - Write tag: (field_id << 3) | wire_type                       │
│  - If LENGTH_DELIMITED: → SIZE phase                            │
│  - Else: → DATA phase                                           │
│  - If buffer full before complete: return BUFFER_FULL           │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         SIZE Phase                              │
│  - Calculate/write size varint (for strings, bytes, messages)   │
│  - For packed repeated: calculate total packed size             │
│  - If buffer full: return BUFFER_FULL                           │
│  - → DATA phase                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         DATA Phase                              │
│  - For scalars: write value (varint or fixed)                   │
│  - For strings/bytes: write bytes, track bytes_remaining        │
│  - For messages: recurse with child state                       │
│  - For repeated: track element_index, loop through elements     │
│  - If buffer full: return BUFFER_FULL                           │
│  - → next field (TAG phase) or COMPLETE                         │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Field Type Serialization Details

#### 5.2.1 Scalar Fields (int32, uint32, sint32, bool, enum, fixed32, float, etc.)

**Wire Format**: `[tag][value]`

**State Machine**:
1. **TAG phase**: Write tag, transition to DATA
2. **DATA phase**: Write value (varint or fixed bytes), transition to COMPLETE

**State Fields Used**:
- `phase`: TAG → DATA → COMPLETE
- `field_id`: Field number

**Partial Behavior**:
- Tag and value can be in different buffers
- If buffer fills during tag write, nothing written, return BUFFER_FULL
- If buffer fills during value write, rollback tag, return BUFFER_FULL

#### 5.2.2 String and Bytes Fields

**Wire Format**: `[tag][size][bytes...]`

**State Machine**:
1. **TAG phase**: Write tag, transition to SIZE
2. **SIZE phase**: Write size varint, store in `size_value`, transition to DATA
3. **DATA phase**: Write bytes, decrement `bytes_remaining`, transition to COMPLETE when done

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: Field number
- `size_value`: Total size (calculated once in SIZE phase)
- `bytes_remaining`: Bytes left to write (in DATA phase)

**Partial Behavior**:
- Tag, size, and data can all be in different buffers
- `bytes_remaining` tracks progress within DATA phase

#### 5.2.3 Nested Message Fields

**Wire Format**: `[tag][size][nested message bytes...]`

**State Machine**:
1. **TAG phase**: Write tag, transition to SIZE
2. **SIZE phase**: Calculate nested message size, write size varint, store in `size_value`, transition to DATA
3. **DATA phase**: Delegate to nested message's `serialize_partial` with `child` state

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: Field number
- `size_value`: Nested message size (calculated once in SIZE phase)
- `bytes_remaining`: Bytes left to write (tracks progress in DATA phase)
- `child`: Pointer to nested message's state

**Partial Behavior**:
- Nested message serialization uses child state
- Parent tracks `bytes_remaining` as the number of bytes still to be written for the nested message
- On first entry to DATA phase: `bytes_remaining = size_value`
- Each call to child's `serialize_partial` may write partial data
- After child returns:
  - Calculate bytes written: `buffer.get_size() - initial_buffer_size`
  - Decrement `bytes_remaining` by bytes written
  - If `bytes_remaining > 0` and child returned `BUFFER_FULL`: return `BUFFER_FULL`
  - If `bytes_remaining == 0`: nested message complete, transition to next field
- When child's `phase == COMPLETE`: parent transitions to next field (TAG phase)

**Pseudocode for DATA phase**:
```cpp
case Phase::DATA:
{
  if(state.bytes_remaining == 0)
  {
    // First time entering DATA phase for this nested message
    state.bytes_remaining = state.size_value;
  }
  
  const uint32_t initial_buffer_size = buffer.get_size();
  
  // Delegate to nested message
  Error err = nested_message.serialize_partial(buffer, *state.child);
  
  if(Error::BUFFER_FULL == err)
  {
    // Calculate how many bytes were actually written
    const uint32_t bytes_written = buffer.get_size() - initial_buffer_size;
    state.bytes_remaining -= bytes_written;
    return Error::BUFFER_FULL;
  }
  else if(Error::NO_ERRORS == err)
  {
    // Nested message fully serialized
    state.bytes_remaining = 0;
    state.phase = Phase::COMPLETE;  // Or transition to next field
  }
  
  return err;
}
```

#### 5.2.4 Repeated Fields (Non-Packed)

**Wire Format**: `[tag1][value1][tag2][value2]...`

**State Machine**:
1. **TAG phase**: Write tag for current element, transition to DATA
2. **DATA phase**: Write element value, increment `element_index`, back to TAG for next element
3. **COMPLETE**: When all elements written

**State Fields Used**:
- `phase`: TAG → DATA → TAG → DATA → ... → COMPLETE
- `field_id`: Field number
- `element_index`: Current element being serialized

**Partial Behavior**:
- Each element is a separate tag+value pair
- Can stop at any point that is normal for the repeated field type, for example repeated messages.

#### 5.2.5 Repeated Fields (Packed)

**Wire Format**: `[tag][total_size][value1][value2]...`

**State Machine**:
1. **TAG phase**: Write tag with LENGTH_DELIMITED wire type, transition to SIZE
2. **SIZE phase**: Calculate total packed size, write size varint, transition to DATA
3. **DATA phase**: Write elements sequentially, track `element_index` and `bytes_remaining`

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: Field number
- `size_value`: Total packed size
- `element_index`: Current element
- `bytes_remaining`: Bytes left in packed data

**Partial Behavior**:
- All elements must be contiguous after size
- Can split between elements

#### 5.2.6 Repeated Message Fields (specific case of Non-Packed)

**Wire Format**: `[tag1][size1][msg1][tag2][size2][msg2]...`

**State Machine**:
1. **TAG phase**: Write tag, transition to SIZE
2. **SIZE phase**: Calculate message size, write size varint, transition to DATA
3. **DATA phase**: Delegate to nested message with child state
4. After message complete: increment `element_index`, back to TAG for next element

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → TAG → SIZE → DATA → ... → COMPLETE
- `field_id`: Field number
- `element_index`: Current message element
- `child`: Nested message state

---

## 6. Deserialization State Machine

### 6.1 Overview

Deserialization follows a similar but input-driven state machine:

```
┌─────────────────────────────────────────────────────────────────┐
│                         TAG Phase                               │
│  - Read tag from buffer                                         │
│  - Extract field_id and wire_type                               │
│  - If LENGTH_DELIMITED: → SIZE phase                            │
│  - Else: → DATA phase                                           │
│  - If buffer empty: return END_OF_BUFFER                        │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         SIZE Phase                              │
│  - Read size varint                                             │
│  - Store in size_value and bytes_remaining                      │
│  - → DATA phase                                                 │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                         DATA Phase                              │
│  - For scalars: read value                                      │
│  - For strings/bytes: read bytes, decrement bytes_remaining     │
│  - For messages: recurse with child state                       │
│  - For unknown fields: skip based on wire_type                  │
│  - → next field (TAG phase) or COMPLETE                         │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 Field Type Deserialization Details

#### 6.2.1 Scalar Fields

**State Machine**:
1. **TAG phase**: Read tag, extract field_id and wire_type, transition to DATA
2. **DATA phase**: Read value based on wire_type, transition to COMPLETE or next TAG

**State Fields Used**:
- `phase`: TAG → DATA → COMPLETE
- `field_id`: From tag
- `wire_type`: From tag

**Partial Behavior**:
- If buffer empty during tag read, return END_OF_BUFFER, state remains in TAG
- If buffer empty during value read, return END_OF_BUFFER, state remains in DATA

#### 6.2.2 String and Bytes Fields

**State Machine**:
1. **TAG phase**: Read tag, transition to SIZE
2. **SIZE phase**: Read size varint, store in `size_value` and `bytes_remaining`, transition to DATA
3. **DATA phase**: Read bytes up to `bytes_remaining`, decrement as we read

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: From tag
- `wire_type`: LENGTH_DELIMITED
- `size_value`: Total size
- `bytes_remaining`: Bytes left to read

**Partial Behavior**:
- Can read partial string/bytes across multiple buffers
- `bytes_remaining` tracks progress

#### 6.2.3 Nested Message Fields

**State Machine**:
1. **TAG phase**: Read tag, transition to SIZE
2. **SIZE phase**: Read size varint, store in `size_value` and `bytes_remaining`, transition to DATA
3. **DATA phase**: Use `ReadBufferSection` to limit child's view, delegate to nested message with `child` state

**State Fields Used**:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `field_id`: From tag
- `wire_type`: LENGTH_DELIMITED
- `size_value`: Total size of nested message (read from wire)
- `bytes_remaining`: Bytes left to read in nested message
- `child`: Nested message state

**Partial Behavior using ReadBufferSection**:

The key insight from the previous implementation (commit `92481a5`) is to use `ReadBufferSection` to limit the child's view of the buffer:

```cpp
case Phase::DATA:
{
  // Create a section that limits how many bytes the child can read
  ReadBufferSection bufferSection(buffer, state.bytes_remaining);
  
  // Delegate to nested message
  Error err = nested_message.deserialize_partial(bufferSection, *state.child);
  
  // After child returns, check how many bytes were consumed
  // ReadBufferSection tracks how many bytes are left in get_size()
  state.bytes_remaining = bufferSection.get_size();  // Bytes remaining in section
  
  if(Error::END_OF_BUFFER == err) 
  {
    // Child needs more data - the section ran out
    // state.bytes_remaining tells us how many more bytes the child needs
    return Error::END_OF_BUFFER;
  }
  else if(Error::NO_ERRORS == err && state.bytes_remaining == 0)
  {
    // Nested message fully deserialized
    state.phase = Phase::COMPLETE;  // Or transition to next field
  }
  else if(Error::NO_ERRORS == err && state.bytes_remaining > 0)
  {
    // Child completed but there are remaining bytes in the section
    // This shouldn't happen for well-formed messages
    return Error::INVALID_DATA;
  }
  
  return err;
}
```

**Key Points**:
- `ReadBufferSection` wraps the parent buffer and limits how many bytes the child can access
- The section's `get_size()` returns how many bytes are left in the section
- When the section runs out, `ReadBufferSection::pop()` returns false, causing `END_OF_BUFFER`
- Parent tracks `bytes_remaining` to know when nested message is complete
- If parent buffer runs out before the section, `END_OF_BUFFER` propagates up

#### 6.2.4 Unknown Fields

When a field_id is not recognized:

1. **TAG phase**: Read tag, determine wire_type
2. **SIZE phase** (if LENGTH_DELIMITED): Read size, store in `bytes_remaining`
3. **SKIP phase**: Skip `bytes_remaining` bytes (or fixed amount for fixed types)

**State Fields Used**:
- `phase`: TAG → SIZE → SKIP → COMPLETE
- `wire_type`: Determines skip strategy
- `bytes_remaining`: Bytes to skip (for LENGTH_DELIMITED)

---

## 7. API Design

### 7.1 Message Interface Extensions

```cpp
class MessageInterface : public Field 
{
  public:
    // ... existing methods ...
    
    /**
     * \brief Serialize message with partial state support
     * 
     * \param buffer Write buffer (may be small)
     * \param state External state object (must persist between calls)
     * \return Error::NO_ERRORS when complete
     * \return Error::BUFFER_FULL when buffer full, call again with fresh buffer
     * \return Other errors on failure (state should be reset)
     */
    virtual Error serialize_partial(WriteBufferInterface& buffer, 
                                   MessageState& state) const = 0;
    
    /**
     * \brief Deserialize message with partial state support
     * 
     * \param buffer Read buffer (may contain partial data)
     * \param state External state object (must persist between calls)
     * \return Error::NO_ERRORS when complete
     * \return Error::END_OF_BUFFER when buffer empty, call again with more data
     * \return Other errors on failure (state should be reset)
     */
    virtual Error deserialize_partial(ReadBufferInterface& buffer,
                                      MessageState& state) = 0;
    
    /**
     * \brief Reset serialization state to initial values
     */
    void reset_serialize_state(MessageState& state) const;
    
    /**
     * \brief Reset deserialization state to initial values
     */
    void reset_deserialize_state(MessageState& state);
};
```

### 7.2 Generated Code Structure

For a message like:
```protobuf
message MyMessage 
{
  int32 id = 1;
  string name = 2;
  repeated int32 values = 3;
}
```

Generated code:
```cpp
class MyMessage final : public ::EmbeddedProto::MessageInterface 
{
  public:
    // State depth for this message (no nested messages)
    static constexpr uint32_t STATE_DEPTH = 1;
    
    // Convenience typedef
    using StateStack = ::EmbeddedProto::MessageStateStack<STATE_DEPTH>;
    
    // ... existing methods ...
    
    Error serialize_partial(WriteBufferInterface& buffer, 
                           MessageState& state) const override;
    
    Error deserialize_partial(ReadBufferInterface& buffer,
                             MessageState& state) override;
    
  private:
    // Internal helpers for each phase
    Error serialize_tag_phase(WriteBufferInterface& buffer, MessageState& state) const;
    Error serialize_size_phase(WriteBufferInterface& buffer, MessageState& state) const;
    Error serialize_data_phase(WriteBufferInterface& buffer, MessageState& state) const;
};
```

### 7.3 Usage Example

```cpp
// Create message
MyMessage msg;
msg.set_id(42);
msg.set_name("Hello World");
msg.add_values(1);
msg.add_values(2);
msg.add_values(3);

// Create small buffer (e.g., CAN-FD with 64 bytes)
WriteBufferFixedSize<64> buffer;

// Create state stack
MyMessage::StateStack state;

// Serialize in chunks
Error err = msg.serialize_partial(buffer, state.root());
while (Error::BUFFER_FULL == err) 
{
  // Send buffer contents
  can_send(buffer.data(), buffer.get_size());
  
  // Clear buffer for next chunk
  buffer.clear();
  
  // Continue serialization
  err = msg.serialize_partial(buffer, state.root());
}

if (Error::NO_ERRORS == err) 
{
  // Send final chunk
  can_send(buffer.data(), buffer.get_size());
}

// Reset state for next message
state.reset();
```

---

## 8. Buffer Interface Considerations

### 8.1 Current Interface

The current `WriteBufferInterface` provides:
- `push(byte)` - Add single byte
- `push(bytes, length)` - Add multiple bytes
- `get_size()` - Current size
- `get_max_size()` - Maximum capacity
- `get_available_size()` - Remaining space
- `clear()` - Reset buffer

### 8.2 No Changes Required

Based on the design decision to "update buffer only on success", no changes to the buffer interface are needed:

1. **Try to write**: Attempt to write tag, size, and data
2. **Check success**: If any write fails (buffer full), nothing was written
3. **Return BUFFER_FULL**: Caller provides fresh buffer and retries

This works because:
- Individual `push()` calls return `bool` indicating success
- We check after each write
- If a write fails, we return immediately without updating state

### 8.3 Implementation Pattern

```cpp
Error serialize_partial(WriteBufferInterface& buffer, MessageState& state) const 
{
  // Get initial buffer size to detect partial writes
  const uint32_t initial_size = buffer.get_size();
  
  while (state.field_id <= MAX_FIELD_ID) 
  {
    switch (state.phase) 
    {
      case Phase::TAG: 
      {
        // Try to write tag
        const uint32_t tag = WireFormatter::MakeTag(state.field_id, wire_type);
        Error err = WireFormatter::SerializeVarint(tag, buffer);
        if (Error::NO_ERRORS != err) 
        {
          return err;  // Buffer full, nothing written
        }
        state.phase = Phase::DATA;  // or SIZE for length-delimited
        break;
      }
      // ... other phases ...
    }
  }
  
  state.phase = Phase::COMPLETE;
  return Error::NO_ERRORS;
}
```

---

## 9. Integration with Existing Code

### 9.1 serialize() Calling serialize_partial()

To avoid code duplication and reduce flash usage, `serialize()` can call `serialize_partial()` internally:

```cpp
Error serialize(WriteBufferInterface& buffer) const override 
{
  // Create temporary state on stack
  MessageStateStack<STATE_DEPTH> state;
  
  // Call partial serialization
  Error err = serialize_partial(buffer, state.root());
  
  // serialize() expects complete serialization in one call
  // If BUFFER_FULL, that's an error for serialize()
  if(Error::BUFFER_FULL == err)
  {
    // Buffer was too small for complete message
    // This is a usage error - buffer should be large enough
    return Error::BUFFER_FULL;
  }
  
  return err;
}
```

**Trade-off**: This adds a small amount of stack usage for the temporary state, but:
- Eliminates code duplication
- Reduces flash usage (one implementation)
- Compiler may optimize away unused state fields for simple messages

### 9.2 deserialize() Calling deserialize_partial()

Similarly for deserialization:

```cpp
Error deserialize(ReadBufferInterface& buffer) override 
{
  // Create temporary state on stack
  MessageStateStack<STATE_DEPTH> state;
  
  // Call partial deserialization
  Error err = deserialize_partial(buffer, state.root());
  
  // deserialize() expects complete message in buffer
  // If END_OF_BUFFER, that's an error for deserialize()
  return err;
}
```

**Note**: The existing `deserialize_id_number_` and `deserialize_wire_type_` member variables in `MessageInterface` can be removed once partial deserialization is implemented, as the state is now external.

---

## 10. Error Handling

### 10.1 Error Codes

Add new error codes to `Errors.h`:

```cpp
enum class Error 
{
  NO_ERRORS = 0,
  END_OF_BUFFER = 1,
  BUFFER_FULL = 2,
  INVALID_WIRETYPE = 3,
  ARRAY_FULL = 4,
  INVALID_FIELD_ID = 5,
  OVERLONG_VARINT = 6,
  INDEX_OUT_OF_BOUND = 7,
  // New errors for partial serialization:
  STATE_MISMATCH = 10,       // State doesn't match message type
  NESTING_TOO_DEEP = 11,     // Message nesting exceeds state depth
  // ... existing errors ...
};
```

### 10.2 Error Categories

| Error | Category | Action |
|-------|----------|--------|
| `BUFFER_FULL` | Recoverable | Clear buffer, call again |
| `END_OF_BUFFER` | Recoverable | Provide more data, call again |
| `STATE_MISMATCH` | Fatal | Reset state, start over |
| `NESTING_TOO_DEEP` | Fatal | Use larger state stack |
| `INVALID_WIRETYPE` | Fatal | Data corrupted, reset |
| `OVERLONG_VARINT` | Fatal | Data corrupted, reset |

### 10.3 State After Error

- **Recoverable errors** (`BUFFER_FULL`, `END_OF_BUFFER`): State is valid, continue from where stopped
- **Fatal errors**: State should be reset before reuse

```cpp
// Example error handling
Error err = msg.serialize_partial(buffer, state);
if (Error::BUFFER_FULL == err) 
{
  // Recoverable - send buffer and continue
  send(buffer);
  buffer.clear();
  err = msg.serialize_partial(buffer, state);
}
else if(Error::NO_ERRORS != err)
{
  // Fatal error - reset state
  state.reset();
  // Handle error...
}
```

---

## 11. Minimum Buffer Size

### 11.1 Requirements

The minimum buffer size depends on the largest single atomic write:

| Field Type | Minimum Size | Reason |
|------------|--------------|--------|
| bool | 2 bytes | tag (1) + value (1) |
| int32 (small) | 2 bytes | tag (1) + value (1) |
| int32 (max) | 11 bytes | tag (1) + value (10) |
| fixed64 | 9 bytes | tag (1) + value (8) |
| double | 9 bytes | tag (1) + value (8) |
| string tag+size | 11 bytes | tag (1) + size (10 max) |

**Recommended minimum**: 20 bytes to handle most cases comfortably.

### 11.2 Documentation

This should be documented but not enforced at compile time, as:
- Users may derive custom buffer classes
- The actual minimum depends on the message structure
- Runtime checks add overhead

### 11.3 Static Assert for Provided Buffers

For the provided `WriteBufferFixedSize` and `ReadBufferFixedSize`, a static assert can be added:

```cpp
template<uint32_t SIZE>
class WriteBufferFixedSize 
{
  static_assert(SIZE >= 10, "Buffer size should be at least 10 bytes for partial serialization");
  // ...
};
```

---

## 12. Implementation Plan

### Phase 1: Core State Classes
- [ ] Add `Phase` enum to `Fields.h` or new header
- [ ] Implement `MessageState` class
- [ ] Implement `MessageStateStack<DEPTH>` template
- [ ] Add unit tests for state management
- [ ] Run unit tests

### Phase 2: Partial Serialization
- [ ] Add `serialize_partial` to `MessageInterface`
- [ ] Implement scalar field serialization
- [ ] Implement string/bytes serialization
- [ ] Implement nested message serialization
- [ ] Implement repeated field serialization (packed and unpacked)
- [ ] Update Python code generator
- [ ] Add unit tests
- [ ] Run unit tests

### Phase 3: Partial Deserialization
- [ ] Add `deserialize_partial` to `MessageInterface`
- [ ] Implement scalar field deserialization
- [ ] Implement string/bytes deserialization
- [ ] Implement nested message deserialization
- [ ] Implement unknown field skipping
- [ ] Implement repeated field deserialization
- [ ] Update Python code generator
- [ ] Add unit tests
- [ ] Run unit tests

### Phase 4: Integration
- [ ] Modify `serialize()` to call `serialize_partial()`
- [ ] Modify `deserialize()` to call `deserialize_partial()`
- [ ] Remove internal state variables from `MessageInterface`
- [ ] Update existing tests
- [ ] Performance benchmarks (Ask user how to do this)

### Phase 5: Documentation
- [ ] Update API documentation
- [ ] Add usage examples
- [ ] Update README
- [ ] Create migration guide

---

## 13. Testing Strategy

### 13.1 Unit Tests

Integrate new unit tests in the excesting framework.

Test each field type with various buffer sizes:
- Large buffer (complete in one call)
- Medium buffer (split at field boundaries)
- Small buffer (split within fields)
- Minimum buffer (10-20 bytes)

### 13.2 Integration Tests

- Nested messages with various depths
- Messages with all field types
- Round-trip: serialize partial → deserialize partial
- Compare with standard serialize/deserialize

### 13.3 Edge Cases

- Empty messages
- Messages with only optional fields (none set)
- Deeply nested messages (exceeding state depth)
- Very large strings/bytes
- Packed repeated fields with many elements
- Unknown fields during deserialization

---

## 14. Open Questions

| # | Question | Status |
|---|----------|--------|
| 1 | Should `serialize()` call `serialize_partial()` internally? | **Decided**: Yes, to reduce code duplication |
| 2 | Minimum buffer size enforcement? | **Decided**: Document only, static_assert for provided classes |
| 3 | State after fatal errors? | **Decided**: Reset required |
| 4 | Remove existing `deserialize_id_number_`? | **Decided**: Yes, replaced by external state |

---

## Appendix A: Wire Format Reference

### A.1 Tag Format

```
Tag = (field_number << 3) | wire_type

Wire Types:
0 - VARINT (int32, int64, uint32, uint64, sint32, sint64, bool, enum)
1 - FIXED64 (fixed64, sfixed64, double)
2 - LENGTH_DELIMITED (string, bytes, embedded messages, packed repeated)
5 - FIXED32 (fixed32, sfixed32, float)
```

### A.2 Field Examples

| Field | Value | Wire Format |
|-------|-------|-------------|
| `int32 a = 1;` | 150 | `08 96 01` (tag=0x08, value=0x96 0x01) |
| `string b = 2;` | "hi" | `12 02 68 69` (tag=0x12, len=2, "hi") |
| `fixed32 c = 3;` | 0x12345678 | `1D 78 56 34 12` (tag=0x1D, value LE) |

---

## Appendix B: State Size Calculation

### B.1 Memory Footprint

```cpp
// MessageState size breakdown (typical 32-bit system)
Phase phase;              // 1 byte (uint8_t)
uint32_t field_id;        // 4 bytes
WireType wire_type;       // 1 byte (uint8_t)
uint32_t element_index;   // 4 bytes
uint32_t bytes_remaining; // 4 bytes
uint32_t size_value;      // 4 bytes
MessageState* child;      // 4 bytes (pointer)
// Total: ~22 bytes (with padding: 24 bytes)
```

### B.2 Example State Stack Sizes

| Message Depth | Stack Size |
|---------------|------------|
| 1 (no nesting) | 24 bytes |
| 2 | 48 bytes |
| 3 | 72 bytes |
| 4 | 96 bytes |
| 5 | 120 bytes |

---

## Appendix C: References

- [Protobuf Wire Format](https://protobuf.dev/programming-guides/encoding/)
- [EmbeddedProto v4 High-Level Specification](v4_high_level_specification.md)
- Git commit `cdc66b81` - Initial draft of MessageDeserializationStateTemplate
- Git commit `92481a5` - Previous partial deserialization implementation (removed), contains valuable unit tests for:
  - Partial nested message deserialization (`test_NestedMessage.cpp`)
  - Partial repeated field deserialization (`test_RepeatedFieldMessage.cpp`)
  - Partial scalar deserialization (`test_SimpleTypes.cpp`)
  - Partial string/bytes deserialization (`test_string_bytes.cpp`)
