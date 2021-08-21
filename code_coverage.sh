#! /bin/bash

#
# Copyright (C) 2020-2021 Embedded AMS B.V. - All Rights Reserved
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
#   Johan Huizingalaan 763a
#   1066 VH, Amsterdam
#   the Netherlands
#

./build/test/test_EmbeddedProto --gtest_output="xml:build/test/test_details.xml"


if [ $# -eq 0 ]; then
  # No arguments provided, execte the analyisis for sonar qube.
  rm -rf ./code_coverage_report/*
  mkdir -p code_coverage_report
  cd code_coverage_report

  # Run gcov on the static source files.
  gcov ../test/*.cpp --object-directory ../build/test/CMakeFiles/test_EmbeddedProto.dir/test/ 
  gcov ../src/*.cpp --object-directory ../build/test/CMakeFiles/test_EmbeddedProto.dir/src/

  cd -

else
# Pass commands
case "$1" in
  -l|--local)
    # Run the code coverage for local machine analysis
    rm -rf ./code_coverage_report/*
    lcov --directory ./build/test --capture --output-file ./code_coverage_report/total_code_coverage.info -rc lcov_branch_coverage=1
    lcov --remove ./code_coverage_report/total_code_coverage.info $PWD'/external/googletest/*' '/usr/include/*' -o ./code_coverage_report/filtered_code_coverage.info
    genhtml ./code_coverage_report/filtered_code_coverage.info --branch-coverage --output-directory ./code_coverage_report
    ;;
  *)
    ;;
esac
fi