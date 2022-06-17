
![alt text](https://embeddedproto.com/wp-content/uploads/2022/04/Embedded_Proto.png "Embedded Proto Logo")


Embedded Proto is a product of Embedded AMS B.V. For more information about Embedded Proto please visit [EmbeddedProto.com](https://EmbeddedProto.com).

Copyrights 2020-2022 Embedded AMS B.V. Amsterdam, [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


# Introduction

Embedded Proto is a C++ implementation of [Google Protocol Buffers](https://developers.google.com/protocol-buffers/) dedicated for micro controllers. This means the implementation focuses on a small footprint and low memory usage. No dynamic memory allocation is used to make the code predictable. To further improve the reliability of the code it is automatically checked to be in line with the [MISRA C++](https://www.misra.org.uk/Activities/MISRAC/tabid/171/Default.aspx) guideline. 

What are protocol buffers? Quoting from the [website](https://developers.google.com/protocol-buffers):
> Protocol buffers are Google's language-neutral, platform-neutral, extensible mechanism for serializing structured data – think XML, but smaller, faster, and simpler. You define how you want your data to be structured once, then you can use special generated source code to easily write and read your structured data to and from a variety of data streams and using a variety of languages

In a .proto file you define the structure of your message. Next you use the protocol buffers compiler *protoc* to generate source code. This code you can use in your project. Protoc natively supports many different programming languages. Your colleague working in a different language can thus use the same message structure. When one of you updates the structure just rerun *protoc* to get the latest source code.

Natively however protocol buffers are not suitable for micro controllers. The C++ generated is written for server and desktop processors. This is where Embedded offers a alternative. Embedded Proto is a plugin for *protoc* generating C++ code suitable for micro controllers. In this way Embedded provides an easy to use interface to exchange data between embedded devices and the out side world. Specifying the data format between your IOT device and other devices, servers, apps or desktop applications in a standardized way.

This document details the following:
* What is new
* Examples
* Supported Features
* Installation
* Usage
* Development
* License
* Gratitude


# What is new

## 3.1.0
The most notable improvements in this version are:
* Reworked code to extend the support back to C++11.
* Worked on optimzing running the code coverage in Sonarqube.

## 3.0.0
The most notable improvements in this version are:
* The length of repeated, string and bytes fields can now be set from the .proto file. You can find information on how to this in the online [documentation](https://embeddedproto.com/documentation/using-a-message/repeated-fields/).
* The ram size of messages has been reduced. This was done by using less polymorphism in the low level field classes. This required upgrading to C++17 and up.
* In a .proto file it is now possible to use a message or enum before it is defined. The plugin will make a dependency tree of the messages and enums defined and sort them before generating the source code. Recursive inclusion are not supported.
* Some of the message functions changed. The functions where already marked as deprecated in the latest 2.X.X release.


# Examples 

Our website hosts an array of [examples](https://embeddedproto.com/examples/) detailing possible use cases and tutorials on toolchain integrations. This includes:
* [A Command and Control structure](https://EmbeddedProto.com/a-simple-uart-example-with-embedded-proto/),
* [Ethernet Communication](https://EmbeddedProto.com/mbed-example-with-embedded-proto/),
* and [Makefile integration](https://EmbeddedProto.com/how-to-set-up-a-project-with-embeddedproto-using-makefiles).


# Supported Features

Below two tables are given indicating the level of support for various variable types and features.

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

All features mentioned above are of version proto3. At this moment proto2 is not supported. Taken from the Protobuf website:
> Prefer proto3 while proto2 will continue to be supported, we encourage new codes to use proto3 instead, which is easier to use and supports more languages.
For this reason it is unlikely that Embedded Proto will support proto2 in the future.


# Installation

What is required to be able to generate source files based on .proto files:
1. Python 3.8 and up
2. Pip
3. Protobuf 3.19.4 (Higher is not yet supported due to breaking changes in version 4.20 and up)
4. Git

After installing the requirements continue by cloning the Embedded Proto repo:
```bash
git clone https://github.com/Embedded-AMS/EmbeddedProto.git
```
Enter the new folder and a terminal to run the setup script. Depending on your system Linux or Windows you need to run a different script.

On Linux:
```bash
./setup.sh
```

On Windows:
```bash
.\setup.bat C:\protoc-XX.Y-win64
```
In which `C:\protoc-XX.Y-win64` refers to the folder in which you installed the Google Protobuf compiler protoc.

After running the setup script you are ready to use Embedded Proto.

More installation documentation can be found on the [documentation website](https://embeddedproto.com/documentation/installation/).


# Usage

When working on your project you write your proto files defining the message structure. Next you would like to use them in your source code. This requires you to generate the code based upon the definitions you have written. This is done using our plugin for the protoc compiler protoc-gen-eams.py. To generate the code use the following command:

On Linux:
```bash
protoc --plugin=protoc-gen-eams -I./LOCATION/PROTO/FILES --eams_out=./generated_src PROTO_MESSAGE_FILE.proto
``` 
On Windows:
```bash
protoc --plugin=protoc-gen-eams=protoc-gen-eams.bat -I.\LOCATION\PROTO\FILES --eams_out=.\generated_src PROTO_MESSAGE_FILE.proto
```

What happens is that protoc is instucted to use our plugin with the option --plugin. Next the the standard option -I includes the folder where your \*.proto files are located. The option --eams_out specifies the location where to store the generated source code. Finally a specific protofile is set to be parsed.

As our plugin is a Python script and the protoc plugin should be an executable a small terminal script is included. This terminal script is called protoc-gen-eams and is used to execute python with the Embedded Proto python script as a parameter. The main take away is that this script should be accessible when running your protoc command.

After running protoc without any errors the generated source code is located in the folder specified by --eams_out. This folder is to be included into your project. This is not the only folder to be included. The generated source files depend on other header and source files. These files can be found in EmbeddedProto/src. You are thus required to include this folder as well in you toolchain.

Various examples how to use and integrate Embedded Proto in your project are given in the [Examples](https://embeddedproto.com/examples/) section.


# Development

If you consider helping with the development of Embedded Proto please consider reading [this](https://embeddedproto.com/documentation/intallation/#for-embedded-proto-developers). It details how you can build the unit tests included in this repo.


# License

Embedded Proto uses a dual licensing model. One for open source projects and one for commercial usage.

## Open Source
You can use Embedded Proto for free in open source projects or for testing. However, on demand support is not available, only if you have a commercial license. For open source projects you can download the source code from Github. The code is licensed under the GNU General Public License V3.0 and you can use is for all your non commercial projects. 

## Commercial License
If you are developing a commercial product you need to buy a commercial license from Embedded Proto. There is a suitable license for each magnitude of business, from startup to enterprise. Depending on the license, it may give you access to:
* An unlimited number of mcu’s
* Professional support
* MISRA code quality report

You can request more information about a commercial license on our [website](https://embeddedproto.com/pricing).

# Give your feedback

[![alt text](https://embeddedproto.com/wp-content/uploads/2022/06/feedback.png)](https://embeddedproto.com/feedback/)

# Gratitude

The team would like to thank you for your interest in Embedded Proto! We greatly appreciate you use our library. If you like working with it consider to Star the library on [Github](https://github.com/Embedded-AMS/EmbeddedProto).

To stay up to date you can also follow our company on [LinkedIn](https://www.linkedin.com/company/embeddedams/) or signup for our [newsletter](https://EmbeddedProto.com/#newsletter).