# Partial Deserialization v2 — Implementation Plan

This plan implements `doc/design/partial_deserialization_design_v2.md` in incremental, verifiable phases.  
Each phase includes targeted validation before moving to the next phase.

## Step 1 — Core API scaffolding (compile-safe)

### Implement
- Add `Field` API in `src/Fields.h`:
  - `virtual Error deserialize_partial_as_field(ReadBufferInterface&, MessageState&) = 0;`
  - protected helper declaration `deserialize_partial_size_phase(...)`.
- Add scalar/enum helper in `FieldTemplate`:
  - `deserialize_partial_check_type(buffer, state, expected_wire_type)`.
- Extend `MessageInterface` in `src/MessageInterface.h`:
  - `virtual Error deserialize_partial(ReadBufferInterface&, MessageState&) = 0;`
  - `Error skip_unknown_field_partial(ReadBufferInterface&, MessageState&) const;`
- Add stub/initial implementations where required to keep build green.

### Verify
- Build:
  - `./build_test.sh`
- Run focused tests:
  - `./build/test/test_EmbeddedProto --gtest_filter="MessageState*:*MessageInterface*"`

### Gate
- Code compiles and foundational state/message tests pass.

---

## Step 2 — SIZE-phase helper + partial unknown-field skip

### Implement
- Implement `Field::deserialize_partial_size_phase()` in `src/Fields.cpp`:
  - active in `Phase::SIZE`
  - decode varint length
  - write `state.size_value` and `state.bytes_remaining`
  - transition to `Phase::DATA`
  - preserve state and return `END_OF_BUFFER` if incomplete.
- Implement `MessageInterface::skip_unknown_field_partial()` in `src/MessageInterface.cpp`:
  - resumable skipping for VARINT/FIXED32/FIXED64 in DATA
  - LENGTH_DELIMITED skip via SIZE then DATA using `bytes_remaining`.

### Verify
- Build:
  - `./build_test.sh`
- Run focused tests:
  - `./build/test/test_EmbeddedProto --gtest_filter="*unkown*:*unknown*:*Wire*:*ReadBufferSection*"`
  - `./build/test/test_EmbeddedProto --gtest_filter="*ReadBuffer*"`

### Gate
- TAG/SIZE/DATA skip path is resumable and correct.

---

## Step 3 — `FieldStringBytes` partial delegate

### Implement
- Implement `FieldStringBytes::deserialize_partial_as_field()` in `src/FieldStringBytes.h`:
  - call size helper in SIZE
  - consume `min(bytes_remaining, buffer.get_size())` in DATA
  - decrement `bytes_remaining`
  - set `Phase::COMPLETE` when `bytes_remaining == 0`.

### Verify
- Build:
  - `./build_test.sh`
- Run focused tests:
  - `./build/test/test_EmbeddedProto --gtest_filter="*string*:*bytes*"`
- Add/extend tests for split boundaries (SIZE split and DATA split), then rerun the same filter.

### Gate
- Chunked string/bytes partial decode passes.

---

## Step 4 — Nested message delegate (`MessageInterface` as field)

### Implement
- Implement `MessageInterface::deserialize_partial_as_field(...)`:
  - SIZE via shared size helper
  - DATA via `ReadBufferSection(buffer, state.bytes_remaining)`
  - call `deserialize_partial(section, *state.child)`
  - propagate `NESTING_TOO_DEEP` when `state.child == nullptr`
  - update `state.bytes_remaining = section.get_size()`
  - set COMPLETE only when child returns `NO_ERRORS` and remaining bytes are zero.

### Verify
- Build:
  - `./build_test.sh`
- Run focused tests:
  - `./build/test/test_EmbeddedProto --gtest_filter="*NestedMessage*:*ReadBufferSection*"`
- Add/extend chunked nested progression tests if needed and rerun.

### Gate
- Nested partial decode progresses correctly across multiple input chunks.

---

## Step 5 — `RepeatedField` packed/unpacked partial paths

### Implement
- Add `RepeatedField::deserialize_partial_as_field()` in `src/RepeatedField.h` with dispatch:
  - packed path: SIZE once, bounded DATA section, element loop, update `element_index/bytes_remaining`, COMPLETE when exhausted.
  - unpacked path: one element per tag occurrence; delegate for message/string/bytes; scalar path via scalar helper + `add()`.
- Ensure overflow returns `Error::ARRAY_FULL`.

### Verify
- Build:
  - `./build_test.sh`
- Run focused tests:
  - `./build/test/test_EmbeddedProto --gtest_filter="*RepeatedField*:*Repeated*"`
- Add/extend tests for packed/unpacked chunking and repeated nested messages.

### Gate
- Repeated partial decode passes for packed and unpacked forms, including overflow behavior.

---

## Step 6 — Generator integration for compact message partial loop

### Implement
- Add template: `EmbeddedProto/templates/Field_DeserializePartial.h.jinja2`.
- Update `EmbeddedProto/templates/TypeDefMsg.h.jinja2`:
  - generate compact `deserialize_partial()` loop
  - TAG read in TAG phase
  - field-id dispatch
  - unknown path through `skip_unknown_field_partial()`
  - per-field COMPLETE reset boundary:
    - `phase = TAG`
    - `size_value = 0`
    - `bytes_remaining = 0`
    - `element_index = 0`
    - `child->reset()` when present.
- Wire the new partial field render hook in generator Python files (`EmbeddedProto/*.py`).
- Keep existing full `deserialize()` path unchanged.

### Verify
- Regenerate code via existing project generation flow.
- Build:
  - `./build_test.sh`
- Run focused impacted suites:
  - `./build/test/test_EmbeddedProto --gtest_filter="*SimpleTypes*:*optional*:*oneof*:*unkown*:*NestedMessage*"`

### Gate
- Generated code compiles and behavior parity is maintained for optional/oneof/unknown handling.

---

## Step 7 — Error policy and invariants hardening

### Implement
- Enforce recoverable vs fatal rule:
  - recoverable: `END_OF_BUFFER` only
  - all structural/data errors remain fatal.
- Ensure deterministic immediate fatal propagation.
- Validate/reset invariants at field-complete boundaries:
  - `phase`, `size_value`, `bytes_remaining`, `element_index`, child state reset.

### Verify
- Build:
  - `./build_test.sh`
- Run focused negative/robustness tests:
  - `./build/test/test_EmbeddedProto --gtest_filter="*invalid*:*overlong*:*ARRAY_FULL*:*NESTING_TOO_DEEP*:*unkown*"`
- Add missing negatives if absent; rerun same filters.

### Gate
- Error matrix behavior aligns with design doc and no state leakage occurs between field occurrences.

---

## Notes

- Keep `deserialize()` API behavior unchanged for backward compatibility.
- `deserialize_partial()` remains opt-in and uses external `MessageStateStack<DEPTH>`.
- Follow MISRA C++ 2023 conventions and project formatting rules.
