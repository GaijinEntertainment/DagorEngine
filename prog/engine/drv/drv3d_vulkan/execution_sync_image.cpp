#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "image_resource.h"

using namespace drv3d_vulkan;

String ExecutionSyncTracker::ImageOp::format() const
{
  return String(128, "syncImgOp: %p [mip:%u-%u arr:%u-%u] %s %s %s %s \n internal caller: %s", obj, area.mipIndex,
    area.mipIndex + area.mipRange, area.arrayIndex, area.arrayIndex + area.arrayRange, formatImageLayout(layout),
    formatPipelineStageFlags(laddr.stage), formatMemoryAccessFlags(laddr.access), obj->getDebugName(), caller.getInternal());
}

bool ExecutionSyncTracker::ImageOp::verifySelfConflict(const ImageOp &cmp) const
{
  bool ret = !conflicts(cmp);

  // UAV access exclusion
  if (laddr.isNonConflictingUAVAccess(cmp.laddr))
    return true;

  // special case for RO depth without no store ops
  if (!ret)
  {
    // both RO layouts
    if (layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL && cmp.layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    {
      // shader vs output stages
      const VkPipelineStageFlags fragTestStages =
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
      const VkPipelineStageFlags gfxShaderStages = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
      if (((cmp.laddr.stage == fragTestStages) && ((laddr.stage & gfxShaderStages) != 0)) ||
          ((laddr.stage == fragTestStages) && ((cmp.laddr.stage & gfxShaderStages) != 0)))
      {
        // read vs DS read at least
        if (((cmp.laddr.access == VK_ACCESS_SHADER_READ_BIT) && ((laddr.access & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT) != 0)) ||
            ((laddr.access == VK_ACCESS_SHADER_READ_BIT) && ((cmp.laddr.access & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT) != 0)))
        {
          // means we compare shader depth read vs read only depth read & store at depth attachment output stage
          // when no store op is not available and that is no conflicting in fact
          return true;
        }
      }
    }
  }
  return ret;
}

bool ExecutionSyncTracker::ImageOp::conflicts(const ImageOp &cmp) const
{
  return area.intersects(cmp.area) && (laddr.conflicting(cmp.laddr) || layout != cmp.layout) && !completed && !cmp.completed;
}

void ExecutionSyncTracker::ImageOp::addToBarrierByTemplateSrc(PipelineBarrier &barrier)
{
  // memory barriers only
  if (changesLayout)
  {
    // clear change layout flag, as it dynamically computed at conflict check stage
    changesLayout = false;
    return;
  }
  barrier.modifyImageTemplate(obj);
  barrier.modifyImageTemplateNewLayout(layout);
  barrier.modifyImageTemplateOldLayout(layout);
  barrier.modifyImageTemplate(area.mipIndex, area.mipRange, area.arrayIndex, area.arrayRange);
  barrier.addImageByTemplate({laddr.access, VK_ACCESS_NONE});
}

void ExecutionSyncTracker::ImageOp::onNativePassEnter(uint16_t pass_idx)
{
  // operation will be handled by render pass subpass deps
  // so we should not treat it as pending operation right after sync/layout barrier is filled
  if (nativePassIdx == pass_idx)
    completed = true;
}

void ExecutionSyncTracker::ImageOp::addToBarrierByTemplateDst(PipelineBarrier &barrier, size_t gpu_work_id)
{
  // layout changing barriers only!
  if (!changesLayout)
    return;
  // clear change layout flag, as it dynamically computed at conflict check stage
  changesLayout = false;

  barrier.modifyImageTemplate(obj);
  barrier.modifyImageTemplateNewLayout(layout);

  bool splitted = false;
  VkImageLayout zeroLayout = obj->layout.get(area.mipIndex, area.arrayIndex);
  for (uint32_t mip = area.mipIndex; mip < area.mipIndex + area.mipRange; ++mip)
  {
    for (uint32_t array = area.arrayIndex; array < area.arrayIndex + area.arrayRange; ++array)
    {
      VkImageLayout subresLayout = obj->layout.get(mip, array);
      splitted |= subresLayout != zeroLayout;
      splitted |= subresLayout == layout;
      if (splitted)
        break;
    }
  }

  if (!splitted)
  {
    barrier.modifyImageTemplate(area.mipIndex, area.mipRange, area.arrayIndex, area.arrayRange);
    barrier.modifyImageTemplateOldLayout(zeroLayout);
    barrier.addImageByTemplate({VK_ACCESS_NONE, laddr.access});
  }
  else
  {
    for (uint32_t mip = area.mipIndex; mip < area.mipIndex + area.mipRange; ++mip)
    {
      for (uint32_t array = area.arrayIndex; array < area.arrayIndex + area.arrayRange; ++array)
      {
        VkImageLayout subresLayout = obj->layout.get(mip, array);
        if (subresLayout == layout)
          continue;
        barrier.modifyImageTemplate(mip, 1, array, 1);
        barrier.modifyImageTemplateOldLayout(subresLayout);
        barrier.addImageByTemplate({VK_ACCESS_NONE, laddr.access});
      }
    }
  }


  for (uint32_t mip = area.mipIndex; mip < area.mipIndex + area.mipRange; ++mip)
    for (uint32_t array = area.arrayIndex; array < area.arrayIndex + area.arrayRange; ++array)
    {
      obj->layout.set(mip, array, layout);
    }

  bool setsRoSeal = obj->requestedRoSeal(gpu_work_id) && !laddr.isWrite() && obj->layout.roSealTargetLayout == layout;
  if (setsRoSeal)
  {
    for (VkImageLayout i : obj->layout.data)
      if (i != obj->layout.roSealTargetLayout)
        return;
    obj->activateRoSeal();
  }
}

void ExecutionSyncTracker::ImageOp::modifyBarrierTemplate(PipelineBarrier &barrier, LogicAddress &src, LogicAddress &dst)
{
  barrier.modifyImageTemplate({src.access, dst.access});
}

void ExecutionSyncTracker::ImageOp::onConflictWithDst(ExecutionSyncTracker::ImageOp &dst, size_t)
{
  bool differentLayouts = dst.layout != layout;
  changesLayout |= differentLayouts;
  dst.changesLayout |= differentLayouts;
}

bool ExecutionSyncTracker::ImageOp::hasObjConflict()
{
  // obj conflict tracks layout change for object without src pending operation
  // so if we found layout change at src conflict checks, no need to search for layout change twice
  // check should be short circuited at callsite of this method, so put assert here instead
  G_ASSERTF(!changesLayout && !dstConflict,
    "vulkan: ImageOp::hasObjConflict called when layout change is [%s] and dst conflict is [%s]", changesLayout ? "yes" : "no",
    dstConflict ? "yes" : "no");

  for (uint32_t mip = area.mipIndex; mip < area.mipIndex + area.mipRange; ++mip)
    for (uint32_t array = area.arrayIndex; array < area.arrayIndex + area.arrayRange; ++array)
      if (obj->layout.get(mip, array) != layout)
      {
        changesLayout = true;
        return true;
      }
  return false;
}

bool ExecutionSyncTracker::ImageOp::mergeCheck(ImageArea, ImageOpAdditionalParams extra)
{
  // if we somehow try to merge intersecting ops with different layout or pass
  // better to know this ahead of time and trigger assert
  if (extra.layout != layout)
    return false;
  else if (extra.nativePassIdx != nativePassIdx)
    return false;

  return true;
}

bool ExecutionSyncTracker::ImageOpsArray::isRoSealValidForOperation(Image *obj, ImageOpAdditionalParams params)
{
  return obj->layout.roSealTargetLayout == params.layout;
}

bool ExecutionSyncTracker::ImageOp::isAreaPartiallyCoveredBy(const ImageOp &dst)
{
  if (!changesLayout)
    return false;

  return !((dst.area.mipIndex <= area.mipIndex) && (dst.area.mipIndex + dst.area.mipRange >= area.mipIndex + area.mipRange) &&
           (dst.area.arrayIndex <= area.arrayIndex) &&
           (dst.area.arrayIndex + dst.area.arrayRange >= area.arrayIndex + area.arrayRange));
}

void ExecutionSyncTracker::AreaCoverageMap::init(ImageArea area)
{
  xsz = area.mipRange;
  ysz = area.arrayRange;
  bits.resize(xsz * ysz, true);

  // original rect
  x0 = area.mipIndex;
  y0 = area.arrayIndex;
  x1 = x0 + xsz;
  y1 = y0 + ysz;

  processedLine = 0;
}

void ExecutionSyncTracker::AreaCoverageMap::exclude(ImageArea area)
{
  // exclude rect
  uint32_t cx0 = area.mipIndex;
  uint32_t cy0 = area.arrayIndex;
  uint32_t cx1 = cx0 + area.mipRange;
  uint32_t cy1 = cy0 + area.arrayRange;

  // clamp to edges
  cx0 = max(cx0, x0);
  cy0 = max(cy0, y0);
  cx1 = min(cx1, x1);
  cy1 = min(cy1, y1);

  // remove base offset
  cx0 -= x0;
  cx1 -= x0;
  cy0 -= y0;
  cy1 -= y0;

  // clear bits
  for (uint32_t i = cx0; i < cx1; ++i)
    for (uint32_t j = cy0; j < cy1; ++j)
      bits[i * ysz + j] = false;
}

bool ExecutionSyncTracker::AreaCoverageMap::getArea(ImageArea &area)
{
  uint32_t mergePoint = 0;
  bool areaFilled = false;
  // try to generate at least line-by-line barriers
  for (uint32_t i = processedLine; i < xsz; ++i)
  {
    for (uint32_t j = 0; j < ysz; ++j)
    {
      int idx = i * ysz + j;
      if (bits[idx])
      {
        if (areaFilled)
        {
          if (mergePoint + 1 == j)
          {
            ++area.arrayRange;
            mergePoint = j;
          }
          else
            return true;
        }
        else
        {
          area = {x0 + i, 1, y0 + j, 1};
          areaFilled = true;
          mergePoint = j;
        }
        bits[idx] = false;
      }
    }
    if (areaFilled)
    {
      // continue from unprocessed area
      processedLine = i + 1;
      return true;
    }
  }
  return false;
}
