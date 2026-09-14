![alt text](https://embeddedproto.com/wp-content/uploads/2022/04/Embedded_Proto.png "Embedded Proto Logo")


Embedded Proto is a product of Embedded AMS B.V. For more information about Embedded Proto please visit [EmbeddedProto.com](https://EmbeddedProto.com).

Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved, [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


# Introduction

Embedded Proto is a C++ implementation of [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) dedicated to microcontrollers. The implementation focuses on a small footprint and low memory usage. No dynamic memory allocation is used to make the code predictable. Unit tests and static code analysis are used to improve the reliability of the code.

Natively, protocol buffers are not suitable for microcontrollers. The C++ code protoc generates is written for server and desktop processors. Embedded Proto is a plugin for *protoc* generating C++ code suitable for microcontrollers. In this way, Embedded Proto provides an easy-to-use interface to exchange data between embedded devices and the outside world. Specify the data format between your IoT device and other devices, servers, apps, or desktop applications in a standardized way!


# Requirements

To generate the code for your messages:
* Python 3.11 or newer, and pip.

The protobuf compiler comes with Embedded Proto, you do not have to install protoc yourself.

To build the generated code for your target:
* A C++ compiler supporting C++17.
* No dynamic memory, no exceptions and no RTTI are required. The library is header only.

Optional, for the alternative ways to install described on the [installation](https://EmbeddedProto.com/documentation/installation/) page:
* [uv](https://docs.astral.sh/uv/), a much faster replacement for pip and venv.
* Git, when you use Embedded Proto as a submodule in your project.


# Getting started

New to Embedded Proto? Follow the [quick start](https://EmbeddedProto.com/documentation/quick-start/). In about ten minutes you install Embedded Proto, define a message, generate the code for it and serialize the message in a small program on your PC. The same code moves to your microcontroller without changes.

In short, Embedded Proto is installed with pip:
```bash
pip install EmbeddedProto
```
Generate the C++ code for the messages in your \*.proto file:
```bash
embeddedproto -I proto --eams_out=generated proto/reading.proto
```
Build your project with C++17, the `generated` folder and the header only library of Embedded Proto as include paths. The command `embeddedproto --cpp-src-location` prints where the library is.
When linking with a C compiler, do not forget to pass `-lstdc++` to the linker. This prevents errors like `undefined reference to`.

Other ways to install, with uv or as a git submodule, are described on the [installation](https://EmbeddedProto.com/documentation/installation/) page.


# Documentation

The manual lives on [EmbeddedProto.com](https://EmbeddedProto.com/documentation/):
* [How it works](https://EmbeddedProto.com/documentation/how-it-works/)
* [Generating source code](https://EmbeddedProto.com/documentation/generating-source-code/) and [setting field options from a file](https://EmbeddedProto.com/documentation/options-file/)
* [Using a message](https://EmbeddedProto.com/documentation/using-a-message/): strings, bytes, repeated fields, maps, Any and more
* [Serialization](https://EmbeddedProto.com/documentation/serialization/) and [deserialization](https://EmbeddedProto.com/documentation/deserialization/)
* [Supported features](https://EmbeddedProto.com/documentation/supported-features/)
* [Advanced topics](https://EmbeddedProto.com/documentation/advanced-topics/): protobuf editions, callback storage and custom storage

Our website also hosts an array of [examples](https://EmbeddedProto.com/examples/) detailing possible use cases and tutorials on toolchain integrations.


# What is new

Version 4.0.0 is the largest release of Embedded Proto so far. It changes how the library is installed, adds maps, Any and streaming, and supports the new protobuf editions. See [what is new in 4.0](https://EmbeddedProto.com/documentation/what-is-new-in-4-0/) for what you gain, and [migrating from 3.x](https://EmbeddedProto.com/documentation/migrating-from-3-x/) for what to change in your project.

To stay up to date, signup for our [User Update](https://EmbeddedProto.com/signup).


# License

Embedded Proto is dual licensed. See [LICENSE](LICENSE) for the full notice.

## Open source
Embedded Proto is free under the GNU General Public License v3.0 for any project whose own source code is released under a GPLv3-compatible open source license. Evaluation and testing before purchase are free as well. The GPL version comes without support.

## Commercial
Building a closed source product? Then you need a commercial license. It removes the GPL obligation to publish your source code, and depending on the tier includes:
* An unlimited number of MCUs
* Professional support
* A code quality report

See [embeddedproto.com/pricing](https://EmbeddedProto.com/pricing/). Setting your license token is described on the [installation](https://EmbeddedProto.com/documentation/installation/) page.


# Development

If you consider helping with the development of Embedded Proto please read [CONTRIBUTING.md](CONTRIBUTING.md) and the [developer section](https://EmbeddedProto.com/documentation/installation/#for-embedded-proto-developers) of the installation page. They detail how you can build the unit tests included in this repo.
