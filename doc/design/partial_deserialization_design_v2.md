# Partial Deserialization v2 — Reducing Generated Code Size

## Document Information
- **Status**: Design
- **Version**: 2.1 (clarified after review)
- **Related**: `doc/design/partial_serialization_design_v2.md` (delegation pattern for partial serialization)
- **Related**: `doc/design/partial_serialization_design.md` (state machine reference)
- **Related**: `src/Fields.h`, `src/FieldStringBytes.h`, `src/MessageInterface.h`, `src/RepeatedField.h`, `src/ReadBufferSection.h`
- **Related**: `EmbeddedProto/templates/TypeDefMsg.h.jinja2` (message-level deserialize code)
- **Related**: `EmbeddedProto/templates/FieldBasic_Deserialize.h.jinja2`, `FieldEnum_Deserialize.h.jinja2`, `FieldMsg_Deserialize.h.jinja2`

---

## 1. Problem

Partial serialization v2 already moved the TAG→SIZE→DATA write state machine into runtime library code, drastically reducing generated code.

Partial deserialization still lacks the same compact architecture. The generated message code remains responsible for most of the control flow:

- read tag and decode `field_id` + `wire_type`
- dispatch through a large switch
- deserialize every field shape inline
- skip unknown fields inline
- keep internal per-message deserialization members

This scales poorly with field count and oneof/repeated complexity, increasing flash usage on MCUs.

## 2. Solution: Move Partial Deserialize State Machine to Library Methods

Apply the same design principle used in serialization v2:

- keep generated code focused on field routing only;
- move TAG/SIZE/DATA phase logic to runtime methods implemented once.

The target is that generated field handling becomes one function call per field-type category.

### 2.1 Class Hierarchy

```
Field (abstract base)
  ├── internal::FieldStringBytes → FieldString, FieldBytes
  ├── MessageInterface
  └── RepeatedField<DATA_TYPE>

FieldTemplate<...> (standalone template for scalar/enum)
```

### 2.2 New/Extended Methods

#### `Field::deserialize_partial_as_field()` — virtual, for LENGTH_DELIMITED Field-derived types

```cpp
// In Field (pure virtual)
virtual Error deserialize_partial_as_field(ReadBufferInterface& buffer,
                                           MessageState& state) = 0;
```

Used by `FieldStringBytes`, `MessageInterface`, and `RepeatedField` (packed/unpacked message/string/bytes forms where applicable).

#### `Field::deserialize_partial_size_phase()` — protected non-virtual helper

```cpp
// In Field (protected, non-virtual)
Error deserialize_partial_size_phase(ReadBufferInterface& buffer,
                                     MessageState& state) const;
```

Responsibilities:

- If `state.phase == Phase::SIZE`: read varint size.
- Store to `state.size_value` and `state.bytes_remaining`.
- Transition to `Phase::DATA`.
- Preserve state and return `END_OF_BUFFER` if size varint is incomplete.

#### `FieldTemplate::deserialize_partial_check_type()` — for scalar/enums

```cpp
// In FieldTemplate (inline)
inline Error deserialize_partial_check_type(ReadBufferInterface& buffer,
                                            MessageState& state,
                                            WireFormatter::WireType expected_wire_type);
```

Responsibilities:

- Validate `state.wire_type` against expected type.
- Deserialize scalar/enum atomically in DATA phase.
- Set `state.phase = Phase::COMPLETE` on success.
- Keep state in DATA on `END_OF_BUFFER`.

### 2.3 Message-Level Partial API

#### `MessageInterface::deserialize_partial()`

```cpp
virtual Error deserialize_partial(ReadBufferInterface& buffer,
                                  MessageState& state) = 0;
```

Generated messages implement this API, but with a compact loop. Runtime helpers perform field-type logic.

#### `MessageInterface::skip_unknown_field_partial()`

```cpp
Error skip_unknown_field_partial(ReadBufferInterface& buffer,
                                 MessageState& state) const;
```

For unknown field ids with resumable behavior across calls:

- VARINT / FIXED32 / FIXED64 skip in DATA
- LENGTH_DELIMITED uses SIZE then DATA with `bytes_remaining`

---

## 3. State Management Details (Including Nested Messages)

This section makes state ownership and transitions explicit for complex nested scenarios.

### 3.1 Ownership and Lifetime

- State is externally allocated by the user through `MessageStateStack<DEPTH>`.
- `root()` is always passed to the outer message.
- `child` links are constructed once by `MessageStateStack` and never re-assigned during runtime.
- No dynamic allocation is introduced.

### 3.2 Parent/Child Transition Rules

For nested messages (`MessageInterface` field):

1. Parent enters SIZE and reads nested payload size into `state.bytes_remaining`.
2. Parent enters DATA and creates `ReadBufferSection(buffer, state.bytes_remaining)`.
3. Parent calls child `deserialize_partial(section, *state.child)`.
4. Parent updates `state.bytes_remaining = section.get_size()`.
5. Parent completes only when both are true:
   - child returns `NO_ERRORS`
   - `state.bytes_remaining == 0`

### 3.3 Reset Boundaries

On a completed field occurrence (`state.phase == Phase::COMPLETE`), message-level loop resets field-local progress before reading next tag:

- `state.phase = Phase::TAG`
- `state.size_value = 0`
- `state.bytes_remaining = 0`
- `state.element_index = 0` (for next repeated field usage)
- if `state.child != nullptr`: `state.child->reset()`

This prevents stale nested COMPLETE states and stale repeated indices from leaking into the next field.

### 3.4 Oneof Interaction Rule

- oneof field selection remains at message dispatch level.
- once a selected oneof field completes, normal reset boundaries apply.
- if malformed wire data targets a non-active oneof field variant, parsing follows wire input (as in normal protobuf parsing), and oneof setter behavior stays generator-defined.

---

## 4. Deserialization Flow

### 4.1 TAG phase (message-level)

When `state.phase == Phase::TAG`:

1. read tag via `WireFormatter::DeserializeTag(buffer, state.wire_type, state.field_id)`
2. determine next phase:
   - `LENGTH_DELIMITED` → `Phase::SIZE` (for fields requiring length)
   - otherwise → `Phase::DATA`
3. dispatch by `state.field_id`

If tag is incomplete, return `END_OF_BUFFER` without state corruption.

### 4.2 SIZE phase (field-level helper)

`deserialize_partial_size_phase()` decodes and stores size once. On success, transition to DATA.

### 4.3 DATA phase (field-specific)

Each field class handles its own data progress:

- scalar/enum: one value read
- string/bytes: incremental byte consumption
- nested message: child-state recursion via `ReadBufferSection`
- repeated packed: iterate elements within bounded section
- repeated unpacked: consume one occurrence per tag

On completion of one field occurrence, set `state.phase = Phase::COMPLETE`; message loop then resets state to TAG for the next tag.

---

## 5. Per-Type Behavior

### 5.1 FieldTemplate scalars/enums

- check wire type against expected
- in DATA phase call existing `deserialize()`
- `NO_ERRORS` → COMPLETE
- `END_OF_BUFFER` → stay DATA

### 5.2 FieldStringBytes (string/bytes)

```cpp
Error deserialize_partial_as_field(...) override
{
  Error return_value = Error::NO_ERRORS;

  if(Phase::SIZE == state.phase)
  {
    return_value = deserialize_partial_size_phase(buffer, state);
  }

  if((Error::NO_ERRORS == return_value) && (Phase::DATA == state.phase))
  {
    // Read min(bytes_remaining, buffer.get_size()) bytes
    // Append to field storage and decrement bytes_remaining
    // bytes_remaining == 0 -> phase COMPLETE
  }

  return return_value;
}
```

### 5.3 MessageInterface (nested messages)

```cpp
Error MessageInterface::deserialize_partial_as_field(...) override
{
  Error return_value = Error::NO_ERRORS;

  if(Phase::SIZE == state.phase)
  {
    return_value = deserialize_partial_size_phase(buffer, state);
  }

  if((Error::NO_ERRORS == return_value) && (Phase::DATA == state.phase))
  {
    ReadBufferSection section(buffer, state.bytes_remaining);
    if(nullptr != state.child)
    {
      return_value = this->deserialize_partial(section, *state.child);
    }
    else
    {
      return_value = Error::NESTING_TOO_DEEP;
    }

    state.bytes_remaining = section.get_size();
    if((Error::NO_ERRORS == return_value) && (0U == state.bytes_remaining))
    {
      state.phase = Phase::COMPLETE;
    }
  }

  return return_value;
}
```

### 5.4 RepeatedField

`deserialize_partial_as_field()` dispatches:

- packed: one tag, one size, many data elements
- unpacked: one occurrence per tag

Packed path:

- SIZE: read packed byte count
- DATA: deserialize elements using a `ReadBufferSection` bounded by `bytes_remaining`
- update `element_index` and `bytes_remaining`
- complete when section is exhausted

Unpacked path:

- each entry uses the current tag occurrence
- for message/string/bytes entries, delegate to element partial method
- for scalar entries, call scalar partial helper and push value

### 5.5 Concrete Repeated Examples

#### Packed repeated scalar (`repeated uint32 values = 3 [packed=true]`)

Wire format: `[tag][size][v1][v2][v3...]`

Pseudocode:

```cpp
if(Phase::SIZE == state.phase)
{
  // Read packed payload size once
  err = deserialize_partial_size_phase(buffer, state);
}

if((Error::NO_ERRORS == err) && (Phase::DATA == state.phase))
{
  ReadBufferSection section(buffer, state.bytes_remaining);
  while((section.get_size() > 0U) && (Error::NO_ERRORS == err))
  {
    DATA_TYPE value;
    err = value.deserialize(section);
    if(Error::NO_ERRORS == err)
    {
      err = this->add(value);
    }
  }
  state.bytes_remaining = section.get_size();
  if((Error::NO_ERRORS == err) && (0U == state.bytes_remaining))
  {
    state.phase = Phase::COMPLETE;
  }
}
```

#### Unpacked repeated nested message (`repeated SubMsg items = 5`)

Wire format: `[tag][len][msg][tag][len][msg]...`

Pseudocode:

```cpp
// One tag occurrence corresponds to one element.
const uint32_t index = this->get_length();
if(index >= this->get_max_length())
{
  return Error::ARRAY_FULL;
}

Error err = this->get(index).deserialize_partial_as_field(buffer, state);
if((Error::NO_ERRORS == err) && (Phase::COMPLETE == state.phase))
{
  // Element finished; message loop will reset phase to TAG for next tag occurrence.
}
```

Boundary behavior:

- If nested element is incomplete: `END_OF_BUFFER`, state remains in SIZE/DATA.
- If packed payload contains malformed element: propagate decode error.
- If destination array is full: return `ARRAY_FULL` (fatal for current message parse).

---

## 6. Error Handling and Recovery Strategy

### 6.1 Error Matrix

| Context | Error | Recoverable | Action |
|---|---|---|---|
| TAG read | `END_OF_BUFFER` | Yes | Provide more bytes, call again |
| TAG read | `INVALID_WIRETYPE` | No | Reset state, abort message |
| TAG read | `OVERLONG_VARINT` | No | Reset state, abort message |
| Field dispatch | `INVALID_FIELD_ID` (if used) | No | Reset state, abort message |
| SIZE read | `END_OF_BUFFER` | Yes | Provide more bytes |
| SIZE read | `OVERLONG_VARINT` | No | Reset state |
| Scalar DATA | `END_OF_BUFFER` | Yes | Continue with more bytes |
| Scalar DATA | `INVALID_WIRETYPE` | No | Reset state |
| Nested DATA | `END_OF_BUFFER` | Yes | Continue with more bytes |
| Nested DATA | `NESTING_TOO_DEEP` | No | Increase state depth / fix call site |
| Repeated add | `ARRAY_FULL` | No | Reset state / reject message |
| Internal invariant | `STATE_MISMATCH` | No | Reset state, investigate bug |

### 6.2 Recoverable vs Fatal Rule

- Recoverable: `END_OF_BUFFER` only.
- Fatal: all structural/data validity failures.

### 6.3 Malformed Data Behavior

On malformed wire data (wrong wire type, invalid varint, inconsistent length):

1. return fatal error immediately,
2. do not attempt speculative resynchronization,
3. caller must reset state and restart from a known message boundary.

This keeps behavior deterministic and aligned with constrained embedded parsing.

---

## 7. Template Simplification and Integration

### 7.1 New partial deserialize field template

Introduce a compact `Field_DeserializePartial.h.jinja2` pattern:

```jinja2
{% if field.uses_serialize_len() %}
{% if field.optional %}
presence_[presence::index(presence::fields::{{field.get_name().upper()}})] |= presence::mask(presence::fields::{{field.get_name().upper()}});
{% endif %}
return_value = {{field.get_variable_name()}}.deserialize_partial_as_field(buffer, state);
{% else %}
{% if field.optional %}
presence_[presence::index(presence::fields::{{field.get_name().upper()}})] |= presence::mask(presence::fields::{{field.get_name().upper()}});
{% endif %}
return_value = {{field.get_variable_name()}}.deserialize_partial_check_type(
    buffer, state, ::EmbeddedProto::WireFormatter::WireType::{{field.get_wire_type_name()}});
{% endif %}
```

### 7.2 Message template (`TypeDefMsg.h.jinja2`)

Add `deserialize_partial()` with a compact state loop:

- TAG read once when phase is TAG
- dispatch by `state.field_id`
- on COMPLETE: reset per-field progress (`phase=TAG`, `element_index=0`, `size_value=0`, and `child->reset()` when present)
- unknown field path calls `skip_unknown_field_partial()`

This removes dependence on per-message internal members like:

- `deserialize_id_number_`
- `deserialize_wire_type_`

for the partial path.

### 7.3 Generation Pipeline Changes

1. Add a dedicated render hook for partial field deserialization (e.g. `render_deserialize_partial(environment)`).
2. Keep existing non-partial templates unchanged.
3. In `TypeDefMsg.h.jinja2`, add `deserialize_partial()` alongside existing `deserialize()`.
4. Route partial build path through new field partial template.

This keeps generated API backward compatible while enabling opt-in partial decode.

### 7.4 Before/After High-Level Generated Shape

Before (current):

- `deserialize()` contains tag state, field dispatch, unknown skipping, and inlined field handling.

After (target):

- `deserialize_partial()` contains compact tag loop and dispatch only.
- field handling delegates to runtime methods by field category.
- unknown skipping delegates to `skip_unknown_field_partial()`.

---

## 8. Performance and RAM Characteristics

### 8.1 Flash

- Generated per-message code decreases because field-specific phase logic is removed from templates.
- Runtime library code increases once, shared by all messages.
- Net effect in multi-message projects is expected flash reduction.

### 8.2 RAM

No heap allocations introduced. RAM cost is dominated by `MessageStateStack<DEPTH>`.

Approximate per-state footprint (32-bit target):

- `phase` + `wire_type`: ~2 bytes (+padding)
- `field_id`, `element_index`, `bytes_remaining`, `size_value`: 16 bytes
- `child` pointer: 4 bytes

Typical padded `MessageState`: ~24 bytes.

Examples:

- depth 1: ~24 bytes
- depth 3: ~72 bytes
- depth 5: ~120 bytes

### 8.3 Runtime Performance

- Adds one or more helper call frames versus large generated inline logic.
- Expected impact is negligible compared to I/O pacing in partial-buffer use cases.

---

## 9. Impact

| Metric | Before | After |
|---|---|---|
| Generated deserialization control-flow per message | Large switch + state handling | Compact dispatch loop |
| Per-field generated partial deserialize code | Type-specific inline logic | 1 helper call |
| Template complexity | High in `TypeDefMsg.h.jinja2` | Lower, reusable helper pattern |
| Library runtime code | Low | Higher (compiled once) |
| Flash usage (many messages/fields) | Higher | Lower overall |
| RAM impact | State already required | Unchanged model (external state) |

---

## 10. Testing Strategy

### 10.1 Unit Tests

- scalar partial deserialize split across buffers
- string/bytes split in SIZE and DATA boundaries
- nested messages with partial child progression via `ReadBufferSection`
- repeated packed and unpacked (including repeated nested messages)
- unknown field partial skip parity with full deserialize

### 10.2 Negative/Robustness Tests

- invalid wire type for known field
- overlong varint in TAG and SIZE
- packed payload length mismatch
- repeated destination overflow (`ARRAY_FULL`)
- forced `NESTING_TOO_DEEP` scenario

### 10.3 Integration Tests

- round-trip: partial serialize → partial deserialize
- mixed known + unknown fields
- oneof and optional presence correctness under chunked input

---

## 11. Sequence Diagrams (Textual)

### 11.1 Message-level dispatch sequence

```text
Caller -> Message::deserialize_partial(state)
Message -> WireFormatter::DeserializeTag (if phase TAG)
Message -> Field helper by state.field_id
Field helper -> (optional) SIZE helper
Field helper -> DATA decode
Field helper -> phase COMPLETE or END_OF_BUFFER
Message -> reset per-field transient state when COMPLETE
Message -> return NO_ERRORS / END_OF_BUFFER / fatal error
```

### 11.2 Nested message sequence (`ReadBufferSection`)

```text
Parent field(DATA) -> ReadBufferSection(parent_buffer, bytes_remaining)
Parent field -> ChildMessage::deserialize_partial(section, child_state)
ChildMessage -> consume bytes from section
Parent field <- child result
Parent field -> bytes_remaining = section.get_size()
Parent field -> COMPLETE when child done and bytes_remaining == 0
```

---

## 12. Files to Modify (Implementation Phase)

| File | Change |
|---|---|
| `src/Fields.h` | Add `deserialize_partial_as_field()` virtual and size-phase helper; scalar partial deserialize helper on `FieldTemplate` |
| `src/Fields.cpp` | Implement shared size-phase helper |
| `src/FieldStringBytes.h` | Implement `deserialize_partial_as_field()` for string/bytes DATA progression |
| `src/MessageInterface.h` | Add virtual `deserialize_partial()` and partial unknown-skip API |
| `src/MessageInterface.cpp` | Implement `skip_unknown_field_partial()` and resumable length-delimited skip |
| `src/RepeatedField.h` | Add repeated partial deserialization paths (packed/unpacked) |
| `EmbeddedProto/templates/TypeDefMsg.h.jinja2` | Add compact `deserialize_partial()` generation and state transitions |
| `EmbeddedProto/templates/Field_DeserializePartial.h.jinja2` | New compact field-level partial-deserialize template |

---

## 13. Compatibility and Migration Notes

- Keep existing `deserialize()` behavior unchanged for full-buffer usage.
- `deserialize_partial()` is opt-in and uses external `MessageStateStack` (already available).
- During integration, regular `deserialize()` can later be refactored to reuse partial logic, but this is not required for this design step.

---

## 14. State Machine Reference

For detailed state definitions and foundational behavior, see:

- `doc/design/partial_serialization_design.md` section 6 (deserialization state machine)
- `src/MessageState.h` for shared state fields (`phase`, `field_id`, `wire_type`, `bytes_remaining`, `size_value`, `element_index`, `child`)
