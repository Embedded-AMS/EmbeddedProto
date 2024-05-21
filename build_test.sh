#!/usr/bin/env bash
#
# Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
#
# This file is part of Embedded Proto.
#
# Embedded Proto is open source software: you can redistribute it and/or 
# modify it under the terms of the GNU General Public License as published 
# by the Free Software Foundation, version 3 of the license.
#
# Embedded Proto  is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
#
# For commercial and closed source application please visit:
# <https://EmbeddedProto.com/license/>.
#
# Embedded AMS B.V.
# Info:
#   info at EmbeddedProto dot com
#
# Postal address:
#   Atoomweg 2
#   1627 LE, Hoorn
#   the Netherlands
#

# Fail on first non-zero return code
set -exuo pipefail

# For validation and testing generate the same message using python
mkdir -p ./build/python
mkdir -p ./build/python/subfolder
protoc -I./test/proto --python_out=./build/python ./test/proto/simple_types.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/nested_message.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/repeated_fields.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/oneof_fields.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/include_other_files.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/file_to_include.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/subfolder/file_to_include_from_subfolder.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/empty_file_to_include.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/string_bytes.proto
protoc -I./test/proto --python_out=./build/python ./test/proto/optional_fields.proto
protoc -I./test/proto -I./generator --python_out=./build/python ./test/proto/field_options.proto

# Build the tests
cmake -DCMAKE_BUILD_TYPE=Debug -B./build/test
make -j16 -C ./build/test
