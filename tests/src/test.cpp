#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <render/Core.h>
#include <render/Logging.hpp>

TEST_CASE("User-facing error messages") {
    MVRender::set_error_message("123abc");
    REQUIRE(strcmp(mvr_GetError(), "123abc") == 0);
}
