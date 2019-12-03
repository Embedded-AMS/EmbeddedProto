#! /bin/sh

# Generate sources using the EAMS plugin.
mkdir -p ./build/EAMS
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/simple_types.proto
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/nested_message.proto
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/repeated_fields.proto
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/oneof_fields.proto

# For validation and testing generate the same message using python
mkdir -p ./build/python
protoc -I./test/proto --python_out=./build/python ./test/proto/simple_types.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/nested_message.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/repeated_fields.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/oneof_fields.proto


# Build the tests
mkdir -p build/test
cd build/test/
cmake -DCMAKE_BUILD_TYPE=Debug ../../
make