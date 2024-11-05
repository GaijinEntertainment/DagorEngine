// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_sync.h"
#include "vk_to_string.h"

using namespace drv3d_vulkan;

bool LogicAddress::isNonConflictingUAVAccess(const LogicAddress &cmp) const
{
  bool bothWrite = isWrite() && cmp.isWrite();
  bool bothRead = isRead() && cmp.isRead();
  bool bothRW = bothRead && bothWrite;
  if (bothRW)
  {
    bool isRenderingStage = (stage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) || (stage == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    bool cmpIsRenderingStage =
      (cmp.stage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) || (cmp.stage == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    return cmp.access == access && (isRenderingStage && cmpIsRenderingStage);
  }
  return false;
}

bool LogicAddress::conflictingStageless(const LogicAddress &cmp) const
{
  // W-R, W-W, R-W
  bool anyWrite = isWrite() || cmp.isWrite();
  bool bothWrite = isWrite() && cmp.isWrite();
  bool anyRead = isRead() || cmp.isRead();
  // special cases: special aliasing/present barriers with access none
  bool anySpecial = (access == VK_ACCESS_NONE) || (cmp.access == VK_ACCESS_NONE);
  return bothWrite || (anyWrite && anyRead) || anySpecial;
}


bool LogicAddress::conflicting(const LogicAddress &cmp) const
{
  // if we have W op with following R ops on different possible async stages (CS/transfer/RP),
  // W-R sync to last encountered stage (in ops order) will be lost, causing this async stage to executed without waiting for W
  // so treat any ops between this async stages as conflict
  // FIXME: improve this somehow!
  bool asyncStages = asyncTimelineBits() != cmp.asyncTimelineBits();
  return conflictingStageless(cmp) || asyncStages;
}

bool LogicAddress::mergeConflicting(const LogicAddress &cmp) const
{
  // merge is ok at logic address if
  //  -both is read
  //  -both is RW at same stage & access
  //  -both is special barriers, that should be merged
  bool bothWrite = isWrite() && cmp.isWrite();
  bool bothRead = isRead() && cmp.isRead();
  bool bothRW = bothRead && bothWrite;
  bool anyWrite = isWrite() || cmp.isWrite();
  bool bothSpecial = (access == VK_ACCESS_NONE) && (cmp.access == VK_ACCESS_NONE);

  if (bothRW)
    return cmp.stage != stage || cmp.access != access;
  else if ((bothRead && !anyWrite) || bothSpecial)
    return false;
  else
    return true;
}

VkPipelineStageFlags LogicAddress::asyncTimelineBits() const
{
  // all stages are probably async
  return stage;
}

bool LogicAddress::isWrite() const
{
  constexpr VkAccessFlags writeMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT |
                                      VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT |
                                      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  return (access & writeMask) != 0;
}

bool LogicAddress::isRead() const
{
  constexpr VkAccessFlags readMask =
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
  return (access & readMask) != 0;
}

static VkPipelineStageFlags stageForResourceAtExecStage[(uint32_t)ExtendedShaderStage::MAX] = {
  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // CS
  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // PS
  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,   // VS
#if D3D_HAS_RAY_TRACING
  VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, // RT
#else
  VK_PIPELINE_STAGE_NONE, // RT-stub
#endif
  VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,    // TC
  VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, // TE
  VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT                 // GS
};

static VkAccessFlags accessForBufferAtRegisterType[RegisterType::COUNT] = {
  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT, // T
  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // U
  VK_ACCESS_UNIFORM_READ_BIT                              // B
};

LogicAddress LogicAddress::forBufferOnExecStage(ExtendedShaderStage stage, RegisterType reg_type)
{
  G_ASSERTF(stage < ExtendedShaderStage::MAX, "vulkan: no info on buffer ladr for stage %s(%u)", formatExtendedShaderStage(stage),
    (uint32_t)stage);
  G_ASSERTF(reg_type < RegisterType::COUNT, "vulkan: unknown register type %u", reg_type);
  return {stageForResourceAtExecStage[(uint32_t)stage], accessForBufferAtRegisterType[reg_type]};
}

static VkAccessFlags readOnlyDepthOptionalAccessBits = VK_ACCESS_NONE;

void LogicAddress::setAttachmentNoStoreSupport(bool supported)
{
  readOnlyDepthOptionalAccessBits = supported ? VK_ACCESS_NONE : VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
}

LogicAddress LogicAddress::forAttachmentWithLayout(VkImageLayout layout)
{
  switch (layout)
  {
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | readOnlyDepthOptionalAccessBits};
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
    // for input attachments
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_INPUT_ATTACHMENT_READ_BIT};
    default: G_ASSERTF(0, "vulkan: wrong layout for render pass attachment %u:%s", layout, formatImageLayout(layout));
  }
  return {VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE};
}

static VkAccessFlags accessForImageAtRegisterType[RegisterType::COUNT] = {
  VK_ACCESS_SHADER_READ_BIT,                              // T
  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // U
  VK_ACCESS_NONE                                          // B
};

LogicAddress LogicAddress::forImageOnExecStage(ExtendedShaderStage stage, RegisterType reg_type)
{
  G_ASSERTF(stage < ExtendedShaderStage::MAX, "vulkan: no info on image ladr for stage %s(%u)", formatExtendedShaderStage(stage),
    (uint32_t)stage);
  G_ASSERTF(reg_type < RegisterType::COUNT, "vulkan: unknown register type %u", reg_type);
  return {stageForResourceAtExecStage[(uint32_t)stage], accessForImageAtRegisterType[reg_type]};
}

LogicAddress LogicAddress::forImageBindlessRead()
{
  return {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    VK_ACCESS_SHADER_READ_BIT};
}

static VkAccessFlags accessForAccelerationStructureAtRegisterType[RegisterType::COUNT] = {
  VK_ACCESS_SHADER_READ_BIT, // T
  VK_ACCESS_NONE,            // U
  VK_ACCESS_NONE             // B
};

LogicAddress LogicAddress::forAccelerationStructureOnExecStage(ExtendedShaderStage stage, RegisterType reg_type)
{
  G_ASSERTF(stage < ExtendedShaderStage::MAX, "vulkan: no info on as ladr for stage %s(%u)", formatExtendedShaderStage(stage),
    (uint32_t)stage);
  G_ASSERTF(reg_type < RegisterType::COUNT, "vulkan: unknown register type %u", reg_type);
  return {stageForResourceAtExecStage[(uint32_t)stage], accessForAccelerationStructureAtRegisterType[reg_type]};
}

LogicAddress LogicAddress::forBLASIndirectReads()
{
  return
  {
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
#if D3D_HAS_RAY_TRACING
    | VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR
#endif
      ,
      VK_ACCESS_SHADER_READ_BIT
  };
}
