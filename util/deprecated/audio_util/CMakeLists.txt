set(src_source
    "recorderDS.cpp"
    "recorderWASAPI.cpp"
    "lowRecorderWASAPI.cpp"
)

set(lib "audio_util_lib")
add_library(${lib} STATIC ${src_source})
target_include_directories(${lib} PUBLIC
    ${RTAUDIO_INCLUDE_DIR}
)
target_link_libraries(${lib} PRIVATE
    ${RTAUDIO_LIB}
)