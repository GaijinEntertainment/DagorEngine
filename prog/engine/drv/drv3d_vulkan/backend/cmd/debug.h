// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "shader_module.h"

namespace drv3d_vulkan
{

struct CmdPushEvent
{
  StringIndexRef index;
};

struct CmdPopEvent
{};

struct CmdUpdateDebugUIPipelinesData
{};

#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
struct CmdSetPipelineUsability
{
  ProgramID id;
  bool value;
};
#endif // VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT

struct CmdPlaceAftermathMarker
{
  StringIndexRef stringIndex;
};

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
struct CmdAttachComputeProgramDebugInfo
{
  ProgramID program;
  ShaderDebugInfo *dbg;
};
#endif

struct CmdWriteDebugMessage
{
  StringIndexRef messageIndex;
  int severity;
};

} // namespace drv3d_vulkan
