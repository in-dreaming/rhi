add_library(shader STATIC
    src/compiler.cpp
)

target_include_directories(shader PUBLIC
    include
)

target_link_libraries(shader PUBLIC
    slang
)

target_compile_definitions(shader PRIVATE
    _CRT_SECURE_NO_WARNINGS
)

set_target_properties(shader PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)
