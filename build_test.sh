#!/usr/bin/env bash
#
# Copyright (C) 2020-2025 Embedded AMS B.V. - All Rights Reserved
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
# <https://embeddedproto.com/pricing/>.
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

SERIALIZATION_MODE="${1:-1}"

if [[ ("${SERIALIZATION_MODE}" != "0") && ("${SERIALIZATION_MODE}" != "1") ]]; then
  echo "Usage: $0 [EP_SERIALIZATION_MODE]"
  echo "  EP_SERIALIZATION_MODE: 0 (full, default in code) or 1 (partial)"
  exit 1
fi

# Build the tests
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-DEP_SERIALIZATION_MODE=${SERIALIZATION_MODE}" -B./build/test
make -j16 -C ./build/test
