#include "execution_sync.h"
#include "pipeline_barrier.h"
#include "device.h"

using namespace drv3d_vulkan;

typedef ContextedPipelineBarrier<BuiltinPipelineBarrierCache::PIPE_SYNC> InternalPipelineBarrier;

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

  OpsProcessAlgorithm(OpsArrayType &in_ops, ExecutionSyncTracker::ScratchData &in_scratch, InternalPipelineBarrier &in_barrier,
    size_t gpu_work_id) :
    ops(in_ops), scratch(in_scratch), barrier(in_barrier), gpuWorkId(gpu_work_id)
  {}

  void sortByObject()
  {
    // sort added ops inside vector
    // and record range start-end for each unique object
    for (size_t i = ops.lastProcessed; i < ops.arr.size();)
    {
      void *currObj = (void *)ops.arr[i].obj;
      size_t startIdx = i;
      for (size_t j = i + 1; j < ops.arr.size(); ++j)
      {
        if (currObj == ((void *)ops.arr[j].obj))
        {
          if (j > i + 1)
          {
            auto swapTemp = ops.arr[i + 1];
            ops.arr[i + 1] = ops.arr[j];
            ops.arr[j] = swapTemp;
          }
          ++i;
        }
      }
      scratch.objSorted[currObj] = {startIdx, i};
      ++i;
    }
  }

  void checkForSelfConflicts()
  {
    for (const auto &i : scratch.objSorted)
    {
      ExecutionSyncTracker::ScratchData::DstObjInfo dstOps = i.second;
      for (size_t j = dstOps.start; j < dstOps.end; ++j)
      {
        for (size_t k = j + 1; k <= dstOps.end; ++k)
        {
          // means that command stream will have conflicting RW ops that are not solvable
          // add additional sync at caller site between such ops
          G_ASSERTF(ops.arr[j].verifySelfConflict(ops.arr[k]),
            "vulkan: mutual-conflicting sync ops %u-%u in complete step!\n %s vs %s", j, k, ops.arr[j].format(), ops.arr[k].format());
        }
      }
    }
  }

  void handlePartialCoverings(size_t srcOpIndex)
  {
    auto &srcOp = ops.arr[srcOpIndex];
    auto cachedArea = ops.arr[scratch.partialCoveringDsts[0]].area;
    scratch.coverageMap.init(srcOp.area);

    // merge dsts if possible
    for (size_t j : scratch.partialCoveringDsts)
    {
      auto &dstOp = ops.arr[j];
      if (cachedArea.mergable(dstOp.area))
      {
        cachedArea.merge(dstOp.area);
        continue;
      }
      else
      {
        scratch.coverageMap.exclude(cachedArea);
        cachedArea = dstOp.area;
      }
    }
    scratch.coverageMap.exclude(cachedArea);

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
      ops.arr.push_back(srcOp);
      ops.arr.back().area = cachedArea;
    }

    // cleanup scratches
    scratch.partialCoveringDsts.clear();
    scratch.coverageMap.clear();
  }

  void detectConflictingOps()
  {
    // push once to scratch srcOps if any conflict found
    const ExecutionSyncTracker::ScratchData::DstObjInfo emptyRefs{1, 0};
    ExecutionSyncTracker::ScratchData::DstObjInfo objRefs = emptyRefs;
    void *lastObj = nullptr;

    for (size_t i = ops.lastIncompleted; i < ops.lastProcessed; ++i)
    {
      auto &srcOp = ops.arr[i];
      if (srcOp.completed)
        continue;

      // op array is object sorted, so until object does not change, dst refs range is same
      if (lastObj != ((void *)srcOp.obj))
      {
        lastObj = (void *)srcOp.obj;
        auto objRefsIter = scratch.objSorted.find((void *)srcOp.obj);
        objRefs = (objRefsIter == scratch.objSorted.end()) ? emptyRefs : objRefs = objRefsIter->second;
      }
      if (objRefs.start > objRefs.end)
        continue;

      bool conflicting = false;
      for (size_t j = objRefs.start; j <= objRefs.end; ++j)
      {
        auto &dstOp = ops.arr[j];
        if (dstOp.conflicts(srcOp))
        {
          srcOp.onConflictWithDst(dstOp, gpuWorkId);
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
    const bool fillDstScratch = scratch.src.size() > 0 || ops.arr[0].allowsConflictFromObject();
    for (const auto &i : scratch.objSorted)
    {
      const ExecutionSyncTracker::ScratchData::DstObjInfo dstOps = i.second;
      // clear last sync op in object here, as we are about to finish processing this last op
      ops.arr[dstOps.start].obj->clearLastSyncOp();
      // fill dst ops to scratch buffer in order to not duplicate them
      if (fillDstScratch)
        for (int j = dstOps.start; j <= dstOps.end; ++j)
          if (ops.arr[j].dstConflict || ops.arr[j].hasObjConflict())
            scratch.dst.push_back(j);
    }
  }

  void buildBarrier()
  {
    if (scratch.src.size() == 0 && scratch.dst.size() == 0)
      return;

    // merge logical addresses
    ExecutionSyncTracker::LogicAddress srcLA = {VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE};
    ExecutionSyncTracker::LogicAddress dstLA = {VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE};

    // dangerous when we have src-less dst together with full src-dst pair
    // but most of time it is handled properly because full src-dst pair stage will match src-less only
    if (scratch.src.size() == 0)
      srcLA.stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

    for (size_t i : scratch.src)
      srcLA.merge(ops.arr[i].laddr);

    for (size_t i : scratch.dst)
      dstLA.merge(ops.arr[i].laddr);

    // fill barrier
    barrier.addStages(srcLA.stage, dstLA.stage);
    ops.arr[0].modifyBarrierTemplate(barrier, srcLA, dstLA);
    for (size_t i : scratch.src)
      ops.arr[i].addToBarrierByTemplateSrc(barrier);
    for (size_t i : scratch.dst)
      ops.arr[i].addToBarrierByTemplateDst(barrier, gpuWorkId);
  }

  void reduceLoopRanges()
  {
    for (size_t i = ops.lastIncompleted; i < ops.lastProcessed; ++i)
    {
      if (!ops.arr[i].completed)
      {
        ops.lastIncompleted = i;
        break;
      }
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
bool mergeToLastSyncOp(OpsArrayType &ops, Resource *obj, ExecutionSyncTracker::LogicAddress laddr, AreaType area, Optional opt)
{
  size_t lastSyncOp = obj->getLastSyncOpIndex();
  if (lastSyncOp < ops.arr.size())
  {
    auto &lop = ops.arr[lastSyncOp];
    if (lop.area.mergable(area) && !lop.laddr.mergeConflicting(laddr) && lop.mergeCheck(area, opt))
    {
      lop.area.merge(area);
      lop.laddr.merge(laddr);
      return true;
    }
  }
  obj->setLastSyncOpIndex(ops.arr.size());
  return false;
}

template <typename OpsArrayType, typename Resource, typename Optional>
bool filterReadsOnSealedObjects(size_t gpu_work_id, OpsArrayType &ops, Resource *obj, ExecutionSyncTracker::LogicAddress laddr,
  Optional opt)
{
  if (obj->hasRoSeal())
  {
    if (laddr.isWrite() || !ops.isRoSealValidForOperation(obj, opt))
    {
      if (obj->tryRemoveRoSeal(gpu_work_id))
        return false;
      else
        fatal("vulkan: trying to write into this work item RO sealed %s %p-%s\n%s", obj->resTypeString(), obj, obj->getDebugName(),
          obj->getCallers());
    }
    else
      return true;
  }
  return false;
}

template <typename OpsArrayType, typename Resource, typename AreaType, typename Optional>
bool filterAccessTracking(size_t gpu_work_id, OpsArrayType &ops, Resource *obj, ExecutionSyncTracker::LogicAddress laddr,
  AreaType area, Optional opt)
{
  if (filterReadsOnSealedObjects(gpu_work_id, ops, obj, laddr, opt))
    return true;
  if (mergeToLastSyncOp(ops, obj, laddr, area, opt))
    return true;
  return false;
}

void ExecutionSyncTracker::addBufferAccess(LogicAddress laddr, Buffer *buf, BufferArea area)
{
  if (filterAccessTracking(gpuWorkId, bufOps, buf, laddr, area, 0))
    return;
  bufOps.arr.push_back({laddr, buf, area, getCaller(), /*completed*/ false, /*dstConflict*/ false});
}

void ExecutionSyncTracker::addImageAccessFull(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area,
  uint16_t native_pass_idx)
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

  ImageOpAdditionalParams opAddParams{layout, native_pass_idx};
  if (filterAccessTracking(gpuWorkId, imgOps, img, laddr, area, opAddParams))
    return;

  imgOps.arr.push_back({laddr, img, area, getCaller(), layout, native_pass_idx, /*completed*/ false, /*dstConflict*/ false,
    /*changesLayout*/ false});
}

void ExecutionSyncTracker::addImageAccessAtNativePass(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area)
{
  addImageAccessFull(laddr, img, layout, area, nativePassIdx);
}

void ExecutionSyncTracker::addImageAccess(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area)
{
  addImageAccessFull(laddr, img, layout, area, 0);
}

void ExecutionSyncTracker::enterNativePass()
{
  for (size_t i = imgOps.lastIncompleted; i < imgOps.lastProcessed; ++i)
    imgOps.arr[i].onNativePassEnter(nativePassIdx);
  ++nativePassIdx;
}

void ExecutionSyncTracker::ScratchData::clear()
{
  src.clear();
  dst.clear();
  objSorted.clear();
}

void ExecutionSyncTracker::completeNeeded(VulkanCommandBufferHandle cmd_buffer, const VulkanDevice &dev)
{
  // early exit if nothing to be done
  if (!anyNonProcessed())
    return;

  TIME_PROFILE(vulkan_sync_complete_needed);

  InternalPipelineBarrier barrier(dev);

  if (bufOps.lastProcessed != bufOps.arr.size())
  {
    TIME_PROFILE(vulkan_buf_sync);
    OpsProcessAlgorithm<BufferOpsArray> bufAlg(bufOps, scratch, barrier, gpuWorkId);
    bufAlg.completeNeeded();
  }

  if (imgOps.lastProcessed != imgOps.arr.size())
  {
    TIME_PROFILE(vulkan_img_sync);
    OpsProcessAlgorithm<ImageOpsArray> imgAlg(imgOps, scratch, barrier, gpuWorkId);
    imgAlg.completeNeeded();
  }

#if D3D_HAS_RAY_TRACING
  if (asOps.lastProcessed != asOps.arr.size())
  {
    TIME_PROFILE(vulkan_as_sync);
    OpsProcessAlgorithm<AccelerationStructureOpsArray> asAlg(asOps, scratch, barrier, gpuWorkId);
    asAlg.completeNeeded();
  }
#endif

  if (!barrier.empty())
    barrier.submit(cmd_buffer);
}

void ExecutionSyncTracker::clearOps()
{
  bufOps.clear();
  imgOps.clear();
#if D3D_HAS_RAY_TRACING
  asOps.clear();
#endif
}


void ExecutionSyncTracker::completeAll(VulkanCommandBufferHandle cmd_buffer, const VulkanDevice &dev, size_t gpu_work_id)
{
  completeNeeded(cmd_buffer, dev);

  // ending current block, so increment in advance
  gpuWorkId = gpu_work_id + 1;

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
      logerr("vulkan: sync: image: incompleted write while registered in bindless, must handle it! %s", op.format());

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

  InternalPipelineBarrier barrier(dev, srcLA.stage, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
  barrier.addMemory({srcLA.access, VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_MEMORY_READ_BIT});
  barrier.submit(cmd_buffer);

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

ExecutionSyncTracker::OpCaller ExecutionSyncTracker::getCaller() { return {backtrace::get_hash(1), 0}; }

String ExecutionSyncTracker::OpCaller::getInternal() const { return backtrace::get_stack_by_hash(internal); }

#endif

#if D3D_HAS_RAY_TRACING

void ExecutionSyncTracker::addAccelerationStructureAccess(LogicAddress laddr, RaytraceAccelerationStructure *as)
{
  AccelerationStructureArea area;
  if (filterAccessTracking(gpuWorkId, asOps, as, laddr, area, 0))
    return;
  asOps.arr.push_back({laddr, as, area, getCaller(), /*completed*/ false, /*dstConflict*/ false});
}

#endif