project(modern_renderer)

set(CXX_FILES src/core.cpp)

add_library(${PROJECT_NAME} ${CXX_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC include/)

# Add dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        GPUOpen::VulkanMemoryAllocator
        SDL3::SDL3
)
