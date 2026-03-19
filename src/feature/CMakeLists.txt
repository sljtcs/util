add_subdirectory(cpp20)
add_subdirectory(cpp23)

add_library(feature_lib INTERFACE)
target_sources(feature_lib
    PUBLIC
        $<TARGET_OBJECTS:feature_20_obj>
        $<TARGET_OBJECTS:feature_23_obj>
)