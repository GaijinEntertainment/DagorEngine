// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "draw_dispatch.h"
#include "backend/context.h"
#include "util/fault_report.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdDispatch &cmd)
{
  startNode(ExecutionNode::CS);
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
  flushComputeState();

  VULKAN_LOG_CALL(Backend::cb.wCmdDispatch(cmd.x, cmd.y, cmd.z));
}

TSPEC void FaultReportDump::dumpCmd(const CmdDispatch &v)
{
  RefId rid = addTagged(GlobalTag::TAG_CMD_PARAM, uint64_t(cmdPtr), String(64, "threads(%u,%u,%u)", v.x, v.y, v.z));
  addRef(rid, GlobalTag::TAG_CMD, currentCommand);
  addRef(significants.programCS, GlobalTag::TAG_CMD, currentCommand);
}

TSPEC void BEContext::execCmd(const CmdDispatchIndirect &cmd)
{
  startNode(ExecutionNode::CS);
  Backend::State::exec.set<StateFieldActiveExecutionStage>(ActiveExecutionStage::COMPUTE);
  trackIndirectArgAccesses(cmd.buffer, cmd.offset, 1, sizeof(VkDispatchIndirectCommand));
  flushComputeState();

  VULKAN_LOG_CALL(Backend::cb.wCmdDispatchIndirect(cmd.buffer.getHandle(), cmd.buffer.bufOffset(cmd.offset)));
}

TSPEC void FaultReportDump::dumpCmd(const CmdDispatchIndirect &v)
{
  VK_CMD_DUMP_PARAM(buffer);
  VK_CMD_DUMP_PARAM(offset);
  addRef(significants.programCS, GlobalTag::TAG_CMD, currentCommand);
}

TSPEC void BEContext::execCmd(const CmdClearView &cmd)
{
  int what = cmd.clearFlags;
  G_ASSERTF((what & FramebufferState::CLEAR_LOAD) == 0,
    "vulkan: used special clear bit outside of driver internal code, need to adjust internal special clear bit");

  startNode(ExecutionNode::RP);
  ensureActivePass();

  // when async pipelines enabled and we trying to discard target, use clear instead
  // this avoids leaking some garbadge when pipelines are not ready
  if (Globals::pipelines.asyncCompileEnabledGR() && (what & CLEAR_DISCARD) != 0)
  {
    if (what & CLEAR_DISCARD_TARGET)
      what |= CLEAR_TARGET;
    if (what & CLEAR_DISCARD_ZBUFFER)
      what |= CLEAR_ZBUFFER;
    if (what & CLEAR_DISCARD_STENCIL)
      what |= CLEAR_STENCIL;
    what &= ~CLEAR_DISCARD;
  }

  getFramebufferState().clearMode |= what;
  Backend::State::exec.interruptRenderPass("ClearView");
  Backend::State::exec.set<StateFieldGraphicsFlush, uint32_t, BackGraphicsState>(0);
  applyStateChanges();
  invalidateActiveGraphicsPipeline();

  getFramebufferState().clearMode = 0;
}

TSPEC void BEContext::execCmd(const CmdDrawIndirect &cmd)
{
  startNode(ExecutionNode::PS);
  ensureActivePass();
  flushGrahpicsState(cmd.top);
  if (!renderAllowed)
  {
    insertEvent("drawIndirect: async compile", 0xFF0000FF);
    return;
  }
  trackIndirectArgAccesses(cmd.buffer, cmd.offset, cmd.count, cmd.stride);

  if (cmd.count > 1 && !Globals::cfg.has.multiDrawIndirect)
  {
    // emulate multi draw indirect
    uint32_t offset = cmd.offset;
    for (uint32_t i = 0; i < cmd.count; ++i)
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndirect(cmd.buffer.getHandle(), cmd.buffer.bufOffset(offset), 1, cmd.stride));
      offset += cmd.stride;
    }
  }
  else
  {
    VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndirect(cmd.buffer.getHandle(), cmd.buffer.bufOffset(cmd.offset), cmd.count, cmd.stride));
  }
}

TSPEC void FaultReportDump::dumpCmd(const CmdDrawIndirect &v)
{
  VK_CMD_DUMP_PARAM(top);
  VK_CMD_DUMP_PARAM(count);
  VK_CMD_DUMP_PARAM(buffer);
  VK_CMD_DUMP_PARAM(offset);
  VK_CMD_DUMP_PARAM(stride);
  addRef(significants.programGR, GlobalTag::TAG_CMD, currentCommand);
}

TSPEC void BEContext::execCmd(const CmdDrawIndexedIndirect &cmd)
{
  startNode(ExecutionNode::PS);
  ensureActivePass();
  flushGrahpicsState(cmd.top);
  if (!renderAllowed)
  {
    insertEvent("drawIndexedIndirect: async compile", 0xFF0000FF);
    return;
  }
  trackIndirectArgAccesses(cmd.buffer, cmd.offset, cmd.count, cmd.stride);

  if (cmd.count > 1 && !Globals::cfg.has.multiDrawIndirect)
  {
    // emulate multi draw indirect
    uint32_t offset = cmd.offset;
    for (uint32_t i = 0; i < cmd.count; ++i)
    {
      VULKAN_LOG_CALL(Backend::cb.wCmdDrawIndexedIndirect(cmd.buffer.getHandle(), cmd.buffer.bufOffset(offset), 1, cmd.stride));
      offset += cmd.stride;
    }
  }
  else
  {
    VULKAN_LOG_CALL(
      Backend::cb.wCmdDrawIndexedIndirect(cmd.buffer.getHandle(), cmd.buffer.bufOffset(cmd.offset), cmd.count, cmd.stride));
  }
}

TSPEC void FaultReportDump::dumpCmd(const CmdDrawIndexedIndirect &v)
{
  VK_CMD_DUMP_PARAM(top);
  VK_CMD_DUMP_PARAM(count);
  VK_CMD_DUMP_PARAM(buffer);
  VK_CMD_DUMP_PARAM(offset);
  VK_CMD_DUMP_PARAM(stride);
  addRef(significants.programGR, GlobalTag::TAG_CMD, currentCommand);
};

TSPEC void BEContext::execCmd(const CmdDraw &cmd)
{
  startNode(ExecutionNode::PS);
  ensureActivePass();
  flushGrahpicsState(cmd.top);
  draw(cmd.count, cmd.instanceCount, cmd.start, cmd.firstInstance);
}

TSPEC void FaultReportDump::dumpCmd(const CmdDraw &v)
{
  VK_CMD_DUMP_PARAM(top);
  RefId rid = addTagged(GlobalTag::TAG_CMD_PARAM, uint64_t(cmdPtr) + ((uintptr_t)&v.start - (uintptr_t)&v),
    String(64, "start %u count %u firstInst %u instances %u", v.start, v.count, v.firstInstance, v.instanceCount));
  addRef(rid, GlobalTag::TAG_CMD, currentCommand);
  addRef(significants.programGR, GlobalTag::TAG_CMD, currentCommand);
};

TSPEC void BEContext::execCmd(const CmdDrawIndexed &cmd)
{
  startNode(ExecutionNode::PS);
  ensureActivePass();
  flushGrahpicsState(cmd.top);
  drawIndexed(cmd.count, cmd.instanceCount, cmd.indexStart, cmd.vertexBase, cmd.firstInstance);
}

TSPEC void FaultReportDump::dumpCmd(const CmdDrawIndexed &v)
{
  VK_CMD_DUMP_PARAM(top);
  RefId rid = addTagged(GlobalTag::TAG_CMD_PARAM, uint64_t(cmdPtr) + ((uintptr_t)&v.indexStart - (uintptr_t)&v),
    String(64, "indexStart %u count %u vertexBase %u firstInst %u instances %u", v.indexStart, v.count, v.vertexBase, v.firstInstance,
      v.instanceCount));
  addRef(rid, GlobalTag::TAG_CMD, currentCommand);
  addRef(significants.programGR, GlobalTag::TAG_CMD, currentCommand);
};
