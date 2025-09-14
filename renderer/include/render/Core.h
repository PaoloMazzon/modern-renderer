/// \brief Top-level user-facing functions
#pragma once
#include "render/Structs.h"

/// \brief Initializes the renderer
/// \param params Parameters to start the renderer, may not be null
/// \return Returns MVR_RESULT_SUCCESS if everything worked
MVR_API MVR_Result mvr_Initialize(MVR_InitializeParams *params);

/// \brief Frees all renderer resources
MVR_API void mvr_Quit();

/// \brief Returns an error message or an empty string if there is none.
MVR_API const char *mvr_GetError();