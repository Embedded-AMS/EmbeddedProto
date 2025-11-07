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

cmake_minimum_required(VERSION 3.21)

project(EmbeddedProto
    HOMEPAGE_URL "https://EmbeddedProto.com"
)

set(EMBEDDED_PROTO_GEN_PATH ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "Root path for EmbeddedProto code generator")
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/venv" AND EMBEDDED_PROTO_GEN_PATH STREQUAL CMAKE_CURRENT_LIST_DIR)
    find_package(Python 3 REQUIRED)
    execute_process(
        COMMAND ${Python_EXECUTABLE} setup.py --ignore_version_diff
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endif()

include(cmake/GenerateEmbeddedProto.cmake)

add_library(${PROJECT_NAME} STATIC)
target_sources(${PROJECT_NAME} PRIVATE
    src/Fields.cpp
    src/MessageInterface.cpp
    src/ReadBufferSection.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
    src
)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -fprofile-arcs -ftest-coverage")
    set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

    add_subdirectory(external/googletest)

    file(GLOB src_files
        "test/*.cpp"
    )
    add_executable(test_${PROJECT_NAME} ${src_files})

    target_include_directories(test_${PROJECT_NAME}
        PRIVATE
            test
            test/mock
            ${CMAKE_CURRENT_BINARY_DIR}/EAMS
    )
    target_link_libraries(test_${PROJECT_NAME}
        PRIVATE
            ${PROJECT_NAME}
            gtest
            gmock
    )
    generate_embedded_proto(PROTO_HEADERS_1
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/EAMS
        INCLUDE_DIRS
            test/proto
        PROTO_FILES
            test/proto/simple_types.proto
            test/proto/nested_message.proto
            test/proto/repeated_fields.proto
            test/proto/oneof_fields.proto
            # Deliberately do not manually generate file_to_include.proto and subfolder/file_to_include_from_subfolder.proto 
            # to test the automatic generation of files from including them in include_other_files.proto.
            test/proto/include_other_files.proto
            test/proto/empty_message.proto
            test/proto/optional_fields.proto
    )
    generate_embedded_proto(PROTO_HEADERS_2
        OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/EAMS
        INCLUDE_DIRS
            test/proto
            generator
        PROTO_FILES
            test/proto/string_bytes.proto
            test/proto/field_options.proto
    )
    add_custom_target(generate_protos
        DEPENDS
            ${PROTO_HEADERS_1}
            ${PROTO_HEADERS_2}
    )
    add_dependencies(test_${PROJECT_NAME} generate_protos)
endif()
