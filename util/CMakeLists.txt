set(lib "util_lib")
add_subdirectory(infra)

add_library(${lib} INTERFACE)
target_link_libraries(${lib} INTERFACE
    infra_lib
)