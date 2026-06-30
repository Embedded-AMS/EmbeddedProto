# Protobuf Editions 2023 & 2024 — Feature Analysis for EmbeddedProto

This document analyzes what Protobuf **editions** (`edition = "2023"` /
`edition = "2024"`) add compared to proto3, and what each feature means for
EmbeddedProto in terms of behavior and embedded cost (flash / RAM / CPU). It is
the reference behind the plan to add editions support to the code generator and
runtime.

> Status at time of writing: the generator *detects* editions but only warns and
> falls back to proto3 (`EmbeddedProto/ProtoFile.py`). It already declares
> `FEATURE_SUPPORTS_EDITIONS` (`EmbeddedProto/main.py`) and derives presence solely
> from `proto3_optional` (`EmbeddedProto/Field.py`).

---

## What editions are

- `edition = "2023"` / `"2024"` replaces the old `syntax = "proto2"/"proto3"`
  switch. Behaviors that proto2/proto3 hardcoded become **features** with
  per-edition **defaults**, overridable by lexical scope
  (file → message → field / enum / oneof).
- **The wire format is unchanged.** Editions change which *behaviors* apply, not how
  bytes are laid out — with the single exception of `message_encoding`, which
  selects between two wire framings that already exist in protobuf.
- **Editions are cumulative.** Edition 2024 supports every feature of 2023 (only
  some defaults change) plus its own additions. A 2024 file therefore must receive
  all 2023 feature handling.
- **Non-C++ plugins must resolve features themselves.** The descriptor sent to a
  plugin carries only the *explicitly-set overrides* (`options.features`), not the
  resolved values. Resolution = walk the scope chain and fall back to the edition's
  defaults.

---

## Edition 2023 — six core features (+ custom default values)

| Feature | Values (2023 default in **bold**) | Meaning for EmbeddedProto | Flash | RAM | CPU |
|---|---|---|---|---|---|
| `field_presence` | **EXPLICIT** / IMPLICIT / LEGACY_REQUIRED | EXPLICIT → track presence + generate `has_*()` (reuse the existing `presence_[]` bitfield). IMPLICIT → today's proto3 non-optional behavior. **In 2023 plain scalars are EXPLICIT by default** (proto3 made them IMPLICIT). LEGACY_REQUIRED → always present/serialized. | small (extra `has_/set_/clear_` inlines) | **+1 bit per explicit field** (rounded up to uint32 words) — the main cost | negligible (bit test/set) |
| `enum_type` | **OPEN** / CLOSED | OPEN = today's behavior (any received int is stored). CLOSED = validate the received value against the declared enumerators and **drop unknown values** (EmbeddedProto keeps no unknown-field set, so an unknown value leaves the field unset). | small check per CLOSED enum decode | none | a few compares per enum value decoded |
| `repeated_field_encoding` | **PACKED** / EXPANDED | Selects which form to *emit*; both decode paths already exist. Read the resolved feature instead of hardcoding by field type. | ~none | none | none |
| `utf8_validation` | **VERIFY** / NONE | EmbeddedProto performs no UTF-8 validation. **Treated as NONE-equivalent and documented.** Implementing VERIFY would add per-byte CPU cost. | avoided | — | avoided |
| `message_encoding` | **LENGTH_PREFIXED** / DELIMITED | See the deep-dive below. DELIMITED = group framing = **single-pass encode** (no size pre-pass). | new ser/deser path | none | encode win; slight decode cost |
| `json_format` | **ALLOW** / LEGACY_BEST_EFFORT | EmbeddedProto has no JSON support. **Ignored.** | none | none | none |
| custom default values | re-enabled in editions | `int32 x = 1 [default = 42];` — emit the literal in the member initializer and in the "serialize only if value ≠ default" check. The value is already in the descriptor (`default_value`). | negligible (a constant; string/bytes default = a literal + an init copy) | none extra | same as today's `!= 0` check |

**Net cost for 2023:** the only non-trivial *runtime* costs are (a) a modest RAM
bump because more fields get a presence bit by default, and (b) small flash/CPU for
CLOSED-enum validation. No new wire-format machinery is needed beyond
`message_encoding`.

---

## Edition 2024 — cumulative, with ~zero extra runtime cost

Edition 2024 is a **superset** of 2023 (same six features, only minor default
tweaks) plus the following additions. For EmbeddedProto these are codegen / naming /
import concerns, not wire behavior:

| Addition in 2024 | What it is | Impact on EmbeddedProto |
|---|---|---|
| `default_symbol_visibility` | `export` / `local` keywords controlling cross-file symbol visibility | Codegen name-resolution only — **zero runtime cost** |
| `enforce_naming_style` | Strict naming, enforced by **protoc itself** | **Zero** — the generator does nothing |
| `ctype` removed → `features.(pb.cpp).string_type` | std::string vs string_view vs Cord | **Irrelevant** — EmbeddedProto uses its own fixed-size strings; the feature is ignored |
| `import option`, `import weak` removed, `java_multiple_files` removed | import / option cleanups | Minor import handling; weak imports and Java options were never supported. **~zero** |

Because editions are cumulative, a 2024 file automatically inherits **all** 2023
feature handling; only these deltas are new.

---

## Deep-dive: `message_encoding` and the single-pass performance win

A nested (sub-)message field has two interchangeable wire framings carrying
identical data:

```
LENGTH_PREFIXED:  1A 03 08 96 01     tag(field 3, LEN) | length=3 | <Inner bytes>
DELIMITED:        1B 08 96 01 1C     START_GROUP(3) | <Inner fields inline> | END_GROUP(3)
```
(both encode `Inner sub = 3;` with `int32 x = 1 = 150`; the START and END tags
carry the same field number.)

- **LENGTH_PREFIXED** needs the sub-message's byte length *before* the bytes. Today
  EmbeddedProto does a **size pre-pass** (`MessageSizeCalculator` walks the entire
  sub-tree), writes the length varint, then **re-walks** the sub-tree to emit bytes.
  Because this recurses, a parent's size pass re-walks children that are then
  re-walked on the encode pass — a real, repeated cost.
- **DELIMITED** needs **no length** — emit the START_GROUP tag, stream the
  sub-message's fields once, emit the END_GROUP tag. **The size pre-pass disappears,
  giving single-pass encoding.**

Implications:

- **Opt-in via the schema, not unilateral.** The author sets
  `features.message_encoding = DELIMITED` (at field, message, or file scope). Setting
  it file-wide makes every nested message single-pass. EmbeddedProto cannot silently
  change the wire format because both peers must agree via the shared schema.
- **Both encode and decode are required.** A device that emits groups will also
  receive them, so the deserializer must parse "read fields until the matching
  END_GROUP," and the unknown-field skipper must skip groups recursively to their
  END_GROUP.
- **The partial-serialization state machine needs a group path.**
  `src/MessageState.h` has explicit `TAG → SIZE → DATA` phases that assume a length
  prefix; groups have no SIZE phase and need group-depth tracking alongside the
  existing `MessageStateStack<DEPTH>` / `NESTING_TOO_DEEP` logic.
- **Honest trade-off.** This is an *encode* win (and avoids needing the size up front
  for streaming). Decoding a group is marginally slower than length-prefixed because
  the reader scans for END_GROUP instead of jumping by a known length. For a device
  that mostly *emits* telemetry, that is an excellent trade.

---

## Architecture note: per-edition selection, cumulative handling

To reconcile "a 2023 file should get only 2023 semantics" with "2024 must support
all 2023 features":

- One shared set of **feature handlers** (presence, enum, repeated, defaults,
  message_encoding) implements the actual codegen/runtime logic.
- A per-edition **profile** object supplies, for a given file, (1) the correct
  **default** value of each feature for that edition and (2) which features/values
  are legal in that edition.

A 2023 file then gets only 2023 defaults (it can never pull a 2024-only behavior),
while a 2024 file inherits all 2023 handling plus the 2024 deltas — because they
share the handlers. This mirrors protoc's own model, which rejects 2024-only
features in a 2023 file.

---

## Scope summary for EmbeddedProto

**Supported / planned:**
- `field_presence`: EXPLICIT (→ `has_*()`), IMPLICIT, LEGACY_REQUIRED
  (always-serialize, no absence enforcement).
- Custom (non-zero) default values.
- `repeated_field_encoding`: PACKED and EXPANDED override.
- `enum_type`: CLOSED (validate-and-drop unknown values) and OPEN.
- `message_encoding`: full DELIMITED support (encode + decode + partial state
  machine) as the single-pass performance option.
- Edition 2024 accepted (cumulative on 2023; its additions are codegen-only).

**Explicit limitations (documented, not implemented):**
- `utf8_validation = VERIFY` — treated as NONE (no validation performed).
- Unknown-field round-tripping — not supported (EmbeddedProto discards unknown
  fields), which is why CLOSED enums *drop* rather than *preserve* unknown values.
