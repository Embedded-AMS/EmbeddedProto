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
set -euxo pipefail

./build/test/test_EmbeddedProto --gtest_output="xml:build/test/test_details.xml"

rm -rf ./code_coverage_report/*
mkdir -p code_coverage_report

if [ $# -eq 0 ]; then
  # No arguments provided, execte the analyisis for sonar qube.
  gcovr --exclude external/ --exclude test/ --sonarqube -o code_coverage_report/coverage.xml

else
# Pass commands
case "$1" in
  -l|--local)
    # Run the code coverage for local machine analysis
    gcovr --exclude external/ --exclude test/ --html --html-details -o code_coverage_report/index.html
    ;;
  *)
    ;;
esac
fi
