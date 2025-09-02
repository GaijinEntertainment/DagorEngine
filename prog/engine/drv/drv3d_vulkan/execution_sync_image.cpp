// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv_log_defs.h>

#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "image_resource.h"
#include "globals.h"
#include "driver_config.h"
#include "vk_to_string.h"
#include "backend_interop.h"
#include "backend.h"

using namespace drv3d_vulkan;

template <>
String ImageSyncOp::format() const
{
  return String(128,
    "syncImgOp %p:\n%p-%s [mip:%u-%u arr:%u-%u]\n%s subpass %u NRP idx %u\n%s\n%s\n internal caller: %s\n external caller: %s", this,
    obj, obj->getDebugName(), area.mipIndex, area.mipIndex + area.mipRange, area.arrayIndex, area.arrayIndex + area.arrayRange,
    formatImageLayout(layout), subpassIdx, nativeRPIndex, formatPipelineStageFlags(laddr.stage), formatMemoryAccessFlags(laddr.access),
    caller.getInternal(), caller.getExternal());
}

namespace
{

// resets fields that are used at complete step and need to be cleared for next such step
void resetIntermediateTracking(ImageSyncOp &op)
{
  // clear change layout flag, as it dynamically computed at conflict check stage
  op.changesLayout = false;
  op.handledBySubpassDependency = false;
  // also clear accumulated conflicting access flags to not carry them over to next sync step
  // (needed because we can fill them at dst conflict and then read again in src conflict)
  op.conflictingLAddr.clear();
}


// applies layout change to object and does related work for op obj
void processLayoutChange(ImageSyncOp &op)
{
#if DAGOR_DBGLEVEL > 0
  if (Globals::cfg.bits.debugGarbadgeReads && op.laddr.isRead() && !op.laddr.isWrite())
  {
    for (uint32_t mip = op.area.mipIndex; mip < op.area.mipIndex + op.area.mipRange; ++mip)
      for (uint32_t array = op.area.arrayIndex; array < op.area.arrayIndex + op.area.arrayRange; ++array)
        if (op.obj->layout.get(mip, array) == VK_IMAGE_LAYOUT_UNDEFINED)
        {
          D3D_ERROR("vulkan: reading garbage from image %p:%s mip %u layer %u at sync op %s", op.obj, op.obj->getDebugName(), mip,
            array, op.format());
          break;
        }
  }
#endif

  for (uint32_t mip = op.area.mipIndex; mip < op.area.mipIndex + op.area.mipRange; ++mip)
    for (uint32_t array = op.area.arrayIndex; array < op.area.arrayIndex + op.area.arrayRange; ++array)
      op.obj->layout.set(mip, array, op.layout);

  resetIntermediateTracking(op);
}

} // namespace

template <>
bool ImageSyncOp::verifySelfConflict(const ImageSyncOp &cmp) const
{
  // use stageless conflict check, there is no way to self conflict in single action step, based on stages
  bool stagelessConflict =
    area.intersects(cmp.area) && (laddr.conflictingStageless(cmp.laddr) || layout != cmp.layout) && !completed && !cmp.completed;
  bool ret = !stagelessConflict;

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

template <>
bool ImageSyncOp::conflicts(const ImageSyncOp &cmp) const
{
  // clang-format off
  return area.intersects(cmp.area)
    && (laddr.conflicting(cmp.laddr) || layout != cmp.layout)
    && !completed && !cmp.completed;
  // clang-format on
}

template <>
void ImageSyncOp::addToBarrierByTemplateSrc(PipelineBarrier &barrier)
{
  if (shouldSuppress())
  {
    resetIntermediateTracking(*this);
    return;
  }

  // memory barriers only

  if (!changesLayout)
  {
    barrier.modifyImageTemplate(obj);
    barrier.modifyImageTemplateStage({laddr.stage, conflictingLAddr.stage});
    barrier.modifyImageTemplateNewLayout(layout);
    barrier.modifyImageTemplateOldLayout(layout);
    barrier.modifyImageTemplate(area.mipIndex, area.mipRange, area.arrayIndex, area.arrayRange);
    barrier.addImageByTemplate({laddr.access, conflictingLAddr.access});
  }

  resetIntermediateTracking(*this);
}

template <>
void ImageSyncOp::addToBarrierByTemplateDst(PipelineBarrier &barrier)
{
  if (shouldSuppress())
  {
    processLayoutChange(*this);
    return;
  }

  // layout changing barriers only!
  if (!changesLayout)
  {
    resetIntermediateTracking(*this);
    return;
  }

  barrier.modifyImageTemplateStage({conflictingLAddr.stage, laddr.stage});

  bool aliasDeactivate = layout == VK_IMAGE_LAYOUT_UNDEFINED;

  barrier.modifyImageTemplate(obj);
  barrier.modifyImageTemplateNewLayout(layout);

  bool splitted = false;
  VkImageLayout zeroLayout = discard ? VK_IMAGE_LAYOUT_UNDEFINED : obj->layout.get(area.mipIndex, area.arrayIndex);
  if (!discard)
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
    if (aliasDeactivate)
      barrier.modifyImageTemplateNewLayout(zeroLayout);
    barrier.modifyImageTemplateOldLayout(zeroLayout);
    barrier.addImageByTemplate({conflictingLAddr.access, laddr.access});
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
        if (aliasDeactivate)
          barrier.modifyImageTemplateNewLayout(subresLayout);
        barrier.modifyImageTemplate(mip, 1, array, 1);
        if (!discard)
          barrier.modifyImageTemplateOldLayout(subresLayout);
        barrier.addImageByTemplate({conflictingLAddr.access, laddr.access});
      }
    }
  }

  processLayoutChange(*this);
}

template <>
void ImageSyncOp::onConflictWithDst(ImageSyncOp &dst)
{
  // record to both ops, to properly handle layout chaning barriers (always dst barriers)
  conflictingLAddr.merge(dst.laddr);
  dst.conflictingLAddr.merge(laddr);

  bool differentLayouts = dst.layout != layout;
  changesLayout |= differentLayouts;
  dst.changesLayout |= differentLayouts;

  // clang-format off
  const bool handledBySubpassDep =
    subpassIdx != ExecutionSyncTracker::SUBPASS_NON_NATIVE &&
    dst.subpassIdx != ExecutionSyncTracker::SUBPASS_NON_NATIVE &&
    nativeRPIndex == dst.nativeRPIndex &&
    nrpAttachment && dst.nrpAttachment;
  // clang-format on
  handledBySubpassDependency = handledBySubpassDep;
  dst.handledBySubpassDependency = handledBySubpassDep;
}

template <>
bool ImageSyncOp::hasObjConflict()
{
  // obj conflict tracks layout change for object without src pending operation
  // so if we found layout change at src conflict checks, no need to search for layout change twice
  // check should be short circuited at callsite of this method, so put assert here instead
  G_ASSERTF(!changesLayout && !dstConflict,
    "vulkan: ImageSyncOp::hasObjConflict called when layout change is [%s] and dst conflict is [%s]", changesLayout ? "yes" : "no",
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

template <>
bool ImageSyncOp::mergeCheck(ImageArea, ImageSyncOpAdditionalParams extra)
{
  // if we somehow try to merge intersecting ops with different layout or pass
  // better to know this ahead of time and trigger assert
  if (extra.layout != layout)
    return false;
  else if (extra.subpassIdx != subpassIdx)
    return false;

  return true;
}

template <>
bool ImageSyncOp::isAreaPartiallyCoveredBy(const ImageSyncOp &dst)
{
  if (!changesLayout)
    return false;

  return !((dst.area.mipIndex <= area.mipIndex) && (dst.area.mipIndex + dst.area.mipRange >= area.mipIndex + area.mipRange) &&
           (dst.area.arrayIndex <= area.arrayIndex) &&
           (dst.area.arrayIndex + dst.area.arrayRange >= area.arrayIndex + area.arrayRange));
}

template <>
void ImageSyncOp::onPartialSplit()
{
  resetIntermediateTracking(*this);
}

template <>
void ImageSyncOp::aliasEndAccess(VkPipelineStageFlags stage, ExecutionSyncTracker &tracker)
{
  tracker.addImageAccess({stage, VK_ACCESS_NONE}, obj, VK_IMAGE_LAYOUT_UNDEFINED, {0, obj->getMipLevels(), 0, obj->getArrayLayers()});
}

template <>
bool ImageSyncOp::allowsConflictFromObject()
{
  return true;
}

template <>
bool ImageSyncOp::processPartials()
{
  return true;
}

template <>
void ExecutionSyncTracker::ImageOpsArray::removeRoSeal(Image *obj)
{
  // we don't know actual caller here
  arr[lastProcessed] =
    ImageSyncOp(SyncOpUid::next(), {}, obj->getRoSealReads(), obj, {0, obj->getMipLevels(), 0, obj->getArrayLayers()});
  arr[lastProcessed].specificInit(false /*nrpAttachment*/, obj->layout.roSealTargetLayout, ExecutionSyncTracker::SUBPASS_NON_NATIVE, 0,
    false /*discard*/);
}

template <>
bool ExecutionSyncTracker::ImageOpsArray::isRoSealValidForOperation(Image *obj, ImageSyncOpAdditionalParams params)
{
  return obj->layout.roSealTargetLayout == params.layout;
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
