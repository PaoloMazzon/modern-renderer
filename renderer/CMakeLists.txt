project(modern_renderer)

set(EXTERNAL_CXX_FILES ${CMAKE_SOURCE_DIR}/3rd/SPIRV-Reflect/spirv_reflect.cpp)
set(CXX_FILES
        src/Core.cpp
        src/Logging.cpp
        src/Renderer.cpp
)

add_library(${PROJECT_NAME} ${CXX_FILES} ${EXTERNAL_CXX_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC include/)

# Add dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        spdlog::spdlog
        SDL3::SDL3
)
