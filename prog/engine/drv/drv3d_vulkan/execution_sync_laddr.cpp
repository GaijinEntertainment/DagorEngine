#include "execution_sync.h"

using namespace drv3d_vulkan;

bool ExecutionSyncTracker::LogicAddress::isNonConflictingUAVAccess(const LogicAddress &cmp) const
{
  bool bothWrite = isWrite() && cmp.isWrite();
  bool bothRead = isRead() && cmp.isRead();
  bool bothRW = bothRead && bothWrite;
  if (bothRW)
  {
    bool isRenderingStage = (cmp.stage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) || (stage == VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT) ||
                            (cmp.stage == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT) || (stage == VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    return cmp.stage == stage && cmp.access == access && isRenderingStage;
  }
  return false;
}

bool ExecutionSyncTracker::LogicAddress::conflicting(const LogicAddress &cmp) const
{
  // if we have W op with following R ops on different possible async stages (CS/transfer/RP),
  // W-R sync to last encountered stage (in ops order) will be lost, causing this async stage to executed without waiting for W
  // so treat any ops between this async stages as conflict
  bool asyncStages = isComputeOrTransfer() != cmp.isComputeOrTransfer();
  // W-R, W-W, R-W
  bool anyWrite = isWrite() || cmp.isWrite();
  bool bothWrite = isWrite() && cmp.isWrite();
  bool anyRead = isRead() || cmp.isRead();
  return bothWrite || (anyWrite && anyRead) || asyncStages;
}

bool ExecutionSyncTracker::LogicAddress::mergeConflicting(const LogicAddress &cmp) const
{
  // merge is ok at logic address if
  //  -both is read
  //  -both is RW at same stage & access
  bool bothWrite = isWrite() && cmp.isWrite();
  bool bothRead = isRead() && cmp.isRead();
  bool bothRW = bothRead && bothWrite;
  bool anyWrite = isWrite() || cmp.isWrite();
  if (bothRW)
    return cmp.stage != stage || cmp.access != access;
  else if (bothRead && !anyWrite)
    return false;
  else
    return true;
}

bool ExecutionSyncTracker::LogicAddress::isComputeOrTransfer() const
{
  return stage & (VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT);
}

bool ExecutionSyncTracker::LogicAddress::isWrite() const
{
  constexpr VkAccessFlags writeMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                      VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT |
                                      VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_MEMORY_WRITE_BIT |
                                      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  return (access & writeMask) != 0;
}

bool ExecutionSyncTracker::LogicAddress::isRead() const
{
  constexpr VkAccessFlags readMask =
    VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT |
    VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_HOST_READ_BIT | VK_ACCESS_MEMORY_READ_BIT |
    VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
  return (access & readMask) != 0;
}

static VkPipelineStageFlags stageForResourceAtExecStage[ShaderStage::STAGE_MAX_EXT] = {
  VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // CS
  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // PS
  VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,   // VS
#if D3D_HAS_RAY_TRACING
  VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR // RAYTRACE
#else
  VK_PIPELINE_STAGE_NONE // STUB
#endif
};

static VkAccessFlags accessForBufferAtRegisterType[RegisterType::COUNT] = {
  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT, // T
  VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, // U
  VK_ACCESS_UNIFORM_READ_BIT                              // B
};

ExecutionSyncTracker::LogicAddress ExecutionSyncTracker::LogicAddress::forBufferOnExecStage(ShaderStage stage, RegisterType reg_type)
{
  G_ASSERTF(stage < ShaderStage::STAGE_MAX_EXT, "vulkan: no info on buffer ladr for stage %s(%u)", formatShaderStage(stage), stage);
  G_ASSERTF(reg_type < RegisterType::COUNT, "vulkan: unknown register type %u", reg_type);
  return {stageForResourceAtExecStage[stage], accessForBufferAtRegisterType[reg_type]};
}

static VkAccessFlags readOnlyDepthOptionalAccessBits = VK_ACCESS_NONE;

void ExecutionSyncTracker::LogicAddress::setAttachmentNoStoreSupport(bool supported)
{
  readOnlyDepthOptionalAccessBits = supported ? VK_ACCESS_NONE : VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
}

ExecutionSyncTracker::LogicAddress ExecutionSyncTracker::LogicAddress::forAttachmentWithLayout(VkImageLayout layout)
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

ExecutionSyncTracker::LogicAddress ExecutionSyncTracker::LogicAddress::forImageOnExecStage(ShaderStage stage, RegisterType reg_type)
{
  G_ASSERTF(stage < ShaderStage::STAGE_MAX_EXT, "vulkan: no info on image ladr for stage %s(%u)", formatShaderStage(stage), stage);
  G_ASSERTF(reg_type < RegisterType::COUNT, "vulkan: unknown register type %u", reg_type);
  return {stageForResourceAtExecStage[stage], accessForImageAtRegisterType[reg_type]};
}

ExecutionSyncTracker::LogicAddress ExecutionSyncTracker::LogicAddress::forImageBindlessRead()
{
  return {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
    VK_ACCESS_SHADER_READ_BIT};
}

static VkAccessFlags accessForAccelerationStructureAtRegisterType[RegisterType::COUNT] = {
  VK_ACCESS_SHADER_READ_BIT, // T
  VK_ACCESS_NONE,            // U
  VK_ACCESS_NONE             // B
};

ExecutionSyncTracker::LogicAddress ExecutionSyncTracker::LogicAddress::forAccelerationStructureOnExecStage(ShaderStage stage,
  RegisterType reg_type)
{
  G_ASSERTF(stage < ShaderStage::STAGE_MAX_EXT, "vulkan: no info on as ladr for stage %s(%u)", formatShaderStage(stage), stage);
  G_ASSERTF(reg_type < RegisterType::COUNT, "vulkan: unknown register type %u", reg_type);
  return {stageForResourceAtExecStage[stage], accessForAccelerationStructureAtRegisterType[reg_type]};
}

ExecutionSyncTracker::LogicAddress ExecutionSyncTracker::LogicAddress::forBLASIndirectReads()
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
