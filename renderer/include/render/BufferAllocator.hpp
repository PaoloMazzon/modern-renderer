/// \brief C++ declaration of the temporary buffer abstraction's class
#pragma once

namespace MVRender {
    // A paging temporary buffer allocator. There should be one of these
    // per frame in flight to allow users to allocate arbitrary temporary
    // buffers in vram.
    class BufferAllocator {
    public:
        BufferAllocator() = default;
        // TODO: The rest of this
    };
}