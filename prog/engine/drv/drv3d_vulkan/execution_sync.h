// Copyright 2023 by Gaijin Games KFT, All rights reserved.

// class to track operations on GPU execution timeline and sync them
// according to supplied information & requests
// generates precise (stage & acccess) barriers for less overhead on synchronization

#pragma once
#include "driver.h"
#include "vulkan_device.h"
#include <EASTL/vector.h>
#include <ska_hash_map/flat_hash_map2.hpp>

namespace drv3d_vulkan
{

class Buffer;
class Image;
class RaytraceAccelerationStructure;
class PipelineBarrier;

class ExecutionSyncTracker
{
public:
  ExecutionSyncTracker() {}
  ~ExecutionSyncTracker() {}

  struct LogicAddress
  {
    VkPipelineStageFlags stage;
    VkAccessFlags access;

    bool isNonConflictingUAVAccess(const LogicAddress &cmp) const;
    bool conflicting(const LogicAddress &cmp) const;
    bool mergeConflicting(const LogicAddress &cmp) const;
    bool isRead() const;
    bool isWrite() const;
    void merge(const LogicAddress &v)
    {
      stage |= v.stage;
      access |= v.access;
    }

    static LogicAddress forBufferOnExecStage(ShaderStage stage, RegisterType reg_type);
    static LogicAddress forAttachmentWithLayout(VkImageLayout layout);
    static LogicAddress forImageOnExecStage(ShaderStage stage, RegisterType reg_type);
    static LogicAddress forAccelerationStructureOnExecStage(ShaderStage stage, RegisterType reg_type);
    static void setAttachmentNoStoreSupport(bool supported);
  };

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
  };
  OpCaller getCaller();
#else
  struct OpCaller
  {
    static String getInternal() { return String("<unknown>"); }
  };
  OpCaller getCaller() { return {}; }
#endif

  struct BufferOp
  {
    LogicAddress laddr;
    Buffer *obj;
    BufferArea area;
    OpCaller caller;
    bool completed : 1;
    bool dstConflict : 1;

    String format() const;
    bool conflicts(const BufferOp &cmp) const;
    bool verifySelfConflict(const BufferOp &cmp) const { return !conflicts(cmp) || laddr.isNonConflictingUAVAccess(cmp.laddr); }
    void addToBarrierByTemplateSrc(PipelineBarrier &barrier) const;
    void addToBarrierByTemplateDst(PipelineBarrier &, size_t) const {}
    static void modifyBarrierTemplate(PipelineBarrier &barrier, LogicAddress &src, LogicAddress &dst);
    void onConflictWithDst(const BufferOp &dst, size_t gpu_work_id);
    static bool allowsConflictFromObject() { return false; }
    bool hasObjConflict() { return false; }
    bool mergeCheck(BufferArea, uint32_t) { return true; }
    bool isAreaPartiallyCoveredBy(const BufferOp &) { return false; }
    static bool processPartials() { return false; }
  };

  struct ImageOpAdditionalParams
  {
    VkImageLayout layout;
    uint16_t nativePassIdx;
  };

  struct ImageOp
  {
    LogicAddress laddr;
    Image *obj;
    ImageArea area;
    OpCaller caller;
    VkImageLayout layout;
    uint16_t nativePassIdx;
    bool completed : 1;
    bool dstConflict : 1;
    bool changesLayout : 1;

    String format() const;
    bool conflicts(const ImageOp &cmp) const;
    bool verifySelfConflict(const ImageOp &cmp) const;
    void addToBarrierByTemplateSrc(PipelineBarrier &barrier);
    void addToBarrierByTemplateDst(PipelineBarrier &barrier, size_t gpu_work_id);
    static void modifyBarrierTemplate(PipelineBarrier &barrier, LogicAddress &src, LogicAddress &dst);
    void onConflictWithDst(ImageOp &dst, size_t gpu_work_id);
    static bool allowsConflictFromObject() { return true; }
    bool hasObjConflict();
    void onNativePassEnter(uint16_t pass_idx);
    bool mergeCheck(ImageArea area, ImageOpAdditionalParams extra);
    bool isAreaPartiallyCoveredBy(const ImageOp &dst);
    static bool processPartials() { return true; }
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
    LogicAddress laddr;
    RaytraceAccelerationStructure *obj;
    AccelerationStructureArea area;
    OpCaller caller;
    bool completed : 1;
    bool dstConflict : 1;

    String format() const;
    bool conflicts(const AccelerationStructureOp &cmp) const;
    bool verifySelfConflict(const AccelerationStructureOp &cmp) const { return !conflicts(cmp); }
    void addToBarrierByTemplateSrc(PipelineBarrier &) const {}
    void addToBarrierByTemplateDst(PipelineBarrier &, size_t) const {}
    static void modifyBarrierTemplate(PipelineBarrier &barrier, LogicAddress &src, LogicAddress &dst);
    void onConflictWithDst(const AccelerationStructureOp &dst, size_t gpu_work_id);
    static bool allowsConflictFromObject() { return false; }
    bool hasObjConflict() { return false; }
    bool mergeCheck(AccelerationStructureArea, uint32_t) { return true; }
    bool isAreaPartiallyCoveredBy(const AccelerationStructureOp &) { return false; }
    static bool processPartials() { return false; }
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
    struct DstObjInfo
    {
      size_t start;
      size_t end;
    };

    ska::flat_hash_map<void *, DstObjInfo> objSorted;
    eastl::vector<size_t> src;
    eastl::vector<size_t> dst;
    eastl::vector<size_t> partialCoveringDsts;
    AreaCoverageMap coverageMap;

    void clear();
  };

  void addBufferAccess(LogicAddress laddr, Buffer *buf, BufferArea area);
  void addImageAccess(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area);
  void addImageAccessAtNativePass(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area);
  void enterNativePass();

  void completeNeeded(VulkanCommandBufferHandle cmd_buffer, const VulkanDevice &dev);
  void completeAll(VulkanCommandBufferHandle cmd_buffer, const VulkanDevice &dev, size_t gpu_work_id);
  bool anyNonProcessed();
  bool allCompleted();

private:
  void addImageAccessFull(LogicAddress laddr, Image *img, VkImageLayout layout, ImageArea area, uint16_t native_pass_idx);
  void clearOps();
  ScratchData scratch;
  ImageOpsArray imgOps;
  BufferOpsArray bufOps;
  uint16_t nativePassIdx = 1;
  size_t gpuWorkId = 0;
};

} // namespace drv3d_vulkan
