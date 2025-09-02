// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// macro-generated command structs & their impls

#include <drv/3d/dag_commands.h>
#include <fsr_args.h>
#include <3d/dag_nvFeatures.h>

#include "util/variant_vector.h"
#include "buffer_resource.h"
#include "graphics_state.h"
#include "fence_manager.h"
#include "debug_ui.h"
#include "async_completion_state.h"
#include "execution_state.h"
#include "pipeline_state.h"
#include "memory_heap_resource.h"
#include "resource_manager.h"
#include "queries.h"

namespace drv3d_vulkan
{

struct SwapchainImage;

#define VULKAN_BEGIN_CONTEXT_COMMAND(name)               \
  struct Cmd##name                                       \
  {                                                      \
    static const char *getName() { return "Cmd" #name; } \
    inline void execute(ExecutionContext &ctx) const;

#define VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(name)         \
  struct Cmd##name                                         \
  {                                                        \
    static const char *getName() { return "CmdTR" #name; } \
    inline void execute(ExecutionContext &ctx) const;

#define VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(name)    \
  struct Cmd##name                                       \
  {                                                      \
    static const char *getName() { return "Cmd" #name; } \
    inline void execute(ExecutionContext &ctx) const;

#define VULKAN_END_CONTEXT_COMMAND \
  }                                \
  ;

#define VULKAN_CONTEXT_COMMAND_PARAM(type, name)             type name;
#define VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) type name[size];
#include "device_context_cmd.inc"
#undef VULKAN_BEGIN_CONTEXT_COMMAND
#undef VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT
#undef VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE
#undef VULKAN_END_CONTEXT_COMMAND
#undef VULKAN_CONTEXT_COMMAND_PARAM
#undef VULKAN_CONTEXT_COMMAND_PARAM_ARRAY

typedef VariantVector<
#define VULKAN_BEGIN_CONTEXT_COMMAND(name)            Cmd##name,
#define VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(name)    Cmd##name,
#define VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(name) Cmd##name,
#define VULKAN_END_CONTEXT_COMMAND
#define VULKAN_CONTEXT_COMMAND_PARAM(type, name)
#define VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.inc"
#undef VULKAN_BEGIN_CONTEXT_COMMAND
#undef VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT
#undef VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE
#undef VULKAN_END_CONTEXT_COMMAND
#undef VULKAN_CONTEXT_COMMAND_PARAM
#undef VULKAN_CONTEXT_COMMAND_PARAM_ARRAY
  void>
  AnyCommandStore;

} // namespace drv3d_vulkan
