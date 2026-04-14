add_subdirectory(http)

add_library(foundation_network INTERFACE)
target_link_libraries(foundation_network INTERFACE
    http_lib
)
add_library(foundation::network ALIAS foundation_network)