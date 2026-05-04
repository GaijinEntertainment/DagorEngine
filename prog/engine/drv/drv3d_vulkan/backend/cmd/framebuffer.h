// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include <math/dag_e3dColor.h>
#include "image_view_state.h"
#include "image_resource.h"

namespace drv3d_vulkan
{

struct CmdSetDepthStencilRWState
{
  bool value;
};

struct CmdSetFramebufferSwapchainSrgbWrite
{
  bool value;
};

struct CmdSetFramebufferClearColor
{
  E3DCOLOR value;
};

struct CmdSetFramebufferClearDepth
{
  float value;
};

struct CmdSetFramebufferClearStencil
{
  uint8_t value;
};

struct CmdAllowOpLoad
{};

struct CmdSetFramebufferAttachment
{
  uint32_t index;
  Image *image;
  ImageViewState view;
};

} // namespace drv3d_vulkan
