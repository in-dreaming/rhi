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

set(SLANG_ENABLE_OPTIX OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_SLANG_RHI OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SLANG_ENABLE_CUDA OFF CACHE BOOL "" FORCE)

add_subdirectory(modules/3rd/slang)
add_subdirectory(modules/rhi)
add_subdirectory(modules/shader)
add_subdirectory(modules/render)
add_subdirectory(modules/plugins)

if(RHI_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
