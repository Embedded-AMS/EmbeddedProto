
![alt text](https://embeddedproto.com/wp-content/uploads/2022/04/Embedded_Proto.png "Embedded Proto Logo")


Embedded Proto is a product of Embedded AMS B.V. For more information about Embedded Proto please visit [EmbeddedProto.com](https://EmbeddedProto.com).

Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved, [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


# Introduction

Embedded Proto is a C++ implementation of [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) dedicated to microcontrollers. The implementation focuses on a small footprint and low memory usage. No dynamic memory allocation is used to make the code predictable. Unit tests and static code analysis are used to improve the reliability of the code.

Natively, protocol buffers are not suitable for microcontrollers. The C++ generated is written for server and desktop processors. This is where Embedded offers a solution. Embedded Proto is a plugin for *protoc* generating C++ code suitable for microcontrollers. In this way, Embedded Proto provides an easy-to-use interface to exchange data between embedded devices and the outside world. Specify the data format between your IoT device and other devices, servers, apps, or desktop applications in a standardized way!

This document details the following:
* What is new
* License
* Installation
* Usage
* Supported Features
* Examples
* Development


# What is new

To stay up to date, signup for our [User Update](https://EmbeddedProto.com/signup).

## 4.0.0
* TODO Installation via pip3: `pip install EmbeddedProto`. This will also install a matching version of protoc. So you do not need to install the protoc compiler yourself.
* TODO Run EmbeddedProto as an executable: `embeddedproto -I YOUR_FOLDER YOUR_PROTO_FILE.proto`
* Included the C++ source files in the python package. You can get the location of these C++ source files by running: `embeddedproto --cpp-src-location`.
* Added the `NULL_TERMINATED_STRINGS` build define. When defined every string field reserves one extra character behind its maximum length which always holds a null terminator, so `get_const()` is a valid c style string also when the string is completely full. It costs one byte of RAM per string field and changes nothing on the wire; bytes fields are unaffected. The default is not to reserve it, which keeps the memory layout of earlier versions.
* Added support for `map<K, V>` fields. A map is declared as in any other protobuf implementation and sized with `maxLength` (the number of entries) plus `keyMaxLength` and `valueMaxLength` for a string or bytes key and value. The generated API is map shaped: `set_`, `get_`, `has_`, `remove_`, `clear_`, `find_` and `_size`. Entries may also be streamed through callbacks with `callbackStorage`, in which case no entries are stored in the message. See [doc/maps.md](doc/maps.md).
* Added the `customStorage` field option. With `[(EmbeddedProto.options).customStorage = true]` you take control of the storage type of a repeated, string, bytes or message field. The generated message exposes the storage type as a plain template parameter (without a size parameter and without a default), so you supply the complete type yourself. The supplied type must derive from `::EmbeddedProto::RepeatedField<T>` (repeated), `::EmbeddedProto::internal::BaseStringBytes` (string/bytes) or `::EmbeddedProto::MessageInterface` (message); this is enforced with a `static_assert`. The option takes precedence over `maxLength`. Fields without the option keep their existing behaviour, so existing `.proto` files and generated user code are unchanged.

## 3.6.0
* Update to Protobuf version 32.0.
* Increated the minimum python version to 3.10.
* Added EmbeddedProto options to allow setting the size of the string (or bytes) field in when it is also repeated, example: `repeated string str = 1 [(EmbeddedProto.options).maxLength = 3, (EmbeddedProto.options).nestedMaxLength = 10];`.

## 3.5.3 
* Fixed build problems in release 3.5.3.

## 3.5.2
* Updated to Protobuf version 27.1.

## 3.5.1
* Updated to Protobuf version 26.1.

## 3.5.0
* Bub fix related to optional string or bytes fields. They where not check to be set or not.
* Small interface change in RepeatedField class to use correct array index type. Tjos is a possible breaking change when you derived from the RepeatedField.
* Support for spaces in folder names on Windows.
* Started reworking the company internal toolchain.

## 3.4.0
* In the background the installation switched to using Python SetupTools (thanks to the contributors). In the future we would like to use Pip for the installation.
* Problems with non-matching versions have been addressed. You now get a warning which allows you to continue even if the version does not match exactly.
* Added some useful command line options to the setup script.

## 3.3.0
* Added a to_string function for debugging (see [documentation](https://embeddedproto.com/documentation/to-string/)).
* Added getter functions which will return an error for index out of bounds cases.
* Bug fix the toposorting algo for nested message definitions.

## 3.2.0
The most notable improvements in this version are:
* Updated to protobuf v21.5. The python module made by Google for this version is not backwards compatible. Please update your protoc installation!
* Wrote a python setup script instead of separate scripts for Linux and Windows.
* Added simple implementations of the ReadBufferInterface and WriteBufferInterface: ReadBufferFixedSize and WriteBufferFixedSize. 

## 3.1.0
The most notable improvements in this version are:
* Reworked code to extend the support back to C++11.
* Worked on optimizing running the code coverage in Sonarqube.

## 3.0.0
The most notable improvements in this version are:
* The length of repeated, string and bytes fields can now be set from the .proto file. You can find information on how to do this in the online [documentation](https://embeddedproto.com/documentation/using-a-message/repeated-fields/).
* The ram size of messages has been reduced. This was done by using less polymorphism in the low-level field classes. This required upgrading to C++17 and up.
* In a .proto file, it is now possible to use a message or enum before it is defined. The plugin will make a dependency tree of the messages and enums defined and sort them before generating the source code. Recursive inclusion is not supported.
* Some of the message functions changed. The functions were already marked as deprecated in the latest 2.X.X release.


# License

Embedded Proto uses a dual licensing model. One for open source projects and one for commercial usage.

## Open Source
You can use Embedded Proto for free in open source projects or for testing. However, on demand support is not available only if you have a commercial license. For open source projects, you can download the source code from Github. The code is licensed under the GNU General Public License V3.0, which you can use for all your non-commercial projects. 

## Commercial License
Are you developing a commercial product? If so, you need to buy a commercial license from Embedded Proto. There is a suitable license for each type of business, from startup to enterprise. Depending on the license, it may give you access to the following:
* An unlimited number of mcu’s
* Professional support
* Code quality report

You can request more information about a commercial license on our [website](https://embeddedproto.com/pricing).


# Installation

What is required to be able to generate source files based on .proto files:
1. Python 3.10 and up
2. Pip
3. Protobuf v32.0
4. Git
5. Optional: [uv](https://docs.astral.sh/uv/), a much faster replacement for pip during the installation.

After installing the requirements, continue by cloning the Embedded Proto repo. We advised using Embedded Proto as a submodule in your project. This way, you can track the version of Embedded Proto with the version of your project.
```bash
cd your_project_dir
git submodule add https://github.com/Embedded-AMS/EmbeddedProto.git
git commit -m "Added the latest version of Embedded Proto as a submodule."
```
Next, enter the Embedded Proto folder and run the setup script. The script will create a self-contained python environment. In this environment, various python packages will be installed, which are required by Embedded Proto.
```bash
cd EmbeddedProto
python install.py
```
The environment is created with pip by default. When you have uv installed the same can be done a lot faster:
```bash
python install.py --installer uv
```
Did you install protoc in a custom folder, or is the include folder of protobuf not in your path? In these cases, you may get an error from the setup script. You have to provide the location of the include with the --include parameter:
```bash
python install.py --include ~/protobuf/protoc-32.0/include
```
In this example, you have installed a specific version of protoc, and you named its installation folder `~/protobuf/protoc-32.0`.

You can check out latest the command line parameters of the setup script using the help parameter:
```bash
python install.py --help
```

More installation documentation can be found on the [documentation website](https://embeddedproto.com/documentation/installation/).


# Usage

You write your proto files defining the message structure when working on your project. Next, you would like to use them in your source code. Generating the code based on your message definitions is required. Please do this by using our plugin for the protoc compiler protoc-gen-eams.py. Generate the code using the following command:

On Linux:
```bash
protoc --plugin=protoc-gen-eams -I./LOCATION/PROTO/FILES --eams_out=./generated_src PROTO_MESSAGE_FILE.proto
``` 
On Windows:
```bash
protoc --plugin=protoc-gen-eams=protoc-gen-eams.bat -I.\LOCATION\PROTO\FILES --eams_out=.\generated_src PROTO_MESSAGE_FILE.proto
```

Protoc is instructed to use our plugin with the option --plugin. The standard option -I includes the folder where your \*.proto files are located. The option --eams_out specifies where to store the generated source code. Finally, the protofile to be parsed is specified.

As our plugin is a Python script and the protoc plugin should be an executable, a small terminal script is included. This terminal script is called protoc-gen-eams and is used to execute python with the Embedded Proto python script as a parameter. The main takeaway is that this script should be accessible when running your protoc command.

After running protoc without errors, the generated source code is located in the folder specified by -eams_out. You have to include two folders in your toolchain:
* The folder you specified with -eams_out, and
* The source code of Embedded Proto is located in EmbeddedProto/src. 
* When building do not for get to pass `-lstdc++` to the linker. This to prevent errors like: `undefined reference to`.


# Setting field options from a file

Embedded Proto's per field options, `maxLength`, `nestedMaxLength`, `keyMaxLength`, `valueMaxLength`, `customStorage` and `callbackStorage`, are normally written in the \*.proto itself:

```proto
message SensorFrame {
  repeated int32 samples = 1 [(EmbeddedProto.options).maxLength = 128];
}
```

That \*.proto is a shared contract though, used by other languages and other teams, while these options describe one embedded target. When the schema is not yours to edit, for instance because it comes from a library, the same options can be supplied from an external JSON file instead.

The file mirrors the structure of your messages: the parts of the package, the message names, any nested messages and finally the field. Options are set on a field. For a schema like this one:

```proto
package foo.telemetry;

message Reading {
  string sensor_id = 1;
  repeated double values = 2;
  bytes raw = 3;

  message Meta {
    repeated string tags = 1;
  }
}
```

the options file reads:

```json
{
  "foo": {
    "telemetry": {
      "Reading": {
        "sensor_id": { "maxLength": 16 },
        "values":    { "maxLength": 64 },
        "raw":       { "callbackStorage": true },
        "Meta": {
          "tags": { "maxLength": 4, "nestedMaxLength": 12 }
        }
      }
    }
  }
}
```

Point the generator at it with the `options_file` parameter:

```bash
protoc --plugin=protoc-gen-eams -I./LOCATION/PROTO/FILES \
       --eams_out=./generated_src \
       --eams_opt=options_file=./cfg/board_x.options.json \
       PROTO_MESSAGE_FILE.proto
```

A few things worth knowing:

* A scope may also be written as a dotted path: `"foo.telemetry": { ... }` says the same as the two nested objects above, and `"foo.telemetry.Reading.sensor_id": { "maxLength": 16 }` says it in one line. Both spellings may be mixed in one file.
* The file is keyed by package and message, never by file name or location. It therefore has no relation to where your \*.proto files are, one file can hold the options of every schema in your build, and it may live anywhere.
* Give `--eams_opt` more than once to layer files: `--eams_opt=options_file=base.json --eams_opt=options_file=board_x.json`. The last file to set an option wins.
* Protoc also accepts parameters written in front of the output directory, `--eams_out=options_file=board_x.options.json:./generated_src`, separating several of them with a comma. That works too, `--eams_opt` is just easier to read.
* An option in the file wins over the same option written in the \*.proto. Overriding a value that is in the \*.proto is reported on the console, so it does not happen unnoticed.
* An entry naming a field or message that does not exist is an error, a typo may not quietly leave a buffer at its default size. Entries for a package that this protoc run does not compile at all are skipped, so one file can serve several builds.
* A file named with `options_file=` that does not exist is an error for the same reason.
* There is no support for a file placed next to the \*.proto and found automatically. Protoc passes a plugin only the file name relative to the include path, so where the \*.proto actually lives is not known to the generator. Name the file explicitly.


# Setting your license token

A commercial license adds a customer-specific header to the generated files. To configure your token, run:

```bash
# pip install:
embeddedproto --set-token <YOUR_TOKEN>

# submodule install (the install script accepts the same token):
python install.py --token <YOUR_TOKEN>
```

This stores the settings in a per-user config file and immediately verifies the token against the license server, printing whether it is active. The file lives at:

* Linux/macOS: `~/.config/embeddedproto/config.ini` (honours `$XDG_CONFIG_HOME`)
* Windows: `%APPDATA%\embeddedproto\config.ini`

It is created automatically (with a commented template) the first time the plugin runs, so you can also just edit it by hand:

```ini
[license]
token = <YOUR_TOKEN>
server_url = https://license.embeddedproto.com/v1/header
```

To point at a different (e.g. staging) server, pass `--server-url https://…` (it must be
`https://`) or set `server_url` in the file. Verify a configured setup at any time with
`embeddedproto --check-license`.

**CI / CD:** do not commit the token. Instead export it as an environment variable in your
pipeline; an exported variable always takes precedence over the config file:

```bash
export EMBEDDEDPROTO_BUILD_TOKEN=<YOUR_TOKEN>
# optional, to override the server:
export EMBEDDEDPROTO_LICENSE_URL=https://license.embeddedproto.com/v1/header
```


# Examples 

Our website hosts an array of [examples](https://embeddedproto.com/examples/) detailing possible use cases and tutorials on toolchain integrations. This includes:
* [A Command and Control structure](https://EmbeddedProto.com/a-simple-uart-example-with-embedded-proto/),
* [Ethernet Communication](https://EmbeddedProto.com/mbed-example-with-embedded-proto/),
* and [Makefile integration](https://EmbeddedProto.com/how-to-set-up-a-project-with-embeddedproto-using-makefiles).


# Supported Features

Below two tables indicate the level of support for various variable types and features.

| Variable Type | Support |
| --- | --- |
double | Full
float | Full
int32 | Full
int64 | Full
uint32 | Full
uint64 | Full
sint32 | Full
sint64 | Full
fixed32 | Full
fixed64 | Full
sfixed32 | Full
sfixed64 | Full
bool | Full
string | Length fixed via template or custom option
bytes | Length fixed via template or custom option

| Feature | Support |
| --- | --- |
Enum | Full
Messages as variables | Full
Defining messages in messages | Minimal
oneof | Full
singular | Full
repeated | Length fixed via template or custom option
optional | Full

At this moment, proto2 is not supported, and it is unlikely that Embedded Proto will support proto2 in the future.

## Protobuf editions

Embedded Proto supports Protobuf **editions** (`edition = "2023"` and `edition = "2024"`).
Editions replace the `syntax = proto2/proto3` switch with per-element *features* that
are overridable at file, message and field/enum scope. The full analysis behind the
implementation lives in [doc/protobuf_editions.md](doc/protobuf_editions.md).

| Editions feature | Support |
| --- | --- |
| `field_presence` (EXPLICIT / IMPLICIT / LEGACY_REQUIRED) | Full. EXPLICIT (the 2023 default) reuses the presence bitfield and generates `has_*()`. LEGACY_REQUIRED is always serialized (no presence bit, no absence enforcement). |
| Custom default values | Scalar and enum fields. See limitations for string/bytes. |
| `repeated_field_encoding` (PACKED / EXPANDED) | Full. Both forms are accepted on decode regardless of which form is emitted. |
| `enum_type` (OPEN / CLOSED) | Full. CLOSED enums validate the decoded value and drop unknown values. |
| `message_encoding` (LENGTH_PREFIXED / DELIMITED) | Full. DELIMITED uses group framing (`START_GROUP`/`END_GROUP`) which needs no length prefix, enabling single-pass encoding. |
| Edition 2024 | Cumulative on 2023; its additions are codegen/naming concerns with no runtime effect. |

### DELIMITED for single-pass encoding

A `LENGTH_PREFIXED` nested message must know its serialized length before its bytes,
which forces a size pre-pass (the size calculator walks the sub-tree, then the encoder
re-walks it). A `DELIMITED` field uses group framing instead, which carries no length, so
the size pre-pass disappears and the message is encoded in a single pass:

```proto
edition = "2023";
// Make every nested message in this file single-pass:
option features.message_encoding = DELIMITED;
```

This is an *encode* win (and removes the need for the size up front when streaming). Decoding
a group is marginally slower than a length-prefixed message because the reader scans for
`END_GROUP` instead of jumping by a known length. Both peers must agree via the shared schema.

### Explicit limitations

* `utf8_validation = VERIFY` is treated as **NONE**; Embedded Proto performs no UTF-8 validation.
* `json_format` is ignored (Embedded Proto has no JSON support).
* Unknown fields are not round-tripped. As a result CLOSED enums **drop** (rather than
  preserve) unknown values.
* Custom default values for **string and bytes** fields are not supported (the fixed-size
  storage has no literal constructor); a warning is printed and the default is ignored.
* In **partial (chunked) serialization** mode an unknown `DELIMITED` field is not skipped
  across buffer boundaries (known DELIMITED fields are fully supported in both modes).
* Edition 2024 files require a `protoc` that can parse edition 2024. The feature-resolution
  layer already handles 2024 (cumulative on 2023); older `protoc` releases cap at edition 2023.


# Development

If you consider helping with the development of Embedded Proto please consider reading [this](https://embeddedproto.com/documentation/installation/#for-embedded-proto-developers). It details how you can build the unit tests included in this repo.


