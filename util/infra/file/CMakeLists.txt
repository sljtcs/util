set(src_source
    "self.cpp"
    "path.cpp"
)

add_library(foundation_file ${src_source})
add_library(foundation::file ALIAS foundation_file)