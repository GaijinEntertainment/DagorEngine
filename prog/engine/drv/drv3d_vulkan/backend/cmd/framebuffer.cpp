// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "framebuffer.h"
#include "backend/context.h"

using namespace drv3d_vulkan;

#define TRANSIT_WRITE(cmd_name, field, type)                                                         \
  TSPEC void BEContext::execCmd(const cmd_name &cmd)                                                 \
  {                                                                                                  \
    Backend::State::pipe.set_raw<field, type, FrontGraphicsState, FrontFramebufferState>(cmd.value); \
  }

TRANSIT_WRITE(CmdSetDepthStencilRWState, StateFieldFramebufferReadOnlyDepth, bool)
TRANSIT_WRITE(CmdSetFramebufferSwapchainSrgbWrite, StateFieldFramebufferSwapchainSrgbWrite, bool)
TRANSIT_WRITE(CmdSetFramebufferClearColor, StateFieldFramebufferClearColor, E3DCOLOR)
TRANSIT_WRITE(CmdSetFramebufferClearDepth, StateFieldFramebufferClearDepth, float)
TRANSIT_WRITE(CmdSetFramebufferClearStencil, StateFieldFramebufferClearStencil, uint8_t)

#undef TRANSIT_WRITE

TSPEC void BEContext::execCmd(const CmdAllowOpLoad &) { getFramebufferState().clearMode |= FramebufferState::CLEAR_LOAD; }

TSPEC void BEContext::execCmd(const CmdSetFramebufferAttachment &cmd)
{
  using Bind = StateFieldFramebufferAttachment;
  Bind bind{cmd.image, cmd.view};
  Backend::State::pipe.set_raw<StateFieldFramebufferAttachments, Bind::Indexed, FrontGraphicsState, FrontFramebufferState>(
    {cmd.index, bind});
}
