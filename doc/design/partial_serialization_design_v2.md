# Partial Serialization v2 — Reducing Generated Code Size

## Document Information
- **Status**: Design
- **Related**: `doc/design/partial_serialization_design.md` (original design, state machine details, wire format)
- **Related**: `src/Fields.h`, `src/FieldStringBytes.h`, `src/MessageInterface.h`, `src/RepeatedField.h`
- **Related**: `EmbeddedProto/templates/Field_SerializePartial.h.jinja2` (template to rewrite)
- **Related**: `EmbeddedProto/templates/Field_Serialize.h.jinja2` (compact pattern to follow)

---

## 1. Problem

The current `Field_SerializePartial.h.jinja2` template inlines the full TAG→SIZE→DATA state machine for every field in every generated message. This produces 30–80 lines of code per field. A message with 15 scalar fields generates ~450 lines just for `serialize_partial`.

By contrast, the non-partial `serialize()` generates ~4 lines per field because it delegates to `serialize_with_id()` and `serialize_len()` library methods.

On resource-constrained MCUs, this code bloat is unacceptable.

## 2. Solution: Move State Machine to Library Methods

Follow the same pattern as `serialize()`: delegate the TAG→SIZE→DATA state machine to **virtual methods on the field classes** so the generated template reduces to one function call per field.

### 2.1 Class Hierarchy

```
Field (abstract base, virtual serialize())
  ├── internal::FieldStringBytes → FieldString, FieldBytes
  ├── MessageInterface (has serialize_partial(buffer, state) for inner content)
  └── RepeatedField<DATA_TYPE>

FieldTemplate<...> (standalone template, NOT inheriting from Field)
```

### 2.2 New Methods

#### `Field::serialize_partial_as_field()` — virtual, for all LENGTH_DELIMITED Field-derived types

```cpp
// In Field (pure virtual)
virtual Error serialize_partial_as_field(uint32_t field_number,
                                         WriteBufferInterface& buffer,
                                         MessageState& state,
                                         bool optional) const = 0;
```

Each derived class overrides this to handle TAG→SIZE→DATA with its own DATA phase:

| Class | DATA phase |
|---|---|
| `FieldStringBytes` | Calls `this->serialize(buffer)`, tracks `bytes_remaining` |
| `MessageInterface` | Calls `this->serialize_partial(buffer, *state.child)`, tracks `bytes_remaining` |
| `RepeatedField` (packed) | Single TAG→SIZE, iterates elements in DATA |
| `RepeatedField` (unpacked) | Iterates elements, each calls `element.serialize_partial_as_field()` |

#### `Field::serialize_partial_tag_and_size()` — protected non-virtual helper

```cpp
// In Field (protected, non-virtual, implemented in Fields.cpp)
Error serialize_partial_tag_and_size(uint32_t field_number,
                                     uint32_t size,
                                     WriteBufferInterface& buffer,
                                     MessageState& state,
                                     bool optional) const;
```

Handles the shared TAG and SIZE phases:
- **TAG phase**: If non-optional and size==0 → COMPLETE. Else write tag varint → SIZE.
- **SIZE phase**: Write size varint, set `state.bytes_remaining = size` → DATA.
- Returns `NO_ERRORS` when phase reaches DATA (caller handles DATA).

All derived classes call this helper first, then handle DATA themselves.

#### `FieldTemplate::serialize_partial_with_id()` — for scalars/enums

```cpp
// In FieldTemplate (inline template method in Fields.h)
inline Error serialize_partial_with_id(uint32_t field_number,
                                       WriteBufferInterface& buffer,
                                       MessageState& state,
                                       bool optional) const;
```

Atomic TAG+DATA: checks default value, verifies buffer space, writes tag+value together → COMPLETE.

### 2.3 Derived Class Implementations

#### FieldStringBytes (string/bytes)
```cpp
Error serialize_partial_as_field(...) const override
{
  Error return_value = Error::NO_ERRORS;
  if(Phase::DATA != state.phase)
  {
    return_value = serialize_partial_tag_and_size(field_number, get_length(), buffer, state, optional);
  }
  if((Error::NO_ERRORS == return_value) && (Phase::DATA == state.phase))
  {
    // Write raw bytes, track bytes_remaining
    const uint32_t initial_size = buffer.get_size();
    return_value = this->serialize(buffer);
    const uint32_t bytes_written = buffer.get_size() - initial_size;
    state.bytes_remaining -= bytes_written;
    if(0 == state.bytes_remaining)
    {
      state.phase = Phase::COMPLETE;
      return_value = Error::NO_ERRORS; // May have been BUFFER_FULL but all bytes written
    }
  }
  return return_value;
}
```

#### MessageInterface (nested messages)
```cpp
Error serialize_partial_as_field(...) const override
{
  Error return_value = Error::NO_ERRORS;
  if(Phase::DATA != state.phase)
  {
    return_value = serialize_partial_tag_and_size(field_number, serialized_size(), buffer, state, optional);
  }
  if((Error::NO_ERRORS == return_value) && (Phase::DATA == state.phase))
  {
    // Delegate to child state for nested message content
    const uint32_t initial_size = buffer.get_size();
    if(nullptr != state.child)
    {
      return_value = this->serialize_partial(buffer, *state.child);
    }
    else
    {
      return_value = this->serialize(buffer);
    }
    const uint32_t bytes_written = buffer.get_size() - initial_size;
    state.bytes_remaining -= bytes_written;
    if(0 == state.bytes_remaining)
    {
      state.phase = Phase::COMPLETE;
      return_value = Error::NO_ERRORS;
    }
  }
  return return_value;
}
```

#### RepeatedField (packed and unpacked)
```cpp
Error serialize_partial_as_field(...) const override
{
  if(REPEATED_FIELD_IS_PACKED)
  {
    return serialize_partial_packed(field_number, buffer, state);
  }
  else
  {
    return serialize_partial_unpacked(field_number, buffer, state);
  }
}
```

**Packed**: TAG→SIZE (using helper), then iterate elements from `state.element_index`.

**Unpacked**: Iterate from `state.element_index`, each element calls `element.serialize_partial_as_field(field_number, buffer, state, true)`. On element COMPLETE, increment `element_index`, reset phase to TAG.

## 3. Template Simplification

`Field_SerializePartial.h.jinja2` becomes ~30 lines:

```jinja2
{% if field.uses_serialize_len() %}
{# LENGTH_DELIMITED: call virtual serialize_partial_as_field #}
{% if field.optional or field.oneof is not none %}
if(has_{{field.get_name()}}())
{
  return_value = {{field.get_variable_name()}}.serialize_partial_as_field(
      static_cast<uint32_t>(FieldNumber::{{field.get_variable_id_name()}}), buffer, state, true);
}
else
{
  state.phase = ::EmbeddedProto::Phase::COMPLETE;
}
{% else %}
return_value = {{field.get_variable_name()}}.serialize_partial_as_field(
    static_cast<uint32_t>(FieldNumber::{{field.get_variable_id_name()}}), buffer, state, false);
{% endif %}
{% else %}
{# Scalar/enum: call serialize_partial_with_id on FieldTemplate #}
{% if field.optional or field.oneof is not none %}
if(has_{{field.get_name()}}())
{
  return_value = {{field.get_variable_name()}}.serialize_partial_with_id(
      static_cast<uint32_t>(FieldNumber::{{field.get_variable_id_name()}}), buffer, state, true);
}
else
{
  state.phase = ::EmbeddedProto::Phase::COMPLETE;
}
{% else %}
return_value = {{field.get_variable_name()}}.serialize_partial_with_id(
    static_cast<uint32_t>(FieldNumber::{{field.get_variable_id_name()}}), buffer, state, false);
{% endif %}
{% endif %}
```

## 4. Impact

| Metric | Before | After |
|---|---|---|
| Generated lines per scalar field | ~30 | 1–5 |
| Generated lines per string/msg field | 50–80 | 2–7 |
| Template file size | ~280 lines | ~30 lines |
| 15-field message `serialize_partial` | ~450 lines | ~30 lines |
| Library code | 0 lines | ~150 lines (compiled once) |
| Flash savings | — | ~96% reduction in generated code |
| RAM impact | — | None |
| Performance | — | Negligible (one extra call frame) |

## 5. Files to Modify

| File | Change |
|---|---|
| `src/Fields.h` | Add virtual `serialize_partial_as_field()` on `Field`, protected `serialize_partial_tag_and_size()`, add `serialize_partial_with_id()` on `FieldTemplate` |
| `src/Fields.cpp` | Implement `serialize_partial_tag_and_size()` |
| `src/FieldStringBytes.h` | Override `serialize_partial_as_field()` |
| `src/MessageInterface.h` | Override `serialize_partial_as_field()` |
| `src/MessageInterface.cpp` | Implement `MessageInterface::serialize_partial_as_field()` |
| `src/RepeatedField.h` | Override `serialize_partial_as_field()` |
| `EmbeddedProto/templates/Field_SerializePartial.h.jinja2` | Rewrite to ~30 lines |

## 6. State Machine Reference

See `doc/design/partial_serialization_design.md` Section 5 for the complete state machine diagrams, phase definitions, and per-field-type wire format details. The `MessageState` class and `Phase` enum are defined in `src/MessageState.h`.

Key state fields used:
- `phase`: TAG → SIZE → DATA → COMPLETE
- `bytes_remaining`: tracks DATA phase progress for LENGTH_DELIMITED
- `element_index`: tracks current element for repeated fields
- `child`: pointer to nested message state
