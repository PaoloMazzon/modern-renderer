cmake_minimum_required(VERSION 3.20)
project(modern_renderer_top LANGUAGES CXX)

# Let subprojects see 3rd/ modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/3rd")

# Global settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dependencies - other projects need em
add_subdirectory(3rd/vk-bootstrap)
add_subdirectory(3rd/SDL)
add_subdirectory(3rd/Catch2)
add_subdirectory(3rd/VulkanMemoryAllocator)
add_subdirectory(3rd/spdlog)
add_subdirectory(3rd/SPIRV-Reflect)

if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Core projects
add_subdirectory(renderer)
add_subdirectory(test-app)
add_subdirectory(tests)