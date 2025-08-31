// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// class to track operations on GPU execution timeline and sync them
// according to supplied information & requests
// generates precise (stage & acccess) barriers for less overhead on synchronization

#include "driver.h"
#include "vulkan_device.h"
#include "logic_address.h"

namespace drv3d_vulkan
{

class Buffer;
class Image;
class RaytraceAccelerationStructure;
class PipelineBarrier;
class Resource;
class ExecutionSyncTracker;

#if EXECUTION_SYNC_TRACK_CALLER > 0
struct SyncOpCaller
{
  uint64_t internal;
  uint64_t external;
  String getInternal() const;
  String getExternal() const;
};
#else
struct SyncOpCaller
{
  static String getInternal() { return String("<unknown>"); }
  static String getExternal() { return String("<unknown>"); }
};
#endif

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
// Op inside array are sorted/moved/swapped, so we need to track originals by some uid/index
struct SyncOpUid
{
  uint32_t v;
  static uint32_t frame_local_next_op_uid;

  static SyncOpUid next() { return {frame_local_next_op_uid++}; }

  static void frame_end() { frame_local_next_op_uid = 0; }
};
#else
struct SyncOpUid
{
  static SyncOpUid next() { return {}; }

  static void frame_end() {}
};
#endif

struct BufferArea
{
  VkDeviceSize offset;
  VkDeviceSize size;

  bool intersects(const BufferArea &cmp) const
  {
    if ((offset + size <= cmp.offset) || (cmp.offset + cmp.size <= offset))
      return false;
    return true;
  }

  bool mergable(const BufferArea &cmp) const
  {
    if ((offset + size < cmp.offset) || (cmp.offset + cmp.size < offset))
      return false;
    return true;
  }

  void merge(const BufferArea &cmp)
  {
    size = max((offset + size), (cmp.offset + cmp.size));
    offset = min(offset, cmp.offset);
    size -= offset;
  }
};

struct ImageArea
{
  uint32_t mipIndex;
  uint32_t mipRange;
  uint32_t arrayIndex;
  uint32_t arrayRange;

  bool intersects(const ImageArea &cmp) const
  {
    if ((mipIndex + mipRange <= cmp.mipIndex) || (cmp.mipIndex + cmp.mipRange <= mipIndex))
      return false;
    if ((arrayIndex + arrayRange <= cmp.arrayIndex) || (cmp.arrayIndex + cmp.arrayRange <= arrayIndex))
      return false;
    return true;
  }

  bool mergable(const ImageArea &cmp) const
  {
    if ((mipIndex + mipRange < cmp.mipIndex) || (cmp.mipIndex + cmp.mipRange < mipIndex))
      return false;
    if ((arrayIndex + arrayRange < cmp.arrayIndex) || (cmp.arrayIndex + cmp.arrayRange < arrayIndex))
      return false;

    uint32_t mipEnd = max((mipIndex + mipRange), (cmp.mipIndex + cmp.mipRange));
    uint32_t mipStart = min(mipIndex, cmp.mipIndex);
    uint32_t arrayEnd = max((arrayIndex + arrayRange), (cmp.arrayIndex + cmp.arrayRange));
    uint32_t arrayStart = min(arrayIndex, cmp.arrayIndex);

    // merge should not cover areas that was not included in both operands
    // i.e. we can extend area by one of dimensions or grow to bigger one if they nested
    bool mipExt = (arrayIndex == cmp.arrayIndex) && (arrayRange == cmp.arrayRange);
    bool arrExt = (mipIndex == cmp.mipIndex) && (mipRange == cmp.mipRange);
    bool inside =
      (mipStart == mipIndex) && (mipEnd == mipIndex + mipRange) && (arrayStart == arrayIndex) && (arrayEnd == arrayIndex + arrayRange);

    return mipExt || arrExt || inside;
  }

  void merge(const ImageArea &cmp)
  {
    uint32_t mipEnd = max((mipIndex + mipRange), (cmp.mipIndex + cmp.mipRange));
    uint32_t mipStart = min(mipIndex, cmp.mipIndex);
    uint32_t arrayEnd = max((arrayIndex + arrayRange), (cmp.arrayIndex + cmp.arrayRange));
    uint32_t arrayStart = min(arrayIndex, cmp.arrayIndex);

    mipIndex = mipStart;
    mipRange = mipEnd - mipStart;
    arrayIndex = arrayStart;
    arrayRange = arrayEnd - arrayStart;
  }
};

template <typename ObjType, typename AreaType, typename AdditionalSyncParamsType, typename ExtraContentType>
struct SyncOp : ExtraContentType
{
  typedef SyncOp<ObjType, AreaType, AdditionalSyncParamsType, ExtraContentType> ThisType;
  typedef ObjType ResType;
  typedef AdditionalSyncParamsType AdditionalParams;

  bool completed : 1;
  bool dstConflict : 1;
  SyncOpUid uid;
  SyncOpCaller caller;
  LogicAddress laddr;
  LogicAddress conflictingLAddr;
  ObjType *obj;
  AreaType area;

  String format() const;
  bool conflicts(const ThisType &cmp) const;
  bool verifySelfConflict(const ThisType &cmp) const;
  void addToBarrierByTemplateSrc(PipelineBarrier &barrier);
  void addToBarrierByTemplateDst(PipelineBarrier &barrier);
  void onConflictWithDst(ThisType &dst);
  static bool allowsConflictFromObject();
  bool hasObjConflict();
  bool mergeCheck(AreaType area, AdditionalSyncParamsType extra);
  bool isAreaPartiallyCoveredBy(const ThisType &op);
  static bool processPartials();
  void onPartialSplit();
  // for followup alias barrier generation
  void aliasEndAccess(VkPipelineStageFlags stage, ExecutionSyncTracker &tracker);

  SyncOp(SyncOpUid _uid, SyncOpCaller _caller, LogicAddress _laddr, ObjType *_obj, AreaType _area) :
    completed(0), dstConflict(0), uid(_uid), caller(_caller), laddr(_laddr), conflictingLAddr({}), obj(_obj), area(_area)
  {}
};

struct EmptySyncOpSpecificStorage
{};

typedef SyncOp<Buffer, BufferArea, uint32_t, EmptySyncOpSpecificStorage> BufferSyncOp;

struct ImageSyncOpSpecificStorage
{
  bool changesLayout : 1;
  bool nrpAttachment : 1;
  bool handledBySubpassDependency : 1;
  bool discard : 1;
  uint8_t subpassIdx;
  uint16_t nativeRPIndex;
  VkImageLayout layout;

  // NRP internal subpass dependency should be suppressed on some conflicts
  bool shouldSuppress() const
  {
    // We suppress barriers if:
    // we have a pair of OPS happening inside the same NRP, in which
    // case the barrier is handled by the subpass dependency
    return handledBySubpassDependency;
  }

  void specificInit(bool _nrpAttachment, VkImageLayout _layout, uint8_t _subpassIdx, uint16_t _nativeRPIndex, bool _discard)
  {
    changesLayout = false;
    handledBySubpassDependency = false;
    nrpAttachment = _nrpAttachment;
    discard = _discard;
    subpassIdx = _subpassIdx;
    nativeRPIndex = _nativeRPIndex;
    layout = _layout;
  }
};

struct ImageSyncOpAdditionalParams
{
  VkImageLayout layout;
  uint8_t subpassIdx;
};

typedef SyncOp<Image, ImageArea, ImageSyncOpAdditionalParams, ImageSyncOpSpecificStorage> ImageSyncOp;

#if VULKAN_HAS_RAYTRACING
struct AccelerationStructureArea
{
  bool intersects(const AccelerationStructureArea &) const { return true; }
  bool mergable(const AccelerationStructureArea &) const { return true; }

  void merge(const AccelerationStructureArea &) {}
};

typedef SyncOp<RaytraceAccelerationStructure, AccelerationStructureArea, uint32_t, EmptySyncOpSpecificStorage>
  AccelerationStructureSyncOp;

#endif

template <typename SyncObjType>
struct SyncOpsArrayBase
{
  size_t lastProcessed = 0;
  size_t lastIncompleted = 0;

  dag::Vector<SyncObjType> arr;
  void clear()
  {
    arr.clear();
    lastProcessed = 0;
    lastIncompleted = 0;
  }
  static bool isRoSealValidForOperation(typename SyncObjType::ResType *obj, typename SyncObjType::AdditionalParams params);
  void removeRoSeal(typename SyncObjType::ResType *obj);
};

class ExecutionSyncTracker
{
public:
  ExecutionSyncTracker() {}
  ~ExecutionSyncTracker() {}

#if EXECUTION_SYNC_TRACK_CALLER > 0
  SyncOpCaller getCaller();
#else
  SyncOpCaller getCaller() { return {}; }
#endif


  static inline constexpr uint8_t SUBPASS_NON_NATIVE = 255;
  static inline constexpr uint16_t NATIVE_RP_INDEX_MAX = 0xFFFF;

  typedef SyncOpsArrayBase<ImageSyncOp> ImageOpsArray;
  typedef SyncOpsArrayBase<BufferSyncOp> BufferOpsArray;

// raytrace ext
#if VULKAN_HAS_RAYTRACING
  typedef SyncOpsArrayBase<AccelerationStructureSyncOp> AccelerationStructureOpsArray;
  void addAccelerationStructureAccess(LogicAddress laddr, RaytraceAccelerationStructure *as);

private:
  AccelerationStructureOpsArray asOps;

public:
#endif

  struct AreaCoverageMap
  {
    dag::Vector<bool> bits;
    uint32_t x0 = 0, x1 = 0;
    uint32_t y0 = 0, y1 = 0;
    uint32_t xsz = 0;
    uint32_t ysz = 0;
    uint32_t processedLine = 0;

    void init(ImageArea area);
    void exclude(ImageArea area);
    bool getArea(ImageArea &area);

    void init(BufferArea) {}
    void exclude(BufferArea) {}
    bool getArea(BufferArea &) { return false; }
#if VULKAN_HAS_RAYTRACING
    void init(AccelerationStructureArea) {}
    void exclude(AccelerationStructureArea) {}
    bool getArea(AccelerationStructureArea &) { return false; }
#endif
    void clear() { bits.clear(); }
  };

  struct ScratchData
  {
    dag::Vector<size_t> src;
    dag::Vector<size_t> dst;
    dag::Vector<size_t> partialCoveringDsts;
    AreaCoverageMap coverageMap;

    void clear();
  };

  void addBufferAccess(LogicAddress laddr, Buffer *buf, BufferArea area);
  void addImageWriteDiscard(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area)
  {
    addImageAccessImpl(laddr, img, layout, area, /* nrp_attachment */ true, /* discard */ true);
  }

  void addImageAccess(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area)
  {
    addImageAccessImpl(laddr, img, layout, area, /* nrp_attachment */ false, /* discard */ false);
  }
  void addNrpAttachmentAccess(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area, bool discard)
  {
    addImageAccessImpl(laddr, img, layout, area, /* nrp_attachment */ true, discard);
  }

  void setCurrentRenderSubpass(uint8_t subpass_idx);
  void setCompletionDelay(bool val)
  {
    G_ASSERTF(val ^ delayCompletion, "vulkan: sync delay range can't nest! make sure user code is correct");
    delayCompletion = val;
  }
  bool isCompletionDelayed() { return delayCompletion; }

  void completeNeeded();
  void completeAll(size_t gpu_work_id);
  void completeOnQueue(size_t gpu_work_id);
  bool anyNonProcessed();
  bool allCompleted();
  void aliasSync(Resource *lobj, VkPipelineStageFlags stage);

private:
  void workItemEndSync(size_t gpu_work_id);
  void addImageAccessImpl(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area, bool nrp_attachment, bool discard);

  void clearOps();
  ScratchData scratch;
  ImageOpsArray imgOps;
  BufferOpsArray bufOps;
  uint16_t nativeRPIndex = 0;
  uint8_t currentRenderSubpass = SUBPASS_NON_NATIVE;
  size_t gpuWorkId = 0;
  bool delayCompletion = false;
};

} // namespace drv3d_vulkan
