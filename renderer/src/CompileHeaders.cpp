// This file exists to compile single-header libraries
#pragma warning(disable:4100)
#pragma warning(disable:4324)
#pragma warning(disable:4189)
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VOLK_HEADER_VERSION
#include <volk.h>
#include <vk_mem_alloc.h>