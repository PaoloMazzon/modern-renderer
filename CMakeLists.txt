cmake_minimum_required(VERSION 3.20)
project(modern_renderer)

option(BUILD_SAMPLE_APP "Build test app for the renderer" OFF)
option(BUILD_TESTS "Build the test suite for the renderer" OFF)
option(BUILD_WITH_COVERAGE "Build with --coverage (Unix only)" OFF)
option(BUILD_WITH_PLAYGROUND "Build with debug playground" OFF)

# Let subprojects see 3rd/ modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/3rd")

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tells spdlog to use our fmt instead of built-in to C++ one
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(SPDLOG_FMT_EXTERNAL ON)

# Dependencies - other projects need em
add_subdirectory(3rd/vk-bootstrap)
add_subdirectory(3rd/SDL)
add_subdirectory(3rd/Catch2)
add_subdirectory(3rd/VulkanMemoryAllocator)
add_subdirectory(3rd/fmt)
add_subdirectory(3rd/spdlog)
add_subdirectory(3rd/SPIRV-Reflect)
add_subdirectory(3rd/volk)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
    if (BUILD_WITH_COVERAGE)
        add_compile_options(--coverage)
        message("Added coverage compile option.")
    endif()
endif()

if (BUILD_WITH_PLAYGROUND)
    add_compile_options(-DBUILD_PLAYGROUND)
endif()

set(EXTERNAL_CXX_FILES ${CMAKE_SOURCE_DIR}/3rd/SPIRV-Reflect/spirv_reflect.cpp)
set(CXX_FILES
        renderer/src/Core.cpp
        renderer/src/Logging.cpp
        renderer/src/Renderer.cpp
        renderer/src/RendererUtil.cpp
        renderer/src/Buffers.cpp
        renderer/src/CompileHeaders.cpp
        renderer/src/Playground.cpp
        renderer/src/Util.cpp
)

add_library(${PROJECT_NAME} ${CXX_FILES} ${EXTERNAL_CXX_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC renderer/include/)

# Add dependencies
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        fmt::fmt
        spdlog::spdlog
        SDL3::SDL3
        volk
)

if (BUILD_WITH_COVERAGE AND NOT MSVC)
    target_link_libraries(${PROJECT_NAME}
            PUBLIC
            --coverage
    )
    message("Linked coverage.")
endif()

# Build app and tests
if (BUILD_SAMPLE_APP)
    add_subdirectory(test-app)
endif()

if (BUILD_TESTS)
    add_subdirectory(tests)
endif()