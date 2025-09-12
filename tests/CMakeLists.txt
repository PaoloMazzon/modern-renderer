project(modern_renderer_tests)

enable_testing()

add_executable(${PROJECT_NAME}
        src/test.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        modern_renderer
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)

catch_discover_tests(${PROJECT_NAME})