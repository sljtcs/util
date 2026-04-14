set(test_dir "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>")
set(src_source
    # "main.cpp"
    "mainWin.cpp"
)

set(exe "dTest")
add_executable(${exe} ${src_source})
target_include_directories(${exe} PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(${exe} PUBLIC
    ${DOCTEST_INCLUDE_DIR}
)

target_link_libraries(${exe} PUBLIC
    util_lib
    foundation::system
)
set_target_properties(${exe} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${test_dir}
    WIN32_EXECUTABLE TRUE
)

# target_include_directories(${exe} PRIVATE
#     ${LUA_INCLUDE_DIR}
# )
# target_link_libraries(${exe) PUBLIC
#     ${LUA_LIB}
# )