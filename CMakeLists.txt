cmake_minimum_required(VERSION 3.24)
project(voxel_raytracer)

set(BINARY_DIR ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${BINARY_DIR})
make_directory(${BINARY_DIR})

configure_file(LICENSE.txt ${BINARY_DIR} COPYONLY)
configure_file(README.md ${BINARY_DIR} COPYONLY)

set(GLM_BUILD_LIBRARY OFF)
add_subdirectory(lib/FastNoiseLite)
add_subdirectory(lib/SDL)
add_subdirectory(lib/glm)
add_subdirectory(lib/imgui)
add_subdirectory(lib/json)
add_executable(voxel_raytracer WIN32
    src/block.cpp
    src/camera.cpp
    src/chunk.cpp
    src/helpers.cpp
    src/main.cpp
    src/world.cpp
)
target_link_libraries(voxel_raytracer SDL3::SDL3 FastNoiseLite glm imgui nlohmann_json)
set_target_properties(voxel_raytracer PROPERTIES CXX_STANDARD 23)
target_precompile_headers(voxel_raytracer PRIVATE
    <array>
    <algorithm>
    <cmath>
    <cstdint>
    <exception>
    <filesystem>
    <format>
    <fstream>
    <glm/glm.hpp>
    <glm/gtc/constants.hpp>
    <imgui.h>
    <imgui_impl_sdl3.h>
    <imgui_impl_sdlgpu3.h>
    <iterator>
    <limits>
    <nlohmann/json.hpp>
    <string>
    <string_view>
    <utility>
    <vector>
)

find_program(SHADERCROSS shadercross)
function(add_shader FILE)
    set(DEPENDS ${ARGN})
    set(HLSL ${CMAKE_SOURCE_DIR}/shaders/${FILE})
    set(SPV ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.spv)
    set(MSL ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.msl)
    set(JSON ${CMAKE_SOURCE_DIR}/shaders/bin/${FILE}.json)
    function(compile OUTPUT)
        add_custom_command(
            OUTPUT ${OUTPUT}
            COMMAND ${SHADERCROSS} ${HLSL} -s hlsl -o ${OUTPUT} -I src
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${HLSL} ${DEPENDS}
            COMMENT ${OUTPUT}
        )
        get_filename_component(NAME ${OUTPUT} NAME)
        string(REPLACE . _ NAME ${NAME})
        set(NAME compile_${NAME})
        add_custom_target(${NAME} DEPENDS ${OUTPUT})
        add_dependencies(voxel_raytracer ${NAME})
    endfunction()
    if (EXISTS ${SHADERCROSS})
        compile(${SPV})
        compile(${MSL})
        compile(${JSON})
    else()
        message("Using prebuilts since SDL_shadercross is missing")
    endif()
    function(package OUTPUT)
        get_filename_component(NAME ${OUTPUT} NAME)
        set(BINARY ${BINARY_DIR}/${NAME})
        add_custom_command(
            OUTPUT ${BINARY}
            COMMAND ${CMAKE_COMMAND} -E copy ${OUTPUT} ${BINARY}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DEPENDS ${OUTPUT}
            COMMENT ${BINARY}
        )
        string(REPLACE . _ NAME ${NAME})
        set(NAME package_${NAME})
        add_custom_target(${NAME} DEPENDS ${BINARY})
        add_dependencies(voxel_raytracer ${NAME})
    endfunction()
    if(APPLE)
        package(${MSL})
    else()
        package(${SPV})
    endif()
    package(${JSON})
endfunction()
add_shader(clear_blocks.comp shaders/shader.hlsl src/config.h)
add_shader(clear_texture.comp shaders/shader.hlsl src/config.h)
add_shader(raytrace.comp shaders/shader.hlsl shaders/random.hlsl src/config.h)
add_shader(sample_texture.comp shaders/shader.hlsl src/config.h)
add_shader(set_blocks.comp shaders/shader.hlsl src/config.h)
add_shader(set_chunks.comp shaders/shader.hlsl src/config.h)