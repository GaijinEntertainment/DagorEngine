// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "debug.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "stacked_profile_events.h"
#include "debug_ui.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdPushEvent &cmd)
{
  Backend::profilerStack.pushInterruptChain(data->charStore.data() + cmd.index.get());
  pushEventTracked(data->charStore.data() + cmd.index.get());
}

TSPEC void BEContext::execCmd(const CmdPopEvent &)
{
  Backend::profilerStack.popInterruptChain();
  popEventTracked();
}

TSPEC void BEContext::execCmd(const CmdUpdateDebugUIPipelinesData &) { debug_ui_update_pipelines_data(); }

#if VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT
TSPEC void BEContext::execCmd(const CmdSetPipelineUsability &cmd)
{
  const bool ret = Globals::pipelines.visit(cmd.id, [valueCopy = cmd.value](auto &pipeline) { pipeline.blockUsage(!valueCopy); });
  G_ASSERTF(ret, "Tried to block an invalid pipeline!");
}
#endif // VULKAN_ENABLE_DEBUG_FLUSHING_SUPPORT

TSPEC void BEContext::execCmd(const CmdPlaceAftermathMarker &cmd)
{
#if VK_EXT_debug_utils
  if (Globals::VK::inst.hasExtension<DebugUtilsEXT>())
  {
    VkDebugUtilsLabelEXT dul = {VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT};
    dul.pLabelName = data->charStore.data() + cmd.stringIndex.get();
    dul.color[0] = 1.f;
    dul.color[1] = 1.f;
    dul.color[2] = 1.f;
    dul.color[3] = 1.f;
    VULKAN_LOG_CALL(Backend::cb.wCmdInsertDebugUtilsLabelEXT(&dul));
  }
#endif
}

#if VULKAN_LOAD_SHADER_EXTENDED_DEBUG_DATA
TSPEC void BEContext::execCmd(const CmdAttachComputeProgramDebugInfo &cmd)
{
  Globals::pipelines.get<ComputePipeline>(cmd.program).setDebugInfo({{*cmd.dbg}});
  delete cmd.dbg;
}
#endif

TSPEC void BEContext::execCmd(const CmdWriteDebugMessage &cmd)
{
  if (cmd.severity < 1)
    debug("%s", data->charStore.data() + cmd.messageIndex.get());
  else if (cmd.severity < 2)
    logwarn("%s", data->charStore.data() + cmd.messageIndex.get());
  else
    D3D_ERROR("%s", data->charStore.data() + cmd.messageIndex.get());
}
