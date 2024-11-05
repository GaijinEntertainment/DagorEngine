// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "subpass_dependency.h"

using namespace drv3d_vulkan;

SubpassDep::BarrierScope SubpassDep::colorW()
{
  return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
}

SubpassDep::BarrierScope SubpassDep::colorR()
{
  return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT};
}

SubpassDep::BarrierScope SubpassDep::colorShaderR()
{
  return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT};
}

SubpassDep::BarrierScope SubpassDep::depthW()
{
  return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
}

SubpassDep::BarrierScope SubpassDep::depthR()
{
  return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT};
}

SubpassDep::BarrierScope SubpassDep::depthShaderR() { return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT}; }

SubpassDep::SubpassDep(VkSubpassDependency &target) : dep(target)
{
  dep.dstAccessMask = 0;
  dep.srcAccessMask = 0;
  dep.dstStageMask = 0;
  dep.srcStageMask = 0;
  dep.dependencyFlags = 0;
}

void SubpassDep::selfDep(uint32_t pass)
{
  dep.srcSubpass = dep.dstSubpass = pass;
  dep.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
}

void SubpassDep::addSrc(BarrierScope v)
{
  dep.srcAccessMask |= v.access;
  dep.srcStageMask |= v.stage;
}

void SubpassDep::addDst(BarrierScope v)
{
  dep.dstAccessMask |= v.access;
  dep.dstStageMask |= v.stage;
}

void SubpassDep::addPaired(BarrierScope v)
{
  addSrc(v);
  addDst(v);
}
