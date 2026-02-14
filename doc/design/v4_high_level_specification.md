# EmbeddedProto v4 High-Level Specification

## Document Information
- **Version**: 0.2 (Draft)
- **Status**: Under Review
- **Last Updated**: Session 2 - Detailed Design Phase

---

## 1. Executive Summary

EmbeddedProto v4 is a major evolution of the library aimed at supporting advanced features while maintaining the core principles of minimal resource usage. The key additions are:

1. **Partial (De)serialization** - Ability to serialize/deserialize messages in chunks with external state
2. **Callbacks** - Event-driven processing for streaming and memory-constrained scenarios
3. **Protobuf Editions Support** - Fine-grained feature control per field (field_presence, packed encoding, etc.)
4. **Maps** - Static implementation of protobuf map type
5. **License System** - Usage tracking and customer identification at code generation time

---

## 2. Design Principles

### 2.1 Core Principles (Unchanged from v3)
1. **No Dynamic Memory Allocation** - All memory statically allocated at compile time
2. **Minimal Flash Usage** - Code size is the highest priority
3. **Minimal RAM Usage** - Data size is very important
4. **Predictable Behavior** - No runtime surprises
5. **Wire Format Compatibility** - Must be compatible with standard protobuf

### 2.2 New Principles for v4
1. **Opt-in Features** - New features should not penalize users who don't use them
2. **Compiler Optimization Friendly** - Unused code paths should be eliminable by the compiler
3. **External State** - State for partial operations stored outside message objects
4. **Backward Compatible API** - Existing `serialize()`/`deserialize()` methods preserved

### 2.3 Priority Ranking
1. Flash size (code size) - **Highest**
2. RAM usage (data size) - **Very Important**
3. CPU cycles (speed) - **Less Important**
4. API simplicity - **Nice to Have** (but important for adoption)

---

## 3. Target Metrics

| Metric | Current (v3) | Target (v4) | Notes |
|--------|--------------|-------------|-------|
| Base library size | ~14 KB | ≤14 KB | No regression for basic usage |
| With all features | N/A | ≤18 KB | Only if features actively used |
| Minimum buffer size | N/A | 8 bytes | For partial (de)serialization |
| C++ Standard | C++14 | C++17 | Enables `if constexpr`, etc. |

---

## 4. Feature Specifications

### 4.1 Partial (De)serialization

**Priority**: 1 (Highest)  
**Status**: Must-Have  

#### 4.1.1 Purpose
Allow serialization and deserialization of messages in chunks, enabling:
- Use of small buffers (e.g., CAN bus with 8 bytes, CAN-FD with 64 bytes)
- Multitasking (yield CPU between chunks)
- Lower peak RAM usage for large messages

#### 4.1.2 API Design

```cpp
// Current API (unchanged - backward compatible)
Error serialize(WriteBufferInterface& buffer) const;
Error deserialize(ReadBufferInterface& buffer);

// New partial API - opt-in, external state
Error serialize_partial(WriteBufferInterface& buffer, 
                       SerializationStateBase& state) const;
Error deserialize_partial(ReadBufferInterface& buffer, 
                         DeserializationStateBase& state);

// State reset methods
void reset_serialize_state(SerializationStateBase& state) const;
void reset_deserialize_state(DeserializationStateBase& state);
```

#### 4.1.3 Nested State Structure

**Key Insight**: For nested messages (A contains B contains C), each level needs its own state. When serializing A and the buffer fills while writing C, we need:
- State for A: which field (B) we're currently serializing
- State for B: which field (C) we're currently serializing
- State for C: which byte/element within C we're writing

**Implementation**:

```cpp
namespace EmbeddedProto {

// Base state class for type-erased storage
class SerializationStateBase {
public:
    enum class Phase : uint8_t {
        FIELD_TAG,       // Writing/reading field tag
        FIELD_SIZE,      // Writing/reading length prefix (for LENGTH_DELIMITED)
        FIELD_DATA,      // Writing/reading actual field data
        COMPLETE         // Done with this level
    };
    
    Phase phase = Phase::FIELD_TAG;
    uint32_t field_index = 0;        // Index of current field in message
    uint32_t bytes_processed = 0;    // Bytes written/read in current phase
    Error last_error = Error::NO_ERRORS;
    
    // For nested messages: pointer to child state (null if not nested)
    SerializationStateBase* child = nullptr;
    
protected:
    SerializationStateBase() = default;
};

// Deserialization state (similar structure)
class DeserializationStateBase {
public:
    enum class Phase : uint8_t {
        TAG,              // Reading field tag
        SIZE,             // Reading length prefix
        DATA,             // Reading field data
        COMPLETE
    };
    
    Phase phase = Phase::TAG;
    uint32_t field_id = 0;           // Field ID from tag
    WireFormatter::WireType wire_type = WireFormatter::WireType::VARINT;
    uint32_t bytes_remaining = 0;    // Bytes left to read for current field
    Error last_error = Error::NO_ERRORS;
    
    DeserializationStateBase* child = nullptr;
    
protected:
    DeserializationStateBase() = default;
};

} // namespace EmbeddedProto
```

#### 4.1.4 State Stack Helper

For automatic state management with nested messages:

```cpp
template<size_t MAX_DEPTH = 4>
class SerializationStateStack {
    std::array<SerializationStateBase, MAX_DEPTH> states_;
    uint8_t current_depth_ = 1;
    
public:
    SerializationStateStack() {
        // Link states together
        for (size_t i = 0; i < MAX_DEPTH - 1; ++i) {
            states_[i].child = &states_[i + 1];
        }
    }
    
    SerializationStateBase& root() { return states_[0]; }
    SerializationStateBase& current() { return states_[current_depth_ - 1]; }
    
    void push() { if (current_depth_ < MAX_DEPTH) ++current_depth_; }
    void pop() { if (current_depth_ > 1) --current_depth_; }
    void reset() { 
        current_depth_ = 1;
        for (auto& s : states_) {
            s.phase = SerializationStateBase::Phase::FIELD_TAG;
            s.field_index = 0;
            s.bytes_processed = 0;
            s.last_error = Error::NO_ERRORS;
        }
    }
    
    size_t depth() const { return current_depth_; }
};
```

#### 4.1.5 Usage Example

```cpp
// Message hierarchy: OuterMsg → InnerMsg → int32 field
OuterMsg msg;
WriteBufferFixedSize<32> buffer;  // Small buffer

// Create state stack for 3 levels of nesting
SerializationStateStack<3> state_stack;

// Serialize with partial support
Error err = msg.serialize_partial(buffer, state_stack.root());

while (Error::BUFFER_FULL == err) {
    // Buffer ran out - send data and get new buffer
    send_data(buffer);
    buffer.clear();
    
    // Resume serialization from where we stopped
    err = msg.serialize_partial(buffer, state_stack.root());
}

if (Error::NO_ERRORS == err) {
    // Final chunk
    send_data(buffer);
}
```

#### 4.1.6 Generated Code Changes

Each message type will have generated partial serialization methods:

```cpp
class MyMessage : public MessageInterface {
public:
    // Standard serialize (unchanged)
    Error serialize(WriteBufferInterface& buffer) const override;
    
    // Partial serialize with state machine
    Error serialize_partial(WriteBufferInterface& buffer, 
                           SerializationStateBase& state) const override;
    
    // Calculate required state depth for this message type
    static constexpr uint32_t required_state_depth() { return 3; }
    
private:
    // Internal helper for each field
    Error serialize_field_partial(uint32_t field_index, 
                                  WriteBufferInterface& buffer,
                                  SerializationStateBase& state) const;
};
```

#### 4.1.7 State Machine for Field Serialization

```
┌─────────────────────────────────────────────────────────────┐
│                    FIELD_TAG Phase                          │
│  - Write tag (field_number << 3 | wire_type)               │
│  - If LENGTH_DELIMITED: → FIELD_SIZE                        │
│  - Else: → FIELD_DATA                                       │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    FIELD_SIZE Phase                         │
│  - Write varint size (may be partial)                       │
│  - Track bytes_processed                                    │
│  - When complete: → FIELD_DATA                              │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                    FIELD_DATA Phase                         │
│  - Write field data (may be partial)                        │
│  - For nested messages: recurse with child state            │
│  - For repeated: track element_index                        │
│  - When complete: → next field (FIELD_TAG)                  │
└─────────────────────────────────────────────────────────────┘
```

---

### 4.2 Callbacks

**Priority**: 2  
**Status**: Must-Have

#### 4.2.1 Purpose
Enable event-driven processing for:
- Processing repeated field elements without storing all in memory
- Streaming data in/out during serialization
- Notifying application when fields are received

#### 4.2.2 Callback Class Design (Inspired by mbed-os Callback)

```cpp
namespace EmbeddedProto {

// Base storage for type-erased callback
// Stores function pointer + object pointer (for member functions)
// or small lambdas (up to ~16 bytes)
class CallbackBase {
protected:
    struct Storage {
        void* obj_ptr;           // Object pointer (for member functions)
        void* func_ptr;          // Function pointer
        uintptr_t padding[2];    // Additional storage for captures
    } storage_{};
    
    using InvokerPtr = void(*)(Storage&, void* result, void** args);
    InvokerPtr invoker_ = nullptr;
    
public:
    CallbackBase() = default;
    bool valid() const { return invoker_ != nullptr; }
    void reset() { invoker_ = nullptr; }
};

// Specialized callback template
template<typename Signature>
class Callback;

template<typename R, typename... Args>
class Callback<R(Args...)> : private CallbackBase {
public:
    Callback() = default;
    Callback(std::nullptr_t) : CallbackBase() {}
    
    // Construct from function pointer
    template<typename F, typename = std::enable_if_t<std::is_function_v<std::remove_pointer_t<F>>>>
    Callback(F f) {
        storage_.func_ptr = reinterpret_cast<void*>(f);
        invoker_ = &invoke_func<F>;
    }
    
    // Construct from member function + object
    template<typename T, typename M>
    Callback(T* obj, M mem_fn) {
        storage_.obj_ptr = obj;
        storage_.func_ptr = reinterpret_cast<void*>(mem_fn);
        invoker_ = &invoke_member<T, M>;
    }
    
    // Construct from small lambda/functor
    template<typename F, typename = std::enable_if_t<
        !std::is_function_v<std::remove_pointer_t<F>> &&
        !std::is_member_pointer_v<F> &&
        sizeof(F) <= sizeof(Storage)
    >>
    Callback(F&& f) {
        new (&storage_) F(std::forward<F>(f));
        invoker_ = &invoke_functor<std::decay_t<F>>;
    }
    
    // Call the callback
    R operator()(Args... args) {
        if (!invoker_) {
            if constexpr (!std::is_void_v<R>) {
                return R{};
            } else {
                return;
            }
        }
        void* arg_ptrs[] = { &args... };
        if constexpr (std::is_void_v<R>) {
            invoker_(storage_, nullptr, arg_ptrs);
        } else {
            R result;
            invoker_(storage_, &result, arg_ptrs);
            return result;
        }
    }
    
    using CallbackBase::valid;
    using CallbackBase::reset;
    
private:
    // Invoker implementations (omitted for brevity)
    template<typename F> static void invoke_func(Storage& s, void* result, void** args);
    template<typename T, typename M> static void invoke_member(Storage& s, void* result, void** args);
    template<typename F> static void invoke_functor(Storage& s, void* result, void** args);
};

} // namespace EmbeddedProto
```

#### 4.2.3 Callback Types for EmbeddedProto

```cpp
// Serialization event callback
using SerializeCallback = Callback<void(uint32_t field_number, Error result)>;

// Deserialization event callback
using DeserializeCallback = Callback<void(uint32_t field_number, Error result)>;

// Repeated field element callback (deserialization) - return true to continue
template<typename T>
using ElementCallback = Callback<bool(const T& element)>;

// Repeated field element source (serialization) - return false when done
template<typename T>
using ElementSourceCallback = Callback<bool(T& element)>;
```

#### 4.2.4 Usage Examples

```cpp
// Example 1: Process measurements without storing all
struct MeasurementProcessor : ElementCallback<Measurement> {
    bool operator()(const Measurement& m) override {
        process_measurement(m);  // Handle immediately
        return true;  // Continue receiving
    }
};

// Example 2: Stream data from external source
struct DataSource : ElementSourceCallback<SensorReading> {
    SensorDataSource* source;
    bool operator()(SensorReading& element) override {
        return source->read_next(element);
    }
};

// Example 3: Lambda callback
msg.set_serialize_callback([](uint32_t field_num, Error err) {
    if (err != Error::NO_ERRORS) {
        log_error("Field %u failed: %d", field_num, err);
    }
});
```

#### 4.2.5 Integration with Fields

```cpp
template<typename DATA_TYPE>
class RepeatedField : public Field {
public:
    // Set callback for processing elements during deserialization
    void set_element_callback(ElementCallback<DATA_TYPE>* cb) { 
        element_callback_ = cb; 
    }
    
    // Set source for streaming elements during serialization
    void set_element_source(ElementSourceCallback<DATA_TYPE>* src) { 
        element_source_ = src; 
    }
    
    // When callback is set, deserialize calls callback instead of storing
    Error deserialize(ReadBufferInterface& buffer) override {
        if (element_callback_) {
            DATA_TYPE temp;
            Error err = temp.deserialize(buffer);
            if (err == Error::NO_ERRORS) {
                (*element_callback_)(temp);
            }
            return err;
        }
        // Normal path: add to array
        // ...
    }
    
private:
    ElementCallback<DATA_TYPE>* element_callback_ = nullptr;
    ElementSourceCallback<DATA_TYPE>* element_source_ = nullptr;
};
```

---

### 4.3 Protobuf Editions Support

**Priority**: 3  
**Status**: Must-Have

#### 4.3.1 Background

Protobuf Editions (2023, 2024, ...) replace `syntax = "proto2/proto3"` with `edition = "2023"`. Features that were implicit in proto2/proto3 are now explicit per-field options.

#### 4.3.2 Key Features to Support

| Feature | Values | Impact on EmbeddedProto |
|---------|--------|------------------------|
| `field_presence` | EXPLICIT, IMPLICIT, LEGACY_REQUIRED | Controls `has_X()` methods and serialization |
| `repeated_field_encoding` | PACKED, EXPANDED | Whether repeated fields use packed encoding |
| `enum_type` | OPEN, CLOSED | How unknown enum values are handled |
| `utf8_validation` | VERIFY, NONE | String validation |
| `message_encoding` | LENGTH_PREFIXED, DELIMITED | Group encoding (legacy) |

#### 4.3.3 Field Presence Feature

```cpp
// IMPLICIT (proto3 default): No presence tracking
class Field_IMPLICIT {
    int32_t value_ = 0;  // Default value
    // No has() method
    // Serialize: skip if value == 0
};

// EXPLICIT (proto2 default, proto3 optional): Presence tracking
class Field_EXPLICIT {
    int32_t value_ = 0;
    bool has_value_ = false;  // Presence flag
    bool has_value() const { return has_value_; }
    // Serialize: always serialize if has_value_ is true
};

// LEGACY_REQUIRED (proto2 required): Must be set
class Field_LEGACY_REQUIRED {
    int32_t value_ = 0;
    bool has_value_ = false;
    // Serialize: ERROR if not set
    Error serialize(...) const {
        if (!has_value_) return Error::REQUIRED_FIELD_NOT_SET;
        // ...
    }
};
```

#### 4.3.4 Generated Code Changes

```cpp
// Generated message with Editions features
class MyMessage : public MessageInterface {
public:
    // Field features (generated from .proto)
    struct FieldFeatures {
        static constexpr FieldPresence field_a_presence = FieldPresence::EXPLICIT;
        static constexpr FieldPresence field_b_presence = FieldPresence::IMPLICIT;
        static constexpr RepeatedEncoding field_c_encoding = RepeatedEncoding::PACKED;
    };
    
    // Fields with presence tracking (only for EXPLICIT/LEGACY_REQUIRED)
    int32_t field_a_ = 0;
    bool has_field_a_ = false;  // Only generated for EXPLICIT fields
    
    int32_t field_b_ = 0;  // IMPLICIT - no presence tracking
    
    // has_X() methods only for EXPLICIT fields
    bool has_field_a() const { return has_field_a_; }
    
    // clear() resets presence flags
    void clear() override {
        field_a_ = 0;
        has_field_a_ = false;
        field_b_ = 0;
    }
};
```

#### 4.3.5 Python Generator Changes

```python
# Field.py additions
class FieldFeatures:
    def __init__(self):
        self.field_presence = "EXPLICIT"  # Default for editions
        self.repeated_encoding = "PACKED"
        self.enum_type = "OPEN"
        self.utf8_validation = "VERIFY"

class Field:
    def __init__(self, ...):
        # ...
        self.features = FieldFeatures()
    
    def parse_edition_features(self, descriptor):
        # Extract features from field options
        if descriptor.options.HasExtension("field_presence"):
            self.features.field_presence = descriptor.options.field_presence
        # ...
```

---

### 4.4 Maps

**Priority**: 4  
**Status**: Nice-to-Have

#### 4.4.1 Wire Format

Maps are encoded as repeated message entries:
```protobuf
map<string, int32> my_map = 1;

// Equivalent wire format:
message MapEntry {
    string key = 1;
    int32 value = 2;
}
repeated MapEntry my_map = 1;
```

#### 4.4.2 Implementation

```cpp
namespace EmbeddedProto {

template<typename KEY_TYPE, typename VALUE_TYPE, uint32_t MAX_SIZE>
class MapField : public Field {
public:
    struct Entry {
        KEY_TYPE key;
        VALUE_TYPE value;
    };
    
private:
    std::array<Entry, MAX_SIZE> entries_;
    uint32_t size_ = 0;
    
public:
    // Map-like interface
    Error insert(const KEY_TYPE& key, const VALUE_TYPE& value);
    Error insert_or_assign(const KEY_TYPE& key, const VALUE_TYPE& value);
    const VALUE_TYPE* find(const KEY_TYPE& key) const;
    VALUE_TYPE* find(const KEY_TYPE& key);
    bool contains(const KEY_TYPE& key) const;
    bool erase(const KEY_TYPE& key);
    void clear();
    uint32_t size() const { return size_; }
    constexpr uint32_t max_size() const { return MAX_SIZE; }
    
    // Iteration
    const Entry* begin() const { return entries_.data(); }
    const Entry* end() const { return entries_.data() + size_; }
    
    // Field interface
    Error serialize(WriteBufferInterface& buffer) const override;
    Error deserialize(ReadBufferInterface& buffer) override;
    Error serialize_partial(WriteBufferInterface& buffer, 
                           SerializationStateBase& state) const override;
    Error deserialize_partial(ReadBufferInterface& buffer,
                             DeserializationStateBase& state) override;
    void clear() override;
    uint32_t serialized_size() const override;
    
private:
    // Linear search (O(n)) - acceptable for small maps
    Entry* find_entry(const KEY_TYPE& key);
    const Entry* find_entry(const KEY_TYPE& key) const;
};

} // namespace EmbeddedProto
```

#### 4.4.3 Limitations (Acceptable per protobuf spec)
- Fixed maximum size (template parameter)
- Keys limited to scalar types and strings
- No ordering guarantees (wire format ordering is undefined)
- Linear search O(n) - acceptable for small maps

---

### 4.5 License System

**Priority**: 5  
**Status**: Must-Have

#### 4.5.1 Purpose
- Identify paying customers in generated code
- Track usage statistics
- Differentiate open source vs commercial usage

#### 4.5.2 Check Location
- Code generation time (protoc plugin)
- NOT at compile time or runtime

#### 4.5.3 Requirements
- Online license server for validation
- Offline mode with local key file
- Graceful fallback to open source license on failure

#### 4.5.4 Generated Code Difference
- Commercial: Custom header with customer identification
- Open source: Standard GPL license header

#### 4.5.5 Security Considerations
- Python code is public, obfuscation limited
- Consider separate private pip package for license module
- Accept that determined users can bypass

#### 4.5.6 Implementation Approach

```python
# license_manager.py (separate private package)
class LicenseManager:
    def __init__(self):
        self.license_key = None
        self.customer_info = None
    
    def check_license(self, key_file=None):
        """
        Check license validity.
        1. Try online validation first
        2. Fall back to local key file if offline
        3. Return GPL license info if all fail
        """
        if self._try_online_validation():
            return LicenseType.COMMERCIAL, self.customer_info
        elif key_file and self._validate_local_key(key_file):
            return LicenseType.COMMERCIAL, self.customer_info
        else:
            return LicenseType.OPENSOURCE, None
    
    def generate_header(self, license_type, customer_info):
        """Generate appropriate header for generated code."""
        if license_type == LicenseType.COMMERCIAL:
            return f"""// Generated by EmbeddedProto Commercial License
// Licensed to: {customer_info['name']}
// License ID: {customer_info['license_id']}
// Valid until: {customer_info['expiry_date']}
"""
        else:
            return """// Generated by EmbeddedProto (Open Source)
// Licensed under GPL v3.0
// https://www.gnu.org/licenses/gpl-3.0.txt
"""
```

#### 4.5.7 License Server Architecture

```
┌─────────────────┐     HTTPS      ┌─────────────────┐
│  protoc plugin  │ ──────────────▶│  License Server │
│  (EmbeddedProto)│                │  (AWS/GCP/Azure)│
└─────────────────┘                └─────────────────┘
        │                                   │
        │ Offline fallback                  │
        ▼                                   ▼
┌─────────────────┐                ┌─────────────────┐
│  Local Key File │                │  License DB     │
│  (.embeddedproto│                │  (customers,    │
│   /license.key) │                │   usage stats)  │
└─────────────────┘                └─────────────────┘
```

#### 4.5.8 Open Questions
- **License server architecture** - Separate pip package or embedded?
- **Key format** - JWT tokens vs custom format?
- **Usage tracking granularity** - Per-project or per-organization?

---

## 5. Architecture Changes

### 5.1 Class Hierarchy

#### 5.1.1 Current Hierarchy
```
Field (base)
├── FieldTemplate<...>  (basic types)
├── MessageInterface : Field
├── RepeatedField<DATA_TYPE> : Field
└── FieldStringBytes : Field
```

#### 5.1.2 Proposed Hierarchy

```
// New base for anything serializable
class Serializable {
public:
    virtual ~Serializable() = default;
    virtual Error serialize(WriteBufferInterface& buffer) const = 0;
    virtual Error deserialize(ReadBufferInterface& buffer) = 0;
    virtual void clear() = 0;
};

// Field base - adds field number and wire type awareness
class Field : public Serializable {
public:
    virtual Error serialize_with_id(uint32_t field_number, 
                                    WriteBufferInterface& buffer, 
                                    bool optional) const = 0;
    virtual Error deserialize_check_type(ReadBufferInterface& buffer,
                                         WireFormatter::WireType wire_type) = 0;
    virtual uint32_t serialized_size() const = 0;
};

// Message interface - adds message-specific features
class MessageInterface : public Field {
public:
    // Partial serialization with external state
    virtual Error serialize_partial(WriteBufferInterface& buffer,
                                   SerializationStateBase& state) const = 0;
    virtual Error deserialize_partial(ReadBufferInterface& buffer,
                                     DeserializationStateBase& state) = 0;
    
    // Callback support (optional)
    void set_serialize_callback(SerializeCallback* cb) { serialize_cb_ = cb; }
    void set_deserialize_callback(DeserializeCallback* cb) { deserialize_cb_ = cb; }
    
protected:
    SerializeCallback* serialize_cb_ = nullptr;
    DeserializeCallback* deserialize_cb_ = nullptr;
};
```

### 5.2 Backward Compatibility

| Feature | Breaking? | Mitigation |
|---------|-----------|------------|
| New `Serializable` base class | No | `MessageInterface` still inherits from `Field` |
| `serialize_partial()` method | No | New method, opt-in |
| External state | No | Existing internal state removed only if using partial API |
| Callback pointers | No | Optional feature, null by default |
| Editions features | No | Default to proto3 behavior |

---

## 6. Implementation Phases

### Phase 1: Foundation (2-3 weeks)
- [ ] Add `SerializationStateBase` and `DeserializationStateBase` classes
- [ ] Add `FieldFeatures` struct for editions features
- [ ] Update `Error` enum with new error codes
- [ ] Create unit test framework for partial serialization

### Phase 2: Partial Serialization (3-4 weeks)
- [ ] Implement `serialize_partial` for `FieldTemplate` (basic types)
- [ ] Implement for `FieldStringBytes`
- [ ] Implement for `RepeatedField`
- [ ] Implement for `MessageInterface`
- [ ] Update Python code generator

### Phase 3: Partial Deserialization (2-3 weeks)
- [ ] Implement `deserialize_partial` for all field types
- [ ] Handle nested message state stacking
- [ ] Update Python code generator

### Phase 4: Editions Support (3-4 weeks)
- [ ] Update Python parser for edition syntax
- [ ] Implement feature extraction from field options
- [ ] Generate feature-aware code
- [ ] Implement `field_presence` feature (EXPLICIT/IMPLICIT/LEGACY_REQUIRED)
- [ ] Implement `repeated_field_encoding` feature (PACKED/EXPANDED)
- [ ] Implement `enum_type` feature (OPEN/CLOSED)

### Phase 5: Map Support (2 weeks)
- [ ] Implement `MapField` template class
- [ ] Add map field detection in Python generator
- [ ] Generate map typedefs
- [ ] Add tests

### Phase 6: Callbacks (2-3 weeks)
- [ ] Implement `Callback` template class
- [ ] Add callback support to `RepeatedField`
- [ ] Add callback support to `MessageInterface`
- [ ] Update code generator for callback-enabled messages
- [ ] Add examples and documentation

### Phase 7: Integration & Testing (2 weeks)
- [ ] Comprehensive test suite
- [ ] Performance benchmarks
- [ ] Documentation update
- [ ] Example projects

---

## 7. Testing Strategy

### 7.1 Unit Tests
- Maintain ~95% coverage
- Add tests for all new features
- Test partial (de)serialization with various buffer sizes (8, 16, 32, 64 bytes)

### 7.2 Integration Tests
- Test against standard protobuf implementations (Python, C++)
- Verify wire format compatibility
- Test Editions features against reference implementations

### 7.3 Size Tests
- Automated size measurement
- Regression detection for code size increases
- Compare with v3 baseline

---

## 8. Open Questions

1. **State ownership**: Should state objects be templated on message type for type safety, or use type erasure for flexibility? → **Decision: Type erasure via base class**

2. **Callback storage**: By pointer (smaller messages, user manages lifetime) or by value? → **Decision: By pointer**

3. **Map implementation**: Linear search vs sorted array? → **Decision: Linear search (acceptable for small maps per protobuf spec)**

4. **Editions default behavior**: Default to proto3 or edition 2023? → **Decision: Default to proto3 for backward compatibility**

5. **License server architecture**: Separate pip package or embedded? → **Open: Needs further discussion**

6. **License key format**: JWT tokens vs custom format? → **Open: Needs further discussion**

7. **Usage tracking granularity**: Per-project or per-organization? → **Open: Needs further discussion**

---

## Appendix A: Error Codes

New error codes to add to `Errors.h`:

```cpp
enum class Error {
    NO_ERRORS = 0,
    END_OF_BUFFER = 1,
    BUFFER_FULL = 2,
    INVALID_WIRETYPE = 3,
    ARRAY_FULL = 4,
    INVALID_FIELD_ID = 5,
    OVERLONG_VARINT = 6,
    INDEX_OUT_OF_BOUND = 7,
    // New errors for v4:
    REQUIRED_FIELD_NOT_SET = 8,   // For LEGACY_REQUIRED fields
    CALLBACK_FAILED = 9,          // Callback returned false
    STATE_MISMATCH = 10,          // State doesn't match message type
    MAP_KEY_NOT_FOUND = 11,       // Map operation failed
    MAP_DUPLICATE_KEY = 12,       // Insert with existing key
};
```

---

## Appendix B: References

- [Protobuf Wire Format](https://protobuf.dev/programming-guides/encoding/)
- [Protobuf Editions Overview](https://protobuf.dev/editions/overview/)
- [Protobuf Editions Features](https://protobuf.dev/editions/features/)
- [Protobuf Maps](https://protobuf.dev/programming-guides/proto3/#maps)
- [MbedOS Callback Implementation](https://github.com/ARMmbed/mbed-os/blob/master/platform/include/platform/Callback.h)

---

## Appendix C: Design Decisions Log

| Date | Decision | Rationale |
|------|----------|-----------|
| Session 2 | External state objects | Enables multiple concurrent streams, no message size overhead |
| Session 2 | Nested state via child pointers | Matches message hierarchy, natural recursion |
| Session 2 | Callback by pointer | Minimizes message size, user manages lifetime |
| Session 2 | Linear search for maps | Acceptable for small maps, matches protobuf spec (unordered) |
| Session 2 | Type-erased state base | Flexibility for generic handling, template for type safety |