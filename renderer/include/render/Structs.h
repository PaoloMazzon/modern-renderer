/// \brief Defines user-facing structs and enums that are needed to communicate to the renderer
#pragma once
#include <SDL3/SDL.h>

#define MVR_API extern "C"

#ifdef __cplusplus
extern "C" {
#endif

/// \brief Core functions may return a result code, negative result codes represent a fatal
/// error. Fatal errors mean the renderer can no longer continue operating, positive result
/// codes represent less severe and recoverable problems.
typedef enum {
    MVR_RESULT_FAILURE = -1,               ///< Unknown critical failure
    MVR_RESULT_CRITICAL_SDL_ERROR = -2,    ///< Error from SDL that cannot be recovered
    MVR_RESULT_NO_DEVICE = -3,             ///< There is no available and compatible GPU
    MVR_RESULT_CRITICAL_VULKAN_ERROR = -3, ///< General error from Vulkan that cannot be recovered
    MVR_RESULT_SDL_ERROR = 1,              ///< General SDL error
    MVR_RESULT_VULKAN_ERROR = 2,           ///< General Vulkan error
    MVR_RESULT_SUCCESS = 0,                ///< Everything worked fine
} MVR_Result;

/// \brief Render present modes, only vsync is guaranteed. If you request something other
/// that vsync and it isn't available on the host machine, vsync will be used instead.
typedef enum {
    MVR_PRESENT_MODE_VSYNC = 0,         ///< Wait for vblank, no tearing
    MVR_PRESENT_MODE_TRIPLE_BUFFER = 1, ///< Mailbox, unlimited framerate/latency with no tearing
    MVR_PRESENT_MODE_IMMEDIATE = 2,     ///< Replace the image as soon as possible, tearing is possible
} MVR_PresentMode;

/// \brief Tells the initialize function all necessary options to start the renderer
typedef struct MVR_InitializeParams_s {
    SDL_Window *window;           ///< SDL window that was created with the `SDL_WINDOW_VULKAN` flag
    bool debug;                   ///< Enables debug features like Vulkan validation layers
    MVR_PresentMode present_mode; ///< Initial present mode
} MVR_InitializeParams;

#ifdef __cplusplus
};
#endif