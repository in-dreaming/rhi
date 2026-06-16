cmake_minimum_required(VERSION 3.22)

project(rhi_project LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(RHI_BUILD_EXAMPLES "Build examples" ON)

add_subdirectory(modules/3rd/sdl)
add_subdirectory(modules/3rd/slang)
add_subdirectory(modules/rhi)
add_subdirectory(modules/shader)

if(RHI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
