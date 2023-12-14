#
#  Copyright (C) 2020-2024 Embedded AMS B.V. - All Rights Reserved
#
#  This file is part of Embedded Proto.
#
#  Embedded Proto is open source software: you can redistribute it and/or 
#  modify it under the terms of the GNU General Public License as published 
#  by the Free Software Foundation, version 3 of the license.
#
#  Embedded Proto  is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Embedded Proto. If not, see <https://www.gnu.org/licenses/>.
#
#  For commercial and closed source application please visit:
#  <https://EmbeddedProto.com/license/>.
#
#  Embedded AMS B.V.
#  Info:
#    info at EmbeddedProto dot com
#
#  Postal address:
#    Atoomweg 2
#    1627 LE, Hoorn
#    the Netherlands
#

cmake_minimum_required(VERSION 3.5)

project(test_EmbeddedProto)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

add_subdirectory(external/googletest)

file(GLOB src_files
    "src/*.cpp"
    "test/*.cpp"
    "build/EAMS/*.cpp"
)

include_directories(test test/mock src build/EAMS)
include_directories(external/googletest/googletest external/googletest/googletest/include
                    external/googletest/googlemock external/googletest/googlemock/include)

add_executable(test_EmbeddedProto ${src_files})
target_link_libraries(test_EmbeddedProto gtest gmock)