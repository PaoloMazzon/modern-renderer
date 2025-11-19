#include "render/Buffers.h"
#include "render/BufferAllocator.hpp"

MVRender::BufferAllocator::BufferAllocator(VkDeviceSize page_size) {
    m_page_size = page_size;
}

MVRender::BufferAllocator::~BufferAllocator() {
    // TODO: This
}

MVR_Buffer MVRender::BufferAllocator::allocate_temp_buffer(VkDeviceSize size, void **data) {
    // TODO: This
}

void MVRender::BufferAllocator::record_copy_commands(VkCommandBuffer command_buffer) {
    // TODO: This
}

void MVRender::BufferAllocator::begin_frame() {
    // TODO: This
}

MVR_API MVR_Result mvr_CreateTempBuffer(uint64_t size, void *data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API MVR_Result mvr_AllocateTempBuffer(uint64_t size, void **data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API MVR_Result mvr_CreateBuffer(uint64_t size, void *data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API MVR_Result mvr_AllocateBuffer(uint64_t size, void **data, MVR_Buffer *buffer) {
    // TODO: This
    return MVR_RESULT_FAILURE;
}

MVR_API void mvr_DestroyBuffer(MVR_Buffer buffer) {
    // TODO: This
}