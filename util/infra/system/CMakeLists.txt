set(src_source
    "command.cpp"
)

add_library(foundation_system ${src_source})
add_library(foundation::system ALIAS foundation_system)