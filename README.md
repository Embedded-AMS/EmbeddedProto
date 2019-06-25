
![alt text](https://embeddedams.nl/wp-content/uploads/2018/07/EmbeddedAMS_long.png "Embedded AMS Logo")
Embedded AMS B.V. Amsterdam, [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


# Introduction

EmbeddedProto is a C++14 implementation of [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) dedicated for micro controllers. This means the implementation focuses on a small footprint and low memory usage. No dynamic memory allocation is used and the code is check to be in line with the [MISRA C++](https://www.misra.org.uk/Activities/MISRAC/tabid/171/Default.aspx) guideline. 

In this way EmbeddedProto provides a very simple interface to exchange data between embedded devices and the out side world. Specifying the data format between your device and other devices, servers, apps or desktop applications is standardized and made simple.


# Supported Features

All features mentioned below are proto3. At this moment proto2 is not supported. Taken from the Protobuf website:
> Prefer proto3 While proto2 will continue to be supported, we encourage new codes to use proto3 instead, which is easier to use and supports more languages.
For this reason it is unlikely we will support proto2 in the future.

Currently EmbeddedProto is work in progress. Below two tables are given indicating the level of support for various variable types and features.

| Variable Type | Support |
| --- | --- |
double | Full
float | Full
int32 | Full
int64 | Full
uint32 | Full
uint64 | Full
sint32 | No
sint64 | No
fixed32 | Full
fixed64 | Full
sfixed32 | Full
sfixed64 | Full
bool | Full
string | No
bytes | No

| Feature | Support |
| --- | --- |
Enum | Full
Other Messages | No support
oneof | No support
singular | No support
repeated | No support


# Installation

What is required to be able to generate the source files:
1. Python 3.6
2. Pip
3. Protobuf 3.6.1
4. CMake 3.10.2 (optional to build PC unit tests)
5. Git if you do not have it already.

Install the required software and continue with checking out the repository. For PC unit testing gtest is used which is included as a submodule. If you intent to run the PC unit tests of EmbeddedProto it is suggested that you pull in the submodules as well. 

Besides the list of tools above additional python packages are required. These are listed in the `requirements.txt` file in this repository. It is advised to install these required packages using pip in a python virtenv. You can however install the requirements globally. To install the packages run the command:
```bash
pip install -r requirements.txt
```


# Normal Usage

When working on your project you write your proto files. Next the would like to use them in your source code. This requires you to generate the code based upon the definitions you have written. This is done using our plugin for the protoc compiler `protoc-gen-eams.py`. To generate the code use the following command:
```bash
protoc --plugin=protoc-gen-eams -I./LOCATION/PROTO/FILES --eams_out=./build PROTO_MESSAGE_FILE.proto
```
What happens is that protoc is toled to use our plugin with the option `--plugin`. Next the the standard option `-I` includes a folder where your \*.proto files are located. The option `--eams_out` specifies the location where to store the generated source code. Finally a specific protofile is set to parse.

As our plugin is a Python script and the protoc plugin should be an executable a small terminal script is included. This terminal 


# Development

## PC Unit Tests

The unit tests for PC are mend to test the logic of EmbeddedProto. The test are build upon the GTest and GMock libraries which are include as a submodule in this repository. 

The tests are build using a small script in the root folder of the project:
```bash
./build_test.sh
```
This will:
1. Generate message source files. The generated files are located in `./build/EAMS/`.
1. Compile GTest and GMock into `./build/google/`.
1. Build the unit tests into `./build/test`.

To run the unit test execute:
```bash
./build/test/test_EmbeddedProto
```
