cmake_minimum_required(VERSION 3.5)

project(test_EmbeddedProto)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall -fprofile-arcs -ftest-coverage")
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE 1)

add_subdirectory(external/googletest)

file(GLOB src_files
    "src/*.cpp"
    "test/*.cpp"
    "build/EAMS/*cpp"
)

include_directories(test test/mock src build/EAMS)
include_directories(external/googletest/googletest external/googletest/googletest/include
                    external/googletest/googlemock external/googletest/googlemock/include)

add_executable(test_EmbeddedProto ${src_files})
target_link_libraries(test_EmbeddedProto gtest gmock)