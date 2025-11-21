/// \brief Constants in use across the whole renderer
#pragma once
#include <cinttypes>

namespace MVRender {
    constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    constexpr uint64_t VRAM_PAGE_SIZE = 256 * 1024;
}