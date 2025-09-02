// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// logic vulkan execution/pipeline address
// basically utility/wrapper to work with pair of stage and access

#include "driver.h"

namespace drv3d_vulkan
{

struct LogicAddress
{
  VkPipelineStageFlags stage;
  VkAccessFlags access;

  bool isNonConflictingUAVAccess(const LogicAddress &cmp) const;
  bool conflictingStageless(const LogicAddress &cmp) const;
  bool conflicting(const LogicAddress &cmp) const;
  bool mergeConflicting(const LogicAddress &cmp) const;
  bool isRead() const;
  bool isWrite() const;
  VkPipelineStageFlags asyncTimelineBits() const;
  void merge(const LogicAddress &v)
  {
    stage |= v.stage;
    access |= v.access;
  }
  bool equal(const LogicAddress &v) { return stage == v.stage && access == v.access; }
  void clear()
  {
    stage = VK_PIPELINE_STAGE_NONE;
    access = VK_ACCESS_NONE;
  }

  static LogicAddress forBufferOnExecStage(ExtendedShaderStage stage, RegisterType reg_type);
  static LogicAddress forAttachmentWithLayout(VkImageLayout layout);
  static LogicAddress forImageOnExecStage(ExtendedShaderStage stage, RegisterType reg_type);
  static LogicAddress forImageBindlessRead();
  static LogicAddress forAccelerationStructureOnExecStage(ExtendedShaderStage stage, RegisterType reg_type);
  static LogicAddress forBLASIndirectReads();
  static void setAttachmentNoStoreSupport(bool supported);
};

} // namespace drv3d_vulkan
