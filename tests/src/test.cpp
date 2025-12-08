#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <render/Core.h>
#include <render/Logging.hpp>
#include <render/Renderer.hpp>

TEST_CASE("User-facing error messages") {
    MVRender::set_error_message("123abc");
    REQUIRE(strcmp(mvr_GetError(), "123abc") == 0);
    try {
        throw MVRender::Exception(MVR_RESULT_CRITICAL_SDL_ERROR, "garbage");
    } catch (MVRender::Exception& e) {
        REQUIRE(e.result() == MVR_RESULT_CRITICAL_SDL_ERROR);
        REQUIRE(strcmp(mvr_GetError(), "garbage") == 0);
    }

    try {
        MVRender::resolve_vulkan_error(VK_ERROR_DEVICE_LOST, true, "garbage");
    } catch (MVRender::Exception& e) {
        REQUIRE(e.result() == MVR_RESULT_CRITICAL_VULKAN_ERROR);
        REQUIRE(strcmp(mvr_GetError(), "garbage, Vulkan error VK_ERROR_DEVICE_LOST") == 0);
    }
}

TEST_CASE("Renderer integration test") {
    auto& renderer = MVRender::Renderer::instance();
    renderer.initialize_vulkan_headless();
    renderer.quit_vulkan_headless();
}