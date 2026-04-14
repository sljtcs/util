set(src_source
    "file.cpp"
)

add_library(foundation_hash ${src_source})
add_library(foundation::hash ALIAS foundation_hash)

target_link_libraries(foundation_hash PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
)