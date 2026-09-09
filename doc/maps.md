# Map fields

EmbeddedProto supports protobuf `map<K, V>` fields. This page shows how to declare
one, how to size it, and how to use the generated API.

The bytes on the wire are exactly those of any other protobuf implementation, so a
map written by EmbeddedProto is read by a standard `protoc` generated peer and the
other way around.

---

## Declaring a map

```proto
syntax = "proto3";

import "embedded_proto_options.proto";

message Device
{
  map<string, int32> readings = 1 [(EmbeddedProto.options) = {
    maxLength: 8,
    keyMaxLength: 16
  }];
}
```

EmbeddedProto allocates statically, so the map needs a compile time bound:

| Option | Meaning |
|---|---|
| `maxLength` | The number of entries the map can hold. |
| `keyMaxLength` | The length of the key, for a `string` key. |
| `valueMaxLength` | The length of the value, for a `string` or `bytes` value. |

`keyMaxLength` and `valueMaxLength` are separate options because a
`map<string, string>` has two independent string lengths to bound. Set only the
ones that apply: `keyMaxLength` on a map with an integral key is an error, and so is
`valueMaxLength` on a map whose value is not a string or bytes.

When an option is omitted the size becomes a C++ template parameter instead, the
same as for any other field:

```cpp
// map<string, int32> readings = 1;  without any options
Device<8, 16> device;  // number of entries, key length
```

Every option can also be set from the
[external options file](../README.md) instead of in the `.proto` itself.

Any key protobuf allows is supported (an integral type, `bool` or `string`) and so
is any value type: scalars, enumerations, `string`, `bytes` and nested messages.

---

## Using a map

```cpp
Device device;

// Insert or replace.
device.set_readings("kitchen", 21);
device.set_readings("garden", 14);
device.set_readings("kitchen", 22);   // replaces, does not add a second entry

device.readings_size();               // 2
device.readings_max_size();           // 8
device.has_readings("kitchen");       // true
device.get_readings("kitchen");       // 22
device.get_readings("cellar");        // 0, the default value of the value type

device.remove_readings("garden");
device.clear_readings();
```

`set_` returns an `Error`. It is `ARRAY_FULL` when the key is new and the map is
already full; replacing the value of a key that is present always succeeds.

### A key or value that does not fit

A string key longer than `keyMaxLength`, or a string value longer than
`valueMaxLength`, is refused with `ARRAY_FULL` and the map is left exactly as it
was. This is deliberate: a plain string field stores such a string cut short, but a
cut short key could never be found again under the name you gave, and a cut short
value would silently differ from what you stored. The check happens before anything
is written, so a rejected replace keeps the old value.

```cpp
device.set_readings("a-name-longer-than-sixteen-chars", 3);   // ARRAY_FULL, nothing stored
device.has_readings("a-name-longer-than-sixteen-chars");      // false
```

A key or value arriving *from the wire* that is too long is refused the same way
any oversized string field is: `deserialize` returns `ARRAY_FULL`.

### Reading a value when absence matters

`get_` with a single argument hands back the default value of the value type when
the key is absent, which is what protobuf prescribes. To tell an absent key from a
key holding the default value, use the two argument form:

```cpp
int32_t reading = 0;
if(::EmbeddedProto::Error::NO_ERRORS == device.get_readings("cellar", reading))
{
  // The key was present, reading holds its value.
}
```

It returns `INDEX_OUT_OF_BOUND` when the key is absent and leaves the value
untouched. For a `bytes` or message value this two argument form is the only
lookup, as those types have no literal default to hand back.

```cpp
Point corner;
if(::EmbeddedProto::Error::NO_ERRORS == msg.get_corners("north", corner))
{
  // corner holds a copy of the stored message.
}
```

### Iterating

A map is stored as an array of entries, each holding a key and a value, so it is
walked by index the way a repeated field is:

```cpp
for(uint32_t i = 0; i < device.readings_size(); ++i)
{
  const auto& entry = device.readings(i);
  entry.get_key();
  entry.get_value();
}
```

`mutable_readings(i)` hands out the entry for modification. The entry type is a
generated nested class named after the field, `Device::ReadingsEntry` here.

The order of the entries is the order in which they were inserted or received.
Protobuf does not give a map any ordering, so do not rely on it.

---

## Streaming a map through callbacks

A map is the field most likely to outgrow the RAM of a small target. Setting
`callbackStorage` makes the entries flow through two user callbacks instead of being
held in the message: one entry of RAM, no matter how many entries pass through.

```proto
message Device
{
  map<string, int32> readings = 1 [(EmbeddedProto.options) = {
    callbackStorage: true,
    keyMaxLength: 16
  }];
}
```

`maxLength` is not needed, nothing is stored. Bind a *source* to serialize and a
*sink* to deserialize:

```cpp
using Entry = Device::ReadingsEntry;
using Field = ::EmbeddedProto::MessageCallback<Entry>;

// Hand out one entry per call, return false when done.
class Readings
{
  public:
    bool operator()(Entry& entry)
    {
      const bool more = index < count;
      if(more)
      {
        entry.mutable_key().set(names[index]);
        entry.set_value(values[index]);
        ++index;
      }
      return more;
    }
};

Readings readings;
Field::SourceCallback source;
source.set(readings);

Device device;
device.mutable_readings().set_source(source);
device.serialize(buffer);   // pulls one entry at a time
```

Deserializing works the same way with `set_sink`, which is handed each entry as it
is parsed:

```cpp
class Collect
{
  public:
    ::EmbeddedProto::Error operator()(const Entry& entry)
    {
      // Store, forward or act on this entry.
      return ::EmbeddedProto::Error::NO_ERRORS;
    }
};
```

The entries a streaming map writes are byte for byte those a resident map writes, so
the two are interchangeable on the wire and a peer can not tell the difference.

### What a streaming map can not do

- **No key lookup.** `has_`, `get_`, `set_`, `remove_` and `find_` need a collection
  to search and are not generated. A stream only offers `mutable_readings()` to bind
  the callbacks with.
- **The source is drained once.** A field whose length has to be known before its
  bytes are written would have to pull every entry twice. The generated code
  therefore refuses such a placement at runtime with `CALLBACK_SEQUENCE` rather than
  calling the source twice. In practice this means a message holding a streaming map
  should be the message you serialize, not a length prefixed field inside a larger
  one.

---

## Duplicate keys

Protobuf allows a peer to send the same key more than once and prescribes that the
last value wins. EmbeddedProto stores every entry it receives and resolves the
duplicate on lookup: `get_`, `has_` and `find_` scan the entries backwards, so they
report the last entry carrying that key. Nothing is scanned or rewritten while
deserializing.

Two consequences are worth knowing:

- A duplicate sent by a peer occupies a slot, so a map sized for the number of
  distinct keys can report `ARRAY_FULL` on a stream containing duplicates.
- Iterating by index shows every entry received, duplicates included, while `get_`
  shows only the winning value. `remove_` drops every entry carrying the key.

Entries written through `set_` are always unique; only the wire can introduce a
duplicate.

---

## Partial serialization

Maps work with the partial (resumable) engine as any other field does. A map may be
split across buffers anywhere, in the middle of an entry as well as between two
entries, and a streaming map pulls each entry exactly once across a resume.
