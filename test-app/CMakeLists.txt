project(modern_renderer_app)

set(CXX_FILES src/main.cpp)

add_executable(${PROJECT_NAME} ${CXX_FILES})

target_link_libraries(${PROJECT_NAME} PRIVATE modern_renderer spdlog::spdlog SDL3::SDL3)

target_include_directories(${PROJECT_NAME} PUBLIC include/)
