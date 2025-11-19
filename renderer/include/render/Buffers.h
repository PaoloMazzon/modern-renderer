/// \brief Tools for allocating both temporary and permanent buffers
///
/// Modern renderer has two buffer types:
///   - Temporary (per frame) buffers
///   - Permanent buffers
/// Temporary buffers are only valid until the end of the frame (ie, you call
/// mvr_PresentFrame), they are for things like temporary shapes or
/// uniform buffers or things like that. Permanent buffers need to be
/// manually managed by the user.
#pragma once
#include "render/Structs.h"

/// \brief Creates a temporary buffer of given size and given data
/// \param size Size of the buffer in bytes
/// \param data Binary data of at least size size to copy into the MVR_Buffer
/// \param buffer Pointer to a buffer handle where the new buffer will be placed
/// \return Returns an MVR_Result status code
MVR_API MVR_Result mvr_CreateTempBuffer(uint64_t size, void *data, MVR_Buffer *buffer);

/// \brief Allocates a temporary buffer of given size, returning the buffer and a memory handle
/// \param size Size of the buffer in bytes
/// \param data Handle that will be given a pointer to the start of the buffer's memory
/// \param buffer Pointer to a buffer handle where the new buffer will be placed
/// \return Returns an MVR_Result status code
///
/// With this function, a pointer is provided to where the buffer's memory lives. You call
/// this function, then you fill in your desired data later with the data pointer.
MVR_API MVR_Result mvr_AllocateTempBuffer(uint64_t size, void **data, MVR_Buffer *buffer);

/// \brief Creates a permanent buffer of given size and given data
/// \param size Size of the buffer in bytes
/// \param data Binary data of at least size size to copy into the MVR_Buffer
/// \param buffer Pointer to a buffer handle where the new buffer will be placed
/// \return Returns an MVR_Result status code
MVR_API MVR_Result mvr_CreateBuffer(uint64_t size, void *data, MVR_Buffer *buffer);

/// \brief Allocates a permanent buffer of given size, returning the buffer and a memory handle
/// \param size Size of the buffer in bytes
/// \param data Handle that will be given a pointer to the start of the buffer's memory
/// \param buffer Pointer to a buffer handle where the new buffer will be placed
/// \return Returns an MVR_Result status code
///
/// With this function, a pointer is provided to where the buffer's memory lives. You call
/// this function, then you fill in your desired data later with the data pointer.
MVR_API MVR_Result mvr_AllocateBuffer(uint64_t size, void **data, MVR_Buffer *buffer);

/// \brief Destroys a permanent MVR_Buffer
/// \param buffer Buffer to destroy
MVR_API void mvr_DestroyBuffer(MVR_Buffer buffer);