#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <render/Core.h>

const char *WINDOW_TITLE = "Test App";
const int WINDOW_WIDTH   = 800;
const int WINDOW_HEIGHT  = 600;

int main() {
    if (!SDL_Init(SDL_INIT_EVENTS)) {
        spdlog::info("Failed to initialize SDL, SDL error {}", SDL_GetError());
        return -1;
    }

    // Create window for test program
    spdlog::info("Creating window.", nullptr);
    SDL_Window *window = SDL_CreateWindow(
            WINDOW_TITLE,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    if (!window) {
        spdlog::error("Failed to create window, SDL error {}", SDL_GetError());
        return 1;
    }

    spdlog::info("Created window successfully.", nullptr);

    // Initialize renderer
    MVR_InitializeParams params = {
            .window = window,
            .debug = true,
    };
    MVR_Result result = mvr_Initialize(&params);

    if (result != MVR_RESULT_SUCCESS) {
        spdlog::error("Failed to create renderer.");
        return 1;
    }

    // Main event loop
    SDL_Event e;
    bool keep_window_open = true;
    while (keep_window_open) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                keep_window_open = false;
                spdlog::info("Received quit signal.", nullptr);
            }
        }

        mvr_PresentFrame();
    }

    mvr_Quit();
    SDL_DestroyWindow(window);
    SDL_Quit();
    spdlog::info("Quit program.", nullptr);
    return 0;
}
