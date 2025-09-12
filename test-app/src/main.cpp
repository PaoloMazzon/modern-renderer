#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

const char *WINDOW_TITLE = "Test App";
const int WINDOW_WIDTH   = 800;
const int WINDOW_HEIGHT  = 600;
using defer = std::shared_ptr<void>;

int main() {
    // Create window for test program
    spdlog::info("Creating window.");
    SDL_Window *window = SDL_CreateWindow(
            WINDOW_TITLE,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    defer _(nullptr, [&window](...) { SDL_DestroyWindow(window); });

    if (!window) {
        spdlog::error("Failed to create window, SDL error {}", SDL_GetError());
        return 1;
    }

    spdlog::info("Created window successfully.");

    // Main event loop
    SDL_Event e;
    bool keep_window_open = true;
    while (keep_window_open) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                keep_window_open = false;
                spdlog::info("Received quit signal.");
            }
        }
    }

    spdlog::info("Quit program.");
    return 0;
}
