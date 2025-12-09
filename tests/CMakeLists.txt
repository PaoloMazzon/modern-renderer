project(modern_renderer_tests)

enable_testing()

if (NOT MSVC AND BUILD_WITH_COVERAGE)
    add_compile_options(--coverage)
endif()

add_executable(${PROJECT_NAME}
        src/test.cpp
)

target_link_libraries(${PROJECT_NAME}
        PRIVATE
        Catch2::Catch2WithMain
        modern_renderer
)

if (BUILD_WITH_COVERAGE AND NOT MSVC)
    target_link_libraries(${PROJECT_NAME}
            PUBLIC
            --coverage
    )
endif()

include(CTest)
include(Catch)

catch_discover_tests(${PROJECT_NAME})