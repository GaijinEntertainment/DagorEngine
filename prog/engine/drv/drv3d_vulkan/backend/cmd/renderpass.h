// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <util/dag_stdint.h>
#include "render_pass_resource.h"
#include "image_resource.h"

namespace drv3d_vulkan
{

struct CmdSetRenderPassTarget
{
  uint32_t index;
  Image *image;
  uint32_t mipLevel;
  uint32_t layer;
  uint32_t clearValueArr[4];
};

struct CmdSetRenderPassResource
{
  RenderPassResource *value;
};

struct CmdSetRenderPassIndex
{
  uint32_t value;
};

struct CmdSetRenderPassSubpassIdx
{
  uint32_t value;
};

struct CmdSetRenderPassArea
{
  RenderPassArea value;
};

struct CmdNativeRenderPassChanged
{};

} // namespace drv3d_vulkan
