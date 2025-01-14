// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// class to track operations on GPU execution timeline and sync them
// according to supplied information & requests
// generates precise (stage & acccess) barriers for less overhead on synchronization

#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING
#include <EASTL/vector.h>

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

class ExecutionSyncTracker
{
public:
  ExecutionSyncTracker() {}
  ~ExecutionSyncTracker() {}

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
      bool inside = (mipStart == mipIndex) && (mipEnd == mipIndex + mipRange) && (arrayStart == arrayIndex) &&
                    (arrayEnd == arrayIndex + arrayRange);

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

#if EXECUTION_SYNC_TRACK_CALLER > 0
  struct OpCaller
  {
    uint64_t internal;
    uint64_t external;
    String getInternal() const;
    String getExternal() const;
  };
  OpCaller getCaller();
#else
  struct OpCaller
  {
    static String getInternal() { return String("<unknown>"); }
    static String getExternal() { return String("<unknown>"); }
  };
  OpCaller getCaller() { return {}; }
#endif

#if EXECUTION_SYNC_DEBUG_CAPTURE > 0
  // Op inside array are sorted/moved/swapped, so we need to track originals by some uid/index
  struct OpUid
  {
    uint32_t v;
    static uint32_t frame_local_next_op_uid;

    static OpUid next() { return {frame_local_next_op_uid++}; }

    static void frame_end() { frame_local_next_op_uid = 0; }
  };
#else
  struct OpUid
  {
    static OpUid next() { return {}; }

    static void frame_end() {}
  };
#endif

  struct BufferOp
  {
    OpUid uid;
    LogicAddress laddr;
    Buffer *obj;
    BufferArea area;
    OpCaller caller;
    VkAccessFlags conflictingAccessFlags;
    bool completed : 1;
    bool dstConflict : 1;

    String format() const;
    bool conflicts(const BufferOp &cmp) const;
    bool verifySelfConflict(const BufferOp &cmp) const;
    void addToBarrierByTemplateSrc(PipelineBarrier &barrier) const;
    void addToBarrierByTemplateDst(PipelineBarrier &barrier);
    void onConflictWithDst(BufferOp &dst);
    static bool allowsConflictFromObject() { return false; }
    bool hasObjConflict() { return false; }
    bool mergeCheck(BufferArea, uint32_t) { return true; }
    bool isAreaPartiallyCoveredBy(const BufferOp &) { return false; }
    static bool processPartials() { return false; }
    void onPartialSplit() {}
    // for followup alias barrier generation
    void aliasEndAccess(VkPipelineStageFlags stage, ExecutionSyncTracker &tracker);
  };

  static inline constexpr uint8_t SUBPASS_NON_NATIVE = 255;
  static inline constexpr uint16_t NATIVE_RP_INDEX_MAX = 0xFFFF;

  struct ImageOpAdditionalParams
  {
    VkImageLayout layout;
    uint8_t subpassIdx;
  };

  struct ImageOp
  {
    OpUid uid;
    LogicAddress laddr;
    Image *obj;
    ImageArea area;
    OpCaller caller;
    VkAccessFlags conflictingAccessFlags;
    VkImageLayout layout;
    uint8_t subpassIdx;
    uint16_t nativeRPIndex;
    bool completed : 1;
    bool dstConflict : 1;
    bool changesLayout : 1;
    bool nrpAttachment : 1;
    bool handledBySubpassDependency : 1;
    bool discard;

    String format() const;
    bool conflicts(const ImageOp &cmp) const;
    bool verifySelfConflict(const ImageOp &cmp) const;
    void addToBarrierByTemplateSrc(PipelineBarrier &barrier);
    void addToBarrierByTemplateDst(PipelineBarrier &barrier);
    void onConflictWithDst(ImageOp &dst);
    static bool allowsConflictFromObject() { return true; }
    bool hasObjConflict();
    bool mergeCheck(ImageArea area, ImageOpAdditionalParams extra);
    bool isAreaPartiallyCoveredBy(const ImageOp &dst);
    static bool processPartials() { return true; }
    void onPartialSplit();
    // for followup alias barrier generation
    void aliasEndAccess(VkPipelineStageFlags stage, ExecutionSyncTracker &tracker);

    // resets fields that are used at complete step and need to be cleared for next such step
    void resetIntermediateTracking();
    // NRP internal subpass dependency should be suppressed on some conflicts
    bool shouldSuppress() const;
    // applies layout change to object and does related work for op obj
    void processLayoutChange();
  };

  struct OpsArrayBase
  {
    size_t lastProcessed = 0;
    size_t lastIncompleted = 0;
    void clearBase()
    {
      lastProcessed = 0;
      lastIncompleted = 0;
    }
  };

  struct ImageOpsArray : OpsArrayBase
  {
    eastl::vector<ImageOp> arr;
    void clear()
    {
      arr.clear();
      clearBase();
    }
    static bool isRoSealValidForOperation(Image *obj, ImageOpAdditionalParams params);
    void removeRoSeal(Image *obj);
  };

  struct BufferOpsArray : OpsArrayBase
  {
    eastl::vector<BufferOp> arr;
    void clear()
    {
      arr.clear();
      clearBase();
    }
    static bool isRoSealValidForOperation(Buffer *, uint32_t) { return true; }
    void removeRoSeal(Buffer *obj);
  };

// raytrace ext
#if D3D_HAS_RAY_TRACING
  struct AccelerationStructureArea
  {
    bool intersects(const AccelerationStructureArea &) const { return true; }

    bool mergable(const AccelerationStructureArea &) const { return true; }

    void merge(const AccelerationStructureArea &) {}
  };

  struct AccelerationStructureOp
  {
    OpUid uid;
    LogicAddress laddr;
    RaytraceAccelerationStructure *obj;
    AccelerationStructureArea area;
    OpCaller caller;
    VkAccessFlags conflictingAccessFlags;
    bool completed : 1;
    bool dstConflict : 1;

    String format() const;
    bool conflicts(const AccelerationStructureOp &cmp) const;
    bool verifySelfConflict(const AccelerationStructureOp &cmp) const { return !conflicts(cmp); }
    void addToBarrierByTemplateSrc(PipelineBarrier &barrier) const;
    void addToBarrierByTemplateDst(PipelineBarrier &barrier) const;
    void onConflictWithDst(const AccelerationStructureOp &dst);
    static bool allowsConflictFromObject() { return false; }
    bool hasObjConflict() { return false; }
    bool mergeCheck(AccelerationStructureArea, uint32_t) { return true; }
    bool isAreaPartiallyCoveredBy(const AccelerationStructureOp &) { return false; }
    static bool processPartials() { return false; }
    void onPartialSplit() {}
  };

  struct AccelerationStructureOpsArray : OpsArrayBase
  {
    eastl::vector<AccelerationStructureOp> arr;
    void clear()
    {
      arr.clear();
      clearBase();
    }
    static bool isRoSealValidForOperation(RaytraceAccelerationStructure *, uint32_t) { return true; }
    void removeRoSeal(RaytraceAccelerationStructure *obj);
  };

  void addAccelerationStructureAccess(LogicAddress laddr, RaytraceAccelerationStructure *as);

private:
  AccelerationStructureOpsArray asOps;

public:
#endif

  struct AreaCoverageMap
  {
    eastl::vector<bool> bits;
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
#if D3D_HAS_RAY_TRACING
    void init(AccelerationStructureArea) {}
    void exclude(AccelerationStructureArea) {}
    bool getArea(AccelerationStructureArea &) { return false; }
#endif
    void clear() { bits.clear(); }
  };

  struct ScratchData
  {
    eastl::vector<size_t> src;
    eastl::vector<size_t> dst;
    eastl::vector<size_t> partialCoveringDsts;
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
  void workItemEndBarrier(size_t gpu_work_id);
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
