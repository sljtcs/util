cmake_minimum_required(VERSION 3.31)
project(quick_test VERSION 1.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(project_dir ${CMAKE_CURRENT_SOURCE_DIR})

# 3rd party
include(${CMAKE_SOURCE_DIR}/arch/cmake/3rd.cmake)

add_subdirectory(util)
add_subdirectory(src)
add_subdirectory(test)