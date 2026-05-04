// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "renderpass.h"
#include "backend/context.h"
#include "util/fault_report.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdSetRenderPassTarget &cmd)
{
  using Bind = StateFieldRenderPassTarget;

  ResourceClearValue rcv;
  rcv.asUint[0] = cmd.clearValueArr[0];
  rcv.asUint[1] = cmd.clearValueArr[1];
  rcv.asUint[2] = cmd.clearValueArr[2];
  rcv.asUint[3] = cmd.clearValueArr[3];

  Bind bind{cmd.image, cmd.mipLevel, cmd.layer, rcv};
  Backend::State::pipe.set_raw<StateFieldRenderPassTargets, Bind::Indexed, FrontGraphicsState, FrontRenderPassState>(
    {cmd.index, bind});
}

#define TRANSIT_WRITE(cmd_name, field, type)                                                        \
  TSPEC void BEContext::execCmd(const cmd_name &cmd)                                                \
  {                                                                                                 \
    Backend::State::pipe.set_raw<field, type, FrontGraphicsState, FrontRenderPassState>(cmd.value); \
  }

TRANSIT_WRITE(CmdSetRenderPassResource, StateFieldRenderPassResource, RenderPassResource *);
TRANSIT_WRITE(CmdSetRenderPassIndex, StateFieldRenderPassIndex, uint32_t);
TRANSIT_WRITE(CmdSetRenderPassSubpassIdx, StateFieldRenderPassSubpassIdx, uint32_t);
TRANSIT_WRITE(CmdSetRenderPassArea, StateFieldRenderPassArea, RenderPassArea);

#undef TRANSIT_WRITE

TSPEC void BEContext::execCmd(const CmdNativeRenderPassChanged &)
{
  // try to skip RP if there is no draws
  uint32_t index = Backend::State::pipe.getRO<StateFieldRenderPassIndex, uint32_t, FrontGraphicsState, FrontRenderPassState>();
  uint32_t nextSubpass =
    Backend::State::pipe.getRO<StateFieldRenderPassSubpassIdx, uint32_t, FrontGraphicsState, FrontRenderPassState>();

  if (nextSubpass == 0)
    processBufferCopyReorders(index);

  if (data->nativeRPDrawCounter[index] == 0)
  {
    RenderPassResource *fRP =
      Backend::State::pipe.getRO<StateFieldRenderPassResource, RenderPassResource *, FrontGraphicsState, FrontRenderPassState>();
    RenderPassResource *bRP = Backend::State::exec.getRO<StateFieldRenderPassResource, RenderPassResource *, BackGraphicsState>();
    // pass end of skipped pass
    if (!fRP && !bRP)
      return;
    // check that RP is no-op without draws
    if (fRP && fRP->isNoOpWithoutDraws())
      return;
  }

  if (nextSubpass == StateFieldRenderPassSubpassIdx::InvalidSubpass)
    // transit into custom stage to avoid starting FB pass right after native one
    beginCustomStage("endNativeRP");
  else
  {
    ensureActivePass();
    Backend::State::exec.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
    applyStateChanges();
  }
  invalidateActiveGraphicsPipeline();
}

TSPEC void FaultReportDump::dumpCmd(const CmdSetRenderPassTarget &v)
{
  VK_CMD_DUMP_PARAM(index);
  VK_CMD_DUMP_PARAM_PTR(image);
}

TSPEC void FaultReportDump::dumpCmd(const CmdSetRenderPassResource &v) { VK_CMD_DUMP_PARAM_PTR(value); }
TSPEC void FaultReportDump::dumpCmd(const CmdSetRenderPassSubpassIdx &v) { VK_CMD_DUMP_PARAM(value); }
