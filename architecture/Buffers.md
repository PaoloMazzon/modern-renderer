# Temporary and permanent buffers
There are two kinds of buffers and they are both handled differently. From a
high-level,

 + Permanent buffers exist until the user explicitly frees them.
 + Temporary buffers are only valid until the next call to `mvr_PresentFrame`,
   or more explicitly, the next call to `MVRender::Renderer::end_frame`.

Internally, they are handled quite differently as well. They are both
represented to the user as a `MVR_Buffer`, and internally they are both
`MVRender::Buffer`, but they both have different ownership and clean-up.

## Overview

Temporary:

 + **Created by** `BufferAllocator`
 + **Belongs to** `BufferAllocator`
 + **Lifetime valid until** the next call to `MVRender::Renderer::end_frame`

Permanent:

+ **Created by** `Renderer`
+ **Belongs to** caller of `mvr_CreateBuffer`
+ **Lifetime valid until** user calls `mvr_DestroyBuffer` on it

## Temporary
Temporary buffers are created and managed by `MVRender::BufferAllocator`, and
there is one `BufferAllocator` per frame-in-flight (stored in the renderer
singleton). When the user requests a temporary buffer, the renderer asks the
current FIF's `BufferAllocator` to make one, and `BufferAllocator` then finds
the space for it in an existing buffer page or makes a new one to accommodate
it. When a new `MVRender::Buffer` is created for these, no parent is provided
and as such the internal `Buffer` will do absolutely nothing inside the
destructor as the memory it refers to belongs to the buffer pages in the
`BufferAllocator` it came from (and is overwritten in following frames).

## Permanent
Permanent buffers are trickier because their lifetime is indeterminant. The
problem is essentially that users can free them whenever, and we don't want to
halt the GPU to free them from memory. So to explain their lifetime, the
top-level renderer is responsible for their creation and deletion as opposed to
it being `BufferAllocator` (as is the case for temporary buffers). When the user
requests a new permanent buffer, it is created instantly but the current frame's
`BufferAllocator` is used as the transfer buffer. Because of the guarantees
provided by the `BufferAllocator`, the permanent buffer will have been
transferred to by the time it gets used.

Permanent buffers are not stored anywhere besides the `MVR_Buffer` handle the
user has (which is just a pointer to an instance of `MVRender::Buffer`). When
the user destroys one, the call to `mvr_DestroyBuffer` calls
`free_permanent_buffer` which in turn adds it to this FIF's free list (free
lists are freed at the start of each FIF).

For example, imagine you have 3 frames-in-flight, and you attempt to free buffer
`b` during FIF index #0. The user may have queued some work that uses buffer `b`
during this frame (FIF #0), meaning we are only sure the GPU will no longer be
using this buffer once we are sure FIF #0 is done with its work. We are only
sure FIF #0 is done with its work once we wait for the FIF #0 timeline semaphore
so we must wait until then to free it.

```
       +-------------------+-------------------+-------------------+
     FIF #0              FIF #1              FIF #2              FIF #0
       ^                   ^                   ^                   ^
       |                   |                   |                   |
User "frees" here          |                   |    Resource actually freed here
                           |                   |
                  (FIF #0 potentially still using buffer b)
```