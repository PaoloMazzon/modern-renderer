/// \brief Top-level user-facing functions
#pragma once
#include "render/Structs.h"

/// \brief Initializes the renderer
/// \param params Parameters to start the renderer, may not be null
/// \return Returns an MVR_Result, if its not MVR_RESULT_SUCCESS something went wrong.
MVR_API MVR_Result mvr_Initialize(MVR_InitializeParams *params);

/// \brief Does necessary presenting tasks at the end of each frame
/// \return Returns an MVR_Result, if its not MVR_RESULT_SUCCESS something went wrong.
MVR_API MVR_Result mvr_PresentFrame();

/// \brief Frees all renderer resources
MVR_API void mvr_Quit();

/// \brief Returns an error message or an empty string if there is none.
///
/// The return value of this function is only considered valid directly after
/// receiving a non-MVR_RESULT_SUCCESS from another function. If you call this
/// after a successful function this may return a value from a previous error
/// long before the call.
MVR_API const char *mvr_GetError();