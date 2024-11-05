// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"

namespace drv3d_vulkan
{

// wrapper for subpass dependency with human readable "barriers"
class SubpassDep
{
  VkSubpassDependency &dep;

public:
  struct BarrierScope
  {
    VkPipelineStageFlags stage;
    VkAccessFlags access;
  };

  static BarrierScope colorW();
  static BarrierScope colorR();
  static BarrierScope colorShaderR();
  static BarrierScope depthW();
  static BarrierScope depthR();
  static BarrierScope depthShaderR();

  SubpassDep(VkSubpassDependency &target);

  void addSrc(BarrierScope v);
  void addDst(BarrierScope v);
  void addPaired(BarrierScope v);
  void selfDep(uint32_t pass);
};

} // namespace drv3d_vulkan
