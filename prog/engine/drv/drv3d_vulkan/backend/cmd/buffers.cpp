// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "buffers.h"
#include "backend/context.h"

using namespace drv3d_vulkan;

TSPEC void BEContext::execCmd(const CmdNotifyBufferDiscard &cmd)
{
  scratch.delayedDiscards.push_back({cmd.oldBuf, cmd.newBuf, cmd.bufFlags});
}

TSPEC void BEContext::execCmd(const CmdAddRefFrameMem &cmd) { cmd.buf->addFrameMemRef(); }
TSPEC void BEContext::execCmd(const CmdReleaseFrameMem &cmd) { cmd.buf->removeFrameMemRef(); }
