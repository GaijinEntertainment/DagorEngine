// macro-generated command structs & their impls
#pragma once

#include "buffer_resource.h"
#include "graphics_state.h"
#include "fence_manager.h"
#include "swapchain.h"
#include "debug_ui.h"
#include "async_completion_state.h"
#include "execution_state.h"
#include "pipeline_state.h"
namespace drv3d_vulkan
{

#define VULKAN_BEGIN_CONTEXT_COMMAND(name) \
  struct Cmd##name                         \
  {                                        \
    static const char *getName()           \
    {                                      \
      return "Cmd" #name;                  \
    }
#define VULKAN_END_CONTEXT_COMMAND \
  }                                \
  ;

#define VULKAN_CONTEXT_COMMAND_PARAM(type, name)             type name;
#define VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) type name[size];
#include "device_context_cmd.inc"
#undef VULKAN_BEGIN_CONTEXT_COMMAND
#undef VULKAN_END_CONTEXT_COMMAND
#undef VULKAN_CONTEXT_COMMAND_PARAM
#undef VULKAN_CONTEXT_COMMAND_PARAM_ARRAY

typedef VariantVector<
#define VULKAN_BEGIN_CONTEXT_COMMAND(name) Cmd##name,
#define VULKAN_END_CONTEXT_COMMAND
#define VULKAN_CONTEXT_COMMAND_PARAM(type, name)
#define VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.inc"
#undef VULKAN_BEGIN_CONTEXT_COMMAND
#undef VULKAN_END_CONTEXT_COMMAND
#undef VULKAN_CONTEXT_COMMAND_PARAM
#undef VULKAN_CONTEXT_COMMAND_PARAM_ARRAY
  void>
  AnyCommandStore;

} // namespace drv3d_vulkan
