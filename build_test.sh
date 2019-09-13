#! /bin/sh

# Generate sources using the EAMS plugin.
mkdir -p ./build/EAMS
protoc --plugin=protoc-gen-eams=protoc-gen-eams -I./test/proto --eams_out=./build/EAMS ./test/proto/simple_types.proto

# Build the tests
mkdir -p build/test
cd build/test/
cmake -DCMAKE_BUILD_TYPE=Debug ../../
make