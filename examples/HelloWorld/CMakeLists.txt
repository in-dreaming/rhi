add_executable(HelloWorld
    main.c
)

set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_OUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)

find_program(SLANGC NAMES slangc slangc.exe
    HINTS ${CMAKE_BINARY_DIR}/RelWithDebInfo/bin
          ${CMAKE_BINARY_DIR}/bin
)

if(SLANGC)
    add_custom_command(
        OUTPUT ${SHADER_OUT_DIR}/tri-vert.spv
               ${SHADER_OUT_DIR}/tri-frag.spv
               ${SHADER_OUT_DIR}/tri-vert.dxil
               ${SHADER_OUT_DIR}/tri-frag.dxil
        COMMAND ${SLANGC} -target spirv -profile spirv_1_3 -entry vertMain -o ${SHADER_OUT_DIR}/tri-vert.spv ${SHADER_SRC_DIR}/tri.slang
        COMMAND ${SLANGC} -target spirv -profile spirv_1_3 -entry fragMain -o ${SHADER_OUT_DIR}/tri-frag.spv ${SHADER_SRC_DIR}/tri.slang
        COMMAND ${SLANGC} -target dxil -profile sm_6_0 -entry vertMain -o ${SHADER_OUT_DIR}/tri-vert.dxil ${SHADER_SRC_DIR}/tri.slang
        COMMAND ${SLANGC} -target dxil -profile sm_6_0 -entry fragMain -o ${SHADER_OUT_DIR}/tri-frag.dxil ${SHADER_SRC_DIR}/tri.slang
        DEPENDS ${SHADER_SRC_DIR}/tri.slang
        COMMENT "Compiling shaders with slangc"
    )
    add_custom_target(compile_shaders_helloworld ALL DEPENDS
        ${SHADER_OUT_DIR}/tri-vert.spv
        ${SHADER_OUT_DIR}/tri-frag.spv
        ${SHADER_OUT_DIR}/tri-vert.dxil
        ${SHADER_OUT_DIR}/tri-frag.dxil
    )
    add_dependencies(HelloWorld compile_shaders_helloworld)
else()
    message(WARNING "slangc not found - shader compilation skipped. Pre-compiled shaders required.")
endif()

add_custom_command(TARGET HelloWorld POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_SRC_DIR} $<TARGET_FILE_DIR:HelloWorld>/shaders
)

target_link_libraries(HelloWorld PRIVATE
    rhi
    shader
    SDL3::SDL3-shared
)
