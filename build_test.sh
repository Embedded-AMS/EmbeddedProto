#! /bin/sh

# Generate sources using the EAMS plugin.
mkdir -p ./build/EAMS
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/simple_types.proto
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/nested_message.proto

# For validation generate the same message using google code.
mkdir -p ./build/google
protoc -I./test/proto --cpp_out=./build/google ./test/proto/simple_types.proto
protoc -I./test/proto --cpp_out=./build/google ./test/proto/nested_message.proto

# Build the tests
mkdir -p build/test
cd build/test/
cmake -DCMAKE_BUILD_TYPE=Debug ../../
make