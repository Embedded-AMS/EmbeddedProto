
![alt text](https://embeddedproto.com/wp-content/uploads/2022/04/Embedded_Proto.png "Embedded Proto Logo")


Embedded Proto is a product of Embedded AMS B.V. For more information about Embedded Proto please visit [EmbeddedProto.com](https://EmbeddedProto.com).

Copyrights 2020-2024 Embedded AMS B.V., [www.EmbeddedAMS.nl](https://www.EmbeddedAMS.nl), [info@EmbeddedAMS.nl](mailto:info@EmbeddedAMS.nl)


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
1. Python 3.8 and up
2. Pip
3. Protobuf v21.5 and up (tested with v26.1)
4. Git

After installing the requirements, continue by cloning the Embedded Proto repo. We advised using Embedded Proto as a submodule in your project. This way, you can track the version of Embedded Proto with the version of your project.
```bash
cd your_project_dir
git submodule add https://github.com/Embedded-AMS/EmbeddedProto.git
git commit -m "Added the latest version of Embedded Proto as a submodule."
```
Next, enter the Embedded Proto folder and run the setup script. The script will create a self-contained python environment. In this environment, various python packages will be installed, which are required by Embedded Proto.
```bash
cd EmbeddedProto
python setup.py
```
Did you install protoc in a custom folder, or is the include folder of protobuf not in your path? In these cases, you may get an error from the setup script. You have to provide the location of the include with the --include parameter:
```bash
python setup.py --include ~/protobuf/protoc-21.5/include
```
In this example, you have installed a specific version of protoc, and you named its installation folder `~/protobuf/protoc-21.5`.

You can check out latest the command line parameters of the setup script using the help parameter:
```bash
python setup.py --help
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

All features mentioned above are of version proto3. At this moment, proto2 is not supported. Taken from the Protobuf website:
> Prefer proto3 while proto2 will continue to be supported, we encourage new codes to use proto3 instead, which is easier to use and supports more languages.
For this reason, it is unlikely that Embedded Proto will support proto2 in the future.


# Development

If you consider helping with the development of Embedded Proto please consider reading [this](https://embeddedproto.com/documentation/installation/#for-embedded-proto-developers). It details how you can build the unit tests included in this repo.


