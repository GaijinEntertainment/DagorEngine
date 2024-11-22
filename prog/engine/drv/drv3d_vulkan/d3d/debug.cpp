// Copyright (C) Gaijin Games KFT.  All rights reserved.

// debug features implementation
#include <drv/3d/dag_driver.h>
#include "globals.h"
#include "global_lock.h"
#include "device_context.h"
#include "debug_ui.h"

using namespace drv3d_vulkan;

void d3d::beginEvent(const char *name)
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

#if VK_EXT_debug_marker || VK_EXT_debug_utils
  Globals::ctx.pushEvent(name);
#else
  (void)name;
#endif

  d3dhang::hang_if_requested(name);
}

void d3d::endEvent()
{
  VERIFY_GLOBAL_LOCK_ACQUIRED();

#if VK_EXT_debug_marker || VK_EXT_debug_utils
  CmdPopEvent cmd;
  Globals::ctx.dispatchCommand(cmd);
#endif
}

#if DAGOR_DBGLEVEL > 0
#include <gui/dag_imgui.h>
REGISTER_IMGUI_WINDOW("VULK", "Memory##VULK-memory", drv3d_vulkan::debug_ui_memory);
REGISTER_IMGUI_WINDOW("VULK", "Frame##VULK-frame", drv3d_vulkan::debug_ui_frame);
REGISTER_IMGUI_WINDOW("VULK", "Timeline##VULK-timeline", drv3d_vulkan::debug_ui_timeline);
REGISTER_IMGUI_WINDOW("VULK", "Resources##VULK-resources", drv3d_vulkan::debug_ui_resources);
REGISTER_IMGUI_WINDOW("VULK", "Pipelines##VULK-pipelines", drv3d_vulkan::debug_ui_pipelines);
REGISTER_IMGUI_WINDOW("VULK", "Sync##VULK-sync", drv3d_vulkan::debug_ui_sync);
REGISTER_IMGUI_WINDOW("VULK", "Misc##VULK-misc-debug-features", drv3d_vulkan::debug_ui_misc);
#endif