set(lib "infra_lib")
add_subdirectory(file)
add_subdirectory(network)

add_library(${lib} INTERFACE)
target_include_directories(${lib} INTERFACE ${CMAKE_SOURCE_DIR})
target_link_libraries(${lib} INTERFACE
    foundation::file
    foundation::network
)