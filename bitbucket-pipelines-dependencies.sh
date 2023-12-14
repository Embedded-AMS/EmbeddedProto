#!/bin/bash

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

if [ -d "$PWD/protoc" ]; then
  echo "found protoc cache"
else
  curl --insecure -OL https://github.com/protocolbuffers/protobuf/releases/download/v25.1/protoc-25.1-linux-x86_64.zip
  unzip -o protoc*.zip -d protoc/
fi
export PATH="$PATH:$PWD/protoc/bin"

if [ -d "$PWD/build-wrapper" ]; then
  echo "found sonar cloud wrapper cache"
else
  curl --insecure -OL https://sonarcloud.io/static/cpp/build-wrapper-linux-x86.zip
  unzip -o build-wrapper-linux-x86.zip
  mv build-wrapper-linux-x86/ build-wrapper/
  # The following is required for my local machine.
  cp build-wrapper/libinterceptor-x86_64.so build-wrapper/libinterceptor-haswell.so
fi
export PATH="$PATH:$PWD/build-wrapper"

if [ -d "$PWD/sonar-scanner/" ]; then
  echo "found sonar scanner scanner cache"
else
  curl --insecure -OL https://binaries.sonarsource.com/Distribution/sonar-scanner-cli/sonar-scanner-cli-5.0.1.3006-linux.zip
  unzip -o sonar-scanner-cli-*-linux.zip
  mv sonar-scanner*/ sonar-scanner/
fi
export PATH="$PATH:$PWD/sonar-scanner/bin"

if [ -d "$PWD/venv" ]; then
  echo "found python virtualenv in cache"
else
  python setup.py
fi

#tree

#cd generator
#protoc -I . --python_out=EmbeddedProto embedded_proto_options.proto
#cd -

echo
ls generator/EmbeddedProto
echo
