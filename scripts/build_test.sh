#!/usr/bin/env bash
#
# Copyright (C) 2020-2026 Embedded AMS B.V. - All Rights Reserved
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

# Convert user-friendly parameter to the compiler define enabling partial mode.
# The serialization mode and, optionally, the extra build defines to test with.
EP_DEFINES=""
MODE_NAME="full"
EXTRA_DEFINES=""

usage() {
  echo "Usage: $0 [MODE] [nullterm]"
  echo "  MODE: 'full' (default) or 'partial'"
  echo "        Full mode: Traditional serialization (complete in one call)"
  echo "        Partial mode: Chunked serialization for constrained environments"
  echo "  nullterm: also define NULL_TERMINATED_STRINGS, reserving a null terminator per string"
  echo ""
  echo "Examples:"
  echo "  $0                    # Build with full serialization (default)"
  echo "  $0 full               # Build with full serialization"
  echo "  $0 partial            # Build with partial serialization"
  echo "  $0 partial nullterm   # Partial serialization with null terminated strings"
}

for ARG in "$@"; do
  case "${ARG}" in
    full)
      EP_DEFINES=""
      MODE_NAME="full"
      ;;
    partial)
      EP_DEFINES="-DPARTIAL_SERIALIZATION_ENABLED"
      MODE_NAME="partial"
      ;;
    nullterm)
      EXTRA_DEFINES="-DNULL_TERMINATED_STRINGS"
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done
EP_DEFINES="${EP_DEFINES} ${EXTRA_DEFINES}"

echo "Building tests with ${MODE_NAME} serialization mode..."

# Build the tests
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="${EP_DEFINES}" -B./build/test
make -j16 -C ./build/test
