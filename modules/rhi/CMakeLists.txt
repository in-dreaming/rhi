add_library(rhi SHARED
    src/rhi.c
)

target_include_directories(rhi PUBLIC
    include
)

target_link_libraries(rhi PRIVATE
    SDL3::SDL3-shared
)

target_compile_definitions(rhi PRIVATE RHI_BUILD_DLL)

set_target_properties(rhi PROPERTIES
    C_STANDARD 11
    POSITION_INDEPENDENT_CODE ON
)
