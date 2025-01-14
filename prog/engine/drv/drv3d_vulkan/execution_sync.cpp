// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "device_resource.h"
#include "image_resource.h"
#include "buffer_resource.h"
#include "resource_manager.h"
#include "globals.h"
#include "backend.h"
#include "execution_state.h"
#include "execution_context.h"
#include "execution_sync_capture.h"

using namespace drv3d_vulkan;

typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::PIPE_SYNC> InternalPipelineBarrier;

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
uint32_t ExecutionSyncTracker::OpUid::frame_local_next_op_uid = 0;
#endif

// #define PROFILE_SYNC(x) TIME_PROFILE(x)
#define PROFILE_SYNC(x)

// universal operations sync algorithm
// for each newly added operation, find any conflicting previous non-completed ones (conflicting = needed to be synced),
// generate sync barriers for such operations via templated option of PipelineBarrier class

template <typename OpsArrayType>
struct OpsProcessAlgorithm
{
  OpsArrayType &ops;
  ExecutionSyncTracker::ScratchData &scratch;
  InternalPipelineBarrier &barrier;
  size_t gpuWorkId;
  size_t lastAdded;

  OpsProcessAlgorithm(OpsArrayType &in_ops, ExecutionSyncTracker::ScratchData &in_scratch, InternalPipelineBarrier &in_barrier,
    size_t gpu_work_id) :
    ops(in_ops), scratch(in_scratch), barrier(in_barrier), gpuWorkId(gpu_work_id), lastAdded(ops.arr.size())
  {}

  void sortByObject()
  {
    PROFILE_SYNC(vk_sync_sort);
    // sort added ops inside vector
    // and record range start-end for each unique object
    for (size_t i = ops.lastProcessed; i < lastAdded;)
    {
      void *currObj = (void *)ops.arr[i].obj;
      size_t startIdx = i;
      for (size_t j = i + 1; j < lastAdded; ++j)
      {
        if (currObj == ((void *)ops.arr[j].obj))
        {
          if (j > i + 1)
            eastl::swap(ops.arr[j], ops.arr[i + 1]);
          ++i;
        }
      }
      // last sync op is reused here to reflect "last" op in sorted single object sequence
      ops.arr[startIdx].obj->setLastSyncOpRange(startIdx, i);
      ++i;
    }
  }

  void checkForSelfConflicts()
  {
    PROFILE_SYNC(vk_sync_self_conflict_check);

    // newly added ops are object sorted, use this fact to iterate faster
    for (size_t i = ops.lastProcessed; i < lastAdded;)
    {
      int rangeEnd = ops.arr[i].obj->getLastSyncOpIndex();
      for (size_t k = i; k <= rangeEnd; ++k)
        for (size_t j = k + 1; j <= rangeEnd; ++j)
        {
          // means that command stream will have conflicting RW ops that are not solvable
          // add additional sync at caller site between such ops
          if (!ops.arr[k].verifySelfConflict(ops.arr[j]))
          {
            D3D_ERROR("vulkan: mutual-conflicting sync ops %u-%u in complete step!\n %s vs %s from %s", k, j, ops.arr[k].format(),
              ops.arr[j].format(), Backend::State::exec.getExecutionContext().getCurrentCmdCaller());
            G_ASSERTF(false,
              "vulkan: mutual-conflicting sync ops in complete step! \n This is application error! Check log and fix at callsite!\n");
          }
        }
      i = rangeEnd + 1;
    }
  }

  void handlePartialCoverings(size_t srcOpIndex)
  {
    PROFILE_SYNC(vk_sync_partial_covering);
    auto &srcOp = ops.arr[srcOpIndex];
    scratch.coverageMap.init(srcOp.area);
    auto cachedArea = ops.arr[scratch.partialCoveringDsts[0]].area;

    // exclude completed areas while merging them if possible
    for (size_t j : scratch.partialCoveringDsts)
    {
      auto &dstOp = ops.arr[j];
      if (cachedArea.mergable(dstOp.area))
        cachedArea.merge(dstOp.area);
      else
      {
        scratch.coverageMap.exclude(cachedArea);
        cachedArea = dstOp.area;
      }
    }
    scratch.coverageMap.exclude(cachedArea);

    // get processable areas from coverage map till its empty
    // (getArea returns false when coverage map is empty)

    // replace original first
    bool replaceArea = scratch.coverageMap.getArea(cachedArea);
    if (replaceArea)
      srcOp.area = cachedArea;
    else
      // if in sum we fully covered src op, complete it
      srcOp.completed = true;

    // add other parts if any
    while (scratch.coverageMap.getArea(cachedArea))
    {
      ops.arr.push_back_uninitialized();
      // read from proper memory if vector was reallocated
      ops.arr.back() = ops.arr[srcOpIndex];
      ops.arr.back().uid = ExecutionSyncTracker::OpUid::next();
      ops.arr.back().area = cachedArea;
      ops.arr.back().onPartialSplit();
      Backend::syncCapture.addOp(ops.arr.back().uid, ops.arr.back().laddr, ops.arr.back().obj, ops.arr.back().caller);
    }

    // cleanup scratches
    scratch.partialCoveringDsts.clear();
    scratch.coverageMap.clear();
  }

  void detectConflictingOps()
  {
    PROFILE_SYNC(vk_sync_detect_conflicts);

    for (size_t i = ops.lastIncompleted; i < ops.lastProcessed; ++i)
    {
      auto &srcOp = ops.arr[i];
      G_ASSERTF(!srcOp.completed, "vulkan: loop reduce failed to sort out completed srcOps");
      if (!srcOp.obj->hasLastSyncOpIndex())
        continue;

      // push once to scratch srcOps if any conflict found
      bool conflicting = false;
      size_t rStart = srcOp.obj->getFirstSyncOpIndex();
      size_t rEnd = srcOp.obj->getLastSyncOpIndex();
      for (size_t j = rStart; j <= rEnd; ++j)
      {
        auto &dstOp = ops.arr[j];
        if (dstOp.conflicts(srcOp))
        {
          Backend::syncCapture.addLink(srcOp.uid, dstOp.uid);
          srcOp.onConflictWithDst(dstOp);
          if (srcOp.processPartials())
          {
            // use partial solving if we already faced partial coverage
            // or not faced any conflicts at all
            if (srcOp.isAreaPartiallyCoveredBy(dstOp) && (conflicting == false || scratch.partialCoveringDsts.size()))
              scratch.partialCoveringDsts.push_back(j);
            // when we have some dst that fully covers src, reset tracking
            else if (scratch.partialCoveringDsts.size())
              scratch.partialCoveringDsts.clear();
          }
          conflicting = true;
          dstOp.dstConflict = true;
        }
      }

      if (conflicting)
      {
        scratch.src.push_back(i);
        if (scratch.partialCoveringDsts.size())
          handlePartialCoverings(i);
        else
          // mark completed ahead of time
          srcOp.completed = true;
      }
    }
  }

  void finalizeConflictSearch()
  {
    PROFILE_SYNC(vk_sync_finalize);
    const bool fillDstScratch = scratch.src.size() > 0 || ops.arr[0].allowsConflictFromObject();
    // newly added ops are object sorted, use this fact to iterate faster
    for (size_t i = ops.lastProcessed; i < lastAdded;)
    {
      // clear last sync op in object here, as we are about to finish processing this last op
      size_t rangeEnd = ops.arr[i].obj->getLastSyncOpIndex();
      ops.arr[i].obj->clearLastSyncOp();
      // fill dst ops to scratch buffer in order to not duplicate them
      if (fillDstScratch)
      {
        for (size_t j = i; j <= rangeEnd; ++j)
          if (ops.arr[j].dstConflict || ops.arr[j].hasObjConflict())
            scratch.dst.push_back(j);
      }
      i = rangeEnd + 1;
    }
  }

  void buildBarrier()
  {
    PROFILE_SYNC(vk_sync_build_barrier);
    if (scratch.src.size() == 0 && scratch.dst.size() == 0)
      return;

    // fill barrier
    for (size_t i : scratch.src)
      ops.arr[i].addToBarrierByTemplateSrc(barrier);

    for (size_t i : scratch.dst)
      ops.arr[i].addToBarrierByTemplateDst(barrier);

    // if src ops did not added any stages due to suppresion or full src-less conflicts
    // do src less barrier as-is if synchronization2 is available
    // otherwise do full commands barrier
    if (!Globals::VK::phy.hasSynchronization2 && barrier.getStagesSrc() == VK_PIPELINE_STAGE_NONE)
      barrier.addStagesSrc(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  }

  void reduceLoopRanges()
  {
    PROFILE_SYNC(vk_sync_reduce_ranges);
    for (size_t i : scratch.src)
    {
      if (ops.arr[i].processPartials() && !ops.arr[i].completed)
        continue;
      if (i != ops.lastIncompleted)
        ops.arr[i] = ops.arr[ops.lastIncompleted];
      ++ops.lastIncompleted;
    }
    ops.lastProcessed = ops.arr.size();
  }

  void completeNeeded()
  {
    //// init phase
    // sort newly added ops by object for matching them with old non completed ops
    sortByObject();
#if EXECUTION_SYNC_CHECK_SELF_CONFLICTS > 0
    // verify that last added ops is not conflicting with each other
    checkForSelfConflicts();
#endif
    //// conflict find phase
    // check if incompleted ops conflict with newly added
    detectConflictingOps();
    // process per object triggers and fill dst scratch info
    finalizeConflictSearch();
    //// sync build phase
    buildBarrier();
    //// finalize phase
    // reduce loop ranges to avoid processing completed/viewed ops
    reduceLoopRanges();
    // cleanup scratch data (but keep memory for zero alloc!)
    scratch.clear();
  }
};

template <typename OpsArrayType, typename Resource, typename AreaType, typename Optional>
bool mergeToLastSyncOp(OpsArrayType &ops, Resource *obj, LogicAddress laddr, AreaType area, Optional opt)
{
  size_t lastSyncOp = obj->getLastSyncOpIndex();
  if (lastSyncOp < ops.arr.size())
  {
    auto &lop = ops.arr[lastSyncOp];
    bool areaMergable = lop.area.mergable(area) && lop.mergeCheck(area, opt);
    if (areaMergable)
    {
      // primary check for conflicting logic address
      bool laddrMergable = !lop.laddr.mergeConflicting(laddr);
      // try again to see if we using same address but non intersecting yet connected(mergable) areas
      if (!laddrMergable)
        laddrMergable = !lop.area.intersects(area) && lop.laddr.equal(laddr);
      if (laddrMergable)
      {
        lop.area.merge(area);
        lop.laddr.merge(laddr);
        return true;
      }
    }
  }
  obj->setLastSyncOpIndex(ops.arr.size());
  return false;
}

template <typename OpsArrayType, typename Resource, typename Optional>
bool filterReadsOnSealedObjects(size_t gpu_work_id, OpsArrayType &ops, Resource *obj, LogicAddress laddr, Optional opt)
{
  if (obj->hasRoSeal())
  {
    if (laddr.isWrite() || !ops.isRoSealValidForOperation(obj, opt))
    {
      // if seal removal is not silent - we must add last RO op to current uncompleted ops
      if (!obj->removeRoSealSilently(gpu_work_id))
      {
        ops.arr.push_back();
        ops.arr.back() = ops.arr[ops.lastProcessed];
        ops.removeRoSeal(obj);
        // at least track to prev sync step
        Backend::syncCapture.addOpPrevStep(ops.arr[ops.lastProcessed].uid, ops.arr[ops.lastProcessed].laddr,
          ops.arr[ops.lastProcessed].obj, ops.arr[ops.lastProcessed].caller);
        ++ops.lastProcessed;
      }
    }
    else
    {
      obj->updateRoSeal(gpu_work_id, laddr);
      return true;
    }
  }
  return false;
}

template <typename OpsArrayType, typename Resource, typename AreaType, typename Optional>
bool filterAccessTracking(size_t gpu_work_id, OpsArrayType &ops, Resource *obj, LogicAddress laddr, AreaType area, Optional opt)
{
  if (filterReadsOnSealedObjects(gpu_work_id, ops, obj, laddr, opt))
    return true;
  if (mergeToLastSyncOp(ops, obj, laddr, area, opt))
    return true;
  return false;
}

template <typename SrcResource, typename OpsArrayType>
void aliasSyncInOpsArray(SrcResource *lobj, const AliasedResourceMemory &lmem, OpsArrayType &ops, VkPipelineStageFlags src_stage,
  ExecutionSyncTracker &tracker)
{
#if EXECUTION_SYNC_CHECK_SELF_CONFLICTS > 0
  const size_t loopRange = ops.arr.size();
#else
  const size_t loopRange = ops.lastProcessed;
#endif
  for (size_t i = ops.lastIncompleted; i < loopRange; ++i)
  {
    auto &op = ops.arr[i];
    auto *robj = op.obj;
    if (robj == lobj)
      continue;

    if (!robj->mayAlias())
      continue;

    // "alias deactivate" pending op should not generate more alias deactivate actions
    if (op.laddr.access == VK_ACCESS_NONE)
      continue;

    const AliasedResourceMemory &rmem = Backend::aliasedMemory.get(robj->getMemoryId());
    if (rmem.intersects(lmem))
    {
#if EXECUTION_SYNC_CHECK_SELF_CONFLICTS > 0
      if (i >= ops.lastProcessed)
        D3D_ERROR("vulkan: alias %p:%s from %p:%s in single sync step! objects are used at same time!", lobj, lobj->getDebugName(),
          robj, robj->getDebugName());
      else
#endif
      {
        op.aliasEndAccess(src_stage, tracker);
      }
    }
  }
}

// need this because there is no mayAlias inside base resource object type
template <typename SrcResource>
void aliasCheckAndSync(SrcResource *lobj, LogicAddress laddr, ExecutionSyncTracker &tracker)
{
  // TODO: optimize: check once per alias change (right now checks for all sync ops)
  if (laddr.access == VK_ACCESS_NONE)
    return;

  if (!lobj->mayAlias())
    return;

  tracker.aliasSync(lobj, laddr.stage);
}

void ExecutionSyncTracker::aliasSync(Resource *lobj, VkPipelineStageFlags stage)
{
  // use special storage to avoid locking memory mutex
  const AliasedResourceMemory &lmem = Backend::aliasedMemory.get(lobj->getMemoryId());
  {
    PROFILE_SYNC(vulkan_alias_sync_buf);
    aliasSyncInOpsArray(lobj, lmem, bufOps, stage, *this);
  }
  {
    PROFILE_SYNC(vulkan_alias_sync_img);
    aliasSyncInOpsArray(lobj, lmem, imgOps, stage, *this);
  }
}

void ExecutionSyncTracker::addBufferAccess(LogicAddress laddr, Buffer *buf, BufferArea area)
{
  buf->checkFrameMemAccess();

  if (filterAccessTracking(gpuWorkId, bufOps, buf, laddr, area, 0))
    return;
  aliasCheckAndSync(buf, laddr, *this);

  bufOps.arr.push_back({OpUid::next(), laddr, buf, area, getCaller(), VK_ACCESS_NONE, /*completed*/ false, /*dstConflict*/ false});
  Backend::syncCapture.addOp(bufOps.arr.back().uid, bufOps.arr.back().laddr, bufOps.arr.back().obj, bufOps.arr.back().caller);
}

void ExecutionSyncTracker::addImageAccessImpl(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area,
  bool nrp_attachment, bool discard)
{
  // 3d image has no array range, but image view uses it to select the slices, so reset array range.
  if (VK_IMAGE_TYPE_3D == img->getType())
  {
    area.arrayIndex = 0;
    area.arrayRange = 1;
  }

  G_ASSERTF(
    (area.mipIndex + area.mipRange - 1) + img->layout.mipLevels * (area.arrayIndex + area.arrayRange - 1) < img->layout.data.size(),
    "vulkan: invalid subres accessed in image %p:%s : mip[%u-%u] layer[%u-%u] mips %u layers %u", img, img->getDebugName(),
    area.mipIndex, area.mipIndex + area.mipRange, area.arrayIndex, area.arrayIndex + area.arrayRange, img->layout.mipLevels,
    img->layout.data.size() / img->layout.mipLevels);

  ImageOpAdditionalParams opAddParams{layout, currentRenderSubpass};
  if (filterAccessTracking(gpuWorkId, imgOps, img, laddr, area, opAddParams))
    return;

  aliasCheckAndSync(img, laddr, *this);


  imgOps.arr.push_back({OpUid::next(), laddr, img, area, getCaller(), VK_ACCESS_NONE, layout, currentRenderSubpass, nativeRPIndex,
    /*completed*/ false, /*dstConflict*/ false,
    /*changesLayout*/ false, nrp_attachment, /*handledBySubpassDependency*/ false, /* discard */ discard});
  Backend::syncCapture.addOp(imgOps.arr.back().uid, imgOps.arr.back().laddr, imgOps.arr.back().obj, imgOps.arr.back().caller);
}

void ExecutionSyncTracker::setCurrentRenderSubpass(uint8_t subpass_idx)
{
  currentRenderSubpass = subpass_idx;
  if (subpass_idx == SUBPASS_NON_NATIVE)
  {
    ++nativeRPIndex;
    G_ASSERTF(nativeRPIndex != NATIVE_RP_INDEX_MAX, "vulkan: sync: too much native RPs in single frame, increase tracker field size");
  }
}

void ExecutionSyncTracker::ScratchData::clear()
{
  src.clear();
  dst.clear();
}

void ExecutionSyncTracker::completeNeeded()
{
  // early exit if nothing to be done or delay is enabled
  if (!anyNonProcessed() || delayCompletion)
    return;

  InternalPipelineBarrier barrier(0, 0);

  if (bufOps.lastProcessed != bufOps.arr.size())
  {
    PROFILE_SYNC(vulkan_buf_sync);
    OpsProcessAlgorithm<BufferOpsArray> bufAlg(bufOps, scratch, barrier, gpuWorkId);
    bufAlg.completeNeeded();
  }

  if (imgOps.lastProcessed != imgOps.arr.size())
  {
    PROFILE_SYNC(vulkan_img_sync);
    OpsProcessAlgorithm<ImageOpsArray> imgAlg(imgOps, scratch, barrier, gpuWorkId);
    imgAlg.completeNeeded();
  }

#if D3D_HAS_RAY_TRACING
  if (asOps.lastProcessed != asOps.arr.size())
  {
    PROFILE_SYNC(vulkan_as_sync);
    OpsProcessAlgorithm<AccelerationStructureOpsArray> asAlg(asOps, scratch, barrier, gpuWorkId);
    asAlg.completeNeeded();
  }
#endif

  if (!barrier.empty())
  {
    PROFILE_SYNC(vulkan_sync_barrier);
    barrier.submit();
  }
  Backend::syncCapture.addSyncStep();
}

void ExecutionSyncTracker::clearOps()
{
  bufOps.clear();
  imgOps.clear();
#if D3D_HAS_RAY_TRACING
  asOps.clear();
#endif
}

void ExecutionSyncTracker::completeOnQueue(size_t gpu_work_id)
{
  completeNeeded();
  G_ASSERTF(!delayCompletion, "vulkan: sync delay must not be interrupted by GPU job change");

  workItemEndBarrier(gpu_work_id);
}

void ExecutionSyncTracker::completeAll(size_t gpu_work_id)
{
  completeNeeded();
  G_ASSERTF(!delayCompletion, "vulkan: sync delay must not be interrupted by GPU job change");

  // ending current block, so increment in advance
  gpuWorkId = gpu_work_id + 1;
  OpUid::frame_end();
  Backend::syncCapture.reset();

  workItemEndBarrier(gpu_work_id);

  nativeRPIndex = 0;
}

void ExecutionSyncTracker::workItemEndBarrier(size_t gpu_work_id)
{
  if (allCompleted())
  {
    clearOps();
    return;
  }

  // do global memory barrier for now
  LogicAddress srcLA = {VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE};
  for (size_t i = bufOps.lastIncompleted; i < bufOps.arr.size(); ++i)
  {
    const BufferOp &op = bufOps.arr[i];
    if (op.completed)
      continue;

    // can't complain based on this logic as some writes can be leaved for frame end sync
    // if (op.laddr.isWrite() && op.obj->isUsedInBindless())
    //  D3D_ERROR("vulkan: sync: buffer: incompleted write while registered in bindless, must handle it! %s", op.format());

    // seal obj on frame end if last op was uncompleted read
    // it will be auto-unsealed on next frame if someone wants to write to it
    if (!op.laddr.isWrite())
      op.obj->optionallyActivateRoSeal(gpu_work_id);

    srcLA.merge(op.laddr);
  }

  for (size_t i = imgOps.lastIncompleted; i < imgOps.arr.size(); ++i)
  {
    const ImageOp &op = imgOps.arr[i];
    if (op.completed)
      continue;

    if (op.laddr.isWrite() && op.obj->isUsedInBindless())
      D3D_ERROR("vulkan: sync: image: incompleted write while registered in bindless, must handle it! %s", op.format());

    if (!op.laddr.isWrite() && op.obj->layout.roSealTargetLayout != VK_IMAGE_LAYOUT_UNDEFINED)
    {
      bool canSeal = true;
      for (VkImageLayout i : op.obj->layout.data)
        if (i != op.obj->layout.roSealTargetLayout)
        {
          canSeal = false;
          break;
        }
      if (canSeal)
        op.obj->optionallyActivateRoSeal(gpu_work_id);
    }

    srcLA.merge(op.laddr);
  }

#if D3D_HAS_RAY_TRACING
  for (size_t i = asOps.lastIncompleted; i < asOps.arr.size(); ++i)
  {
    const AccelerationStructureOp &op = asOps.arr[i];
    if (op.completed)
      continue;

    // seal obj on frame end if last op was uncompleted read
    // it will be auto-unsealed on next frame if someone wants to write to it
    if (!op.laddr.isWrite())
      op.obj->optionallyActivateRoSeal(gpu_work_id);

    srcLA.merge(op.laddr);
  }
#endif

  InternalPipelineBarrier barrier(srcLA.stage, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
  barrier.addMemory({srcLA.access, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT});
  // can be empty due to native RP sync exclusion
  if (srcLA.stage != VK_PIPELINE_STAGE_NONE)
    barrier.submit();

  clearOps();
}

bool ExecutionSyncTracker::anyNonProcessed()
{
  return bufOps.lastProcessed != bufOps.arr.size() || imgOps.lastProcessed != imgOps.arr.size()
#if D3D_HAS_RAY_TRACING
         || asOps.lastProcessed != asOps.arr.size()
#endif
    ;
}

bool ExecutionSyncTracker::allCompleted()
{
  return bufOps.lastIncompleted == bufOps.arr.size() && imgOps.lastIncompleted == imgOps.arr.size()
#if D3D_HAS_RAY_TRACING
         && asOps.lastIncompleted == asOps.arr.size()
#endif
    ;
}

#if EXECUTION_SYNC_TRACK_CALLER > 0

ExecutionSyncTracker::OpCaller ExecutionSyncTracker::getCaller()
{
  return {backtrace::get_hash(1), Backend::State::exec.getExecutionContext().getCurrentCmdCallerHash()};
}

String ExecutionSyncTracker::OpCaller::getInternal() const { return backtrace::get_stack_by_hash(internal); }
String ExecutionSyncTracker::OpCaller::getExternal() const { return backtrace::get_stack_by_hash(external); }

#endif

#if D3D_HAS_RAY_TRACING

void ExecutionSyncTracker::addAccelerationStructureAccess(LogicAddress laddr, RaytraceAccelerationStructure *as)
{
  AccelerationStructureArea area;
  if (filterAccessTracking(gpuWorkId, asOps, as, laddr, area, 0))
    return;

  asOps.arr.push_back({OpUid::next(), laddr, as, area, getCaller(), VK_ACCESS_NONE, /*completed*/ false, /*dstConflict*/ false});
  Backend::syncCapture.addOp(asOps.arr.back().uid, asOps.arr.back().laddr, asOps.arr.back().obj, asOps.arr.back().caller);
}

#endif