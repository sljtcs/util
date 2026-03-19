set(test_dir "${CMAKE_SOURCE_DIR}/bin_test/$<CONFIG>")
set(src_source
    "main.cpp"
)

set(exe "quick_test")
add_executable(${exe} ${src_source})

target_include_directories(${exe} PRIVATE
    ${project_dir}
)


target_link_libraries(${exe} PUBLIC
    util_lib
)

set_target_properties(${exe} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${test_dir}
)


target_include_directories(${exe} PRIVATE
    ${LUA_INCLUDE_DIR}
)
target_link_libraries(${exe} PUBLIC
    ${LUA_LIB}
)