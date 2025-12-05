cmake_minimum_required(VERSION 3.15)
project(quick_test VERSION 1.0)
set(CMAKE_CXX_STANDARD 20)

set(project_dir ${CMAKE_CURRENT_SOURCE_DIR})

# 3rd
set(third_party_dir ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# opencv
# set(opencv_include_dir ${third_party_dir}/opencv480/include)
# set(opencv_lib ${third_party_dir}/opencv480/lib/opencv_world480.lib)

# add_subdirectory(src)
add_subdirectory(test)