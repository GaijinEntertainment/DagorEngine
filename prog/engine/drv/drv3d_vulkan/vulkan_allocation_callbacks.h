// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define ALLOW_VULKAN_ALLOCATION_CALLBACKS 1

#include "vulkan_api.h"

#define ALLOCATION_TYPE_LIST                  \
  ALLOCATION_TYPE(device)                     \
  ALLOCATION_TYPE(instance)                   \
  ALLOCATION_TYPE(command_pool)               \
  ALLOCATION_TYPE(fence)                      \
  ALLOCATION_TYPE(event)                      \
  ALLOCATION_TYPE(query_pool)                 \
  ALLOCATION_TYPE(semaphore)                  \
  ALLOCATION_TYPE(pipeline)                   \
  ALLOCATION_TYPE(pipeline_layout)            \
  ALLOCATION_TYPE(pipeline_cache)             \
  ALLOCATION_TYPE(shader_module)              \
  ALLOCATION_TYPE(descriptor_pool)            \
  ALLOCATION_TYPE(descriptor_set_layout)      \
  ALLOCATION_TYPE(descriptor_update_template) \
  ALLOCATION_TYPE(render_pass)                \
  ALLOCATION_TYPE(framebuffer)                \
  ALLOCATION_TYPE(swapchain)                  \
  ALLOCATION_TYPE(device_memory)              \
  ALLOCATION_TYPE(buffer)                     \
  ALLOCATION_TYPE(buffer_view)                \
  ALLOCATION_TYPE(image)                      \
  ALLOCATION_TYPE(image_view)                 \
  ALLOCATION_TYPE(sampler)                    \
  ALLOCATION_TYPE(acceleration_structure)

namespace drv3d_vulkan
{
#if ALLOW_VULKAN_ALLOCATION_CALLBACKS

enum class AllocationType
{
#define ALLOCATION_TYPE(var) var,
  ALLOCATION_TYPE_LIST
#undef ALLOCATION_TYPE
    COUNT
};

VkAllocationCallbacks *getAllocationCallbacksPtr(AllocationType type);
#define VKALLOC(type) getAllocationCallbacksPtr(AllocationType::type)
void printAllocationCallbacksStatistics();

#else

#define VKALLOC(type) nullptr
inline void printAllocationCallbacksStatistics() {}

#endif

} // namespace drv3d_vulkan