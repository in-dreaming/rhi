add_executable(HelloWorld
    main.c
)

find_program(SLANGC NAMES slangc slangc.exe
    HINTS ${CMAKE_BINARY_DIR}/RelWithDebInfo/bin
          ${CMAKE_BINARY_DIR}/bin
)

set(SHADER_SRC ${CMAKE_CURRENT_SOURCE_DIR}/shaders/tri.slang)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SHADER_OUT_DIR})

set(SHADER_OUTPUTS
    ${SHADER_OUT_DIR}/tri-vert.spv
    ${SHADER_OUT_DIR}/tri-frag.spv
    ${SHADER_OUT_DIR}/tri-vert.dxil
    ${SHADER_OUT_DIR}/tri-frag.dxil
)

if(SLANGC)
    add_custom_command(
        OUTPUT ${SHADER_OUTPUTS}
        COMMAND ${SLANGC} -target spirv -profile spirv_1_3 -entry vertMain -o ${SHADER_OUT_DIR}/tri-vert.spv ${SHADER_SRC}
        COMMAND ${SLANGC} -target spirv -profile spirv_1_3 -entry fragMain -o ${SHADER_OUT_DIR}/tri-frag.spv ${SHADER_SRC}
        COMMAND ${SLANGC} -target dxil -profile sm_6_0 -entry vertMain -o ${SHADER_OUT_DIR}/tri-vert.dxil ${SHADER_SRC}
        COMMAND ${SLANGC} -target dxil -profile sm_6_0 -entry fragMain -o ${SHADER_OUT_DIR}/tri-frag.dxil ${SHADER_SRC}
        DEPENDS ${SHADER_SRC}
        COMMENT "Compiling shaders with slangc"
    )
    add_custom_target(compile_shaders_helloworld DEPENDS ${SHADER_OUTPUTS})
    add_dependencies(HelloWorld compile_shaders_helloworld)
else()
    message(WARNING "slangc not found - shader compilation skipped. Pre-compiled shaders required.")
endif()

target_link_libraries(HelloWorld PRIVATE
    rhi
    SDL3::SDL3-shared
)
