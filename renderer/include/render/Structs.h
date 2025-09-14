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
    MVR_RESULT_FAILURE = -1, ///< Unknown critical failure
    MVR_RESULT_SUCCESS = 0,  ///< Everything worked fine
} MVR_Result;

/// \brief Tells the initialize function all necessary options to start the renderer
typedef struct MVR_InitializeParams_s {
    SDL_Window *window; ///< SDL window that was created with the `SDL_WINDOW_VULKAN` flag
    bool debug;         ///< Enables debug features like Vulkan compatibility layers
} MVR_InitializeParams;

#ifdef __cplusplus
};
#endif