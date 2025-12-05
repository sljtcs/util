cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 20)
project(test)

set(test_dir "${CMAKE_SOURCE_DIR}/bin_test/$<CONFIG>")
set(src_source
    "main.cpp"
)

set(exe "quick_test")
add_executable(${exe} ${src_source})

target_include_directories(${exe} PRIVATE
    ${project_dir}
)

set_target_properties(${exe} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${test_dir}
)