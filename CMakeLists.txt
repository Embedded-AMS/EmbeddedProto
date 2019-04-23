cmake_minimum_required(VERSION 3.10)

project(test_EmbeddedProto)

set(CMAKE_CXX_FLAGS "-std=c++14 -Wall")


add_subdirectory(googletest)

file(GLOB src_files
    "test/*.cpp"
)

include_directories(test src)
include_directories(googletest/googletest googletest/googletest/include)

add_executable(test_EmbeddedProto ${src_files})
target_link_libraries(test_EmbeddedProto gtest)