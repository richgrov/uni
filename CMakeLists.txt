cmake_minimum_required(VERSION 3.1)
project(uni C)

set(CMAKE_C_STANDARD 99)

add_subdirectory(src)

if (CMAKE_PROJECT_NAME STREQUAL uni)
    add_subdirectory(example)
endif()
