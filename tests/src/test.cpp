#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <render/Core.h>
#include <render/Logging.hpp>
#include <render/Renderer.hpp>
#include <render/Buffers.h>

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

    // Test temporary buffers
    uint8_t garbage[100] = {0};
    void *data;
    MVR_Buffer temp_buffer;
    REQUIRE(mvr_CreateTempBuffer(100, garbage, &temp_buffer) == MVR_RESULT_SUCCESS);
    REQUIRE(mvr_AllocateTempBuffer(100, &data, &temp_buffer) == MVR_RESULT_SUCCESS);

    // Test permanent buffers
    MVR_Buffer permanent;
    REQUIRE(mvr_CreateBuffer(100, garbage, &permanent) == MVR_RESULT_SUCCESS);
    mvr_DestroyBuffer(permanent);

    renderer.quit_vulkan_headless();
}