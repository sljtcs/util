set(src_source
    "libcurl_util.cpp"
    # "downloadClient.cpp"
    # "messageClient.cpp"
    "quickClient.cpp"
    "base/address.cpp"
    "sync/smdClient.cpp"
    # "sync/syncDownloadClient2.cpp"
)

set(lib "http_lib")
add_library(${lib} ${src_source})

target_include_directories(${lib} PUBLIC
    ${CURL_INCLUDE_DIR}
)
target_link_libraries(${lib} PRIVATE
    # CURL::libcurl
    ${CURL_LIB}
)