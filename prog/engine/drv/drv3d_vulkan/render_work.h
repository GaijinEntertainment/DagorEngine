// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// replay work that is executed in worker thread

#include <EASTL/array.h>
#include <EASTL/stack.h>
#include <EASTL/string.h>
#include <fsr_args.h>

#include "query_pools.h"
#include "queries.h"
#include "device_context_cmd.h"
#include "util/variant_vector.h"
#include "cleanup_queue.h"
#include "util/fault_report.h"
#include "globals.h"
#include "driver_config.h"
#include "copy_info.h"
#include "resource_readbacks.h"

namespace drv3d_vulkan
{

struct ReorderedBufferCopy
{
  BufferRef src;
  BufferRef dst;
  uint32_t srcOffset;
  uint32_t dstOffset;
  uint32_t size;
  uint32_t pass;
};

struct BindlessTexSwap
{
  Image *src;
  Image *dst;
  ImageViewState viewState;
};

enum BindlessUpdateType
{
  RES,
  COPY
};

// constructor is not implicitly called - intended logic
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4582)
#endif

struct BindlessTexUpdateInfo
{
  BindlessTexUpdateInfo(uint32_t _index, Image *img, ImageViewState view, bool is_stub, bool is_stub_swap) :
    index(_index), count(1), type(BindlessUpdateType::RES), stub(is_stub), stubSwap(is_stub_swap)
  {
    variant.res = {img, view};
  }
  BindlessTexUpdateInfo(uint32_t src, uint32_t dst, uint32_t _count) :
    index(dst), count(_count), type(BindlessUpdateType::COPY), stub(false), stubSwap(false)
  {
    variant.copy.src = src;
  }
  BindlessTexUpdateInfo(uint32_t _index, uint32_t _count) :
    index(_index), count(_count), type(BindlessUpdateType::RES), stub(false), stubSwap(false)
  {
    variant.res = {nullptr, {}};
  }

  uint32_t index;
  uint32_t count;
  BindlessUpdateType type;
  bool stub : 1;
  bool stubSwap : 1;
  union Variants
  {
    struct
    {
      Image *img;
      ImageViewState viewState;
    } res;
    struct
    {
      uint32_t src;
    } copy;

    Variants() {}
  } variant;
};


struct BindlessBufUpdateInfo
{
  BindlessBufUpdateInfo(uint32_t _index, const BufferRef &bref) : index(_index), count(1), type(BindlessUpdateType::RES)
  {
    variant.res.bref = bref;
  }
  BindlessBufUpdateInfo(uint32_t src, uint32_t dst, uint32_t _count) : index(dst), count(_count), type(BindlessUpdateType::COPY)
  {
    variant.copy.src = src;
  }
  BindlessBufUpdateInfo(uint32_t _index, uint32_t _count) : index(_index), count(_count), type(BindlessUpdateType::RES)
  {
    variant.res.bref = {};
  }

  uint32_t index;
  uint32_t count;
  BindlessUpdateType type;
  union Variants
  {
    struct
    {
      BufferRef bref;
    } res;
    struct
    {
      uint32_t src;
    } copy;

    Variants() {}
  } variant;
};

#ifdef _MSC_VER
// C4582 off
#pragma warning(pop)
#endif

struct BindlessSamplerUpdateInfo
{
  uint32_t index;
  const SamplerResource *sampler;
};

struct RaytraceBLASBufferRefs
{
  Buffer *geometry;
  uint32_t geometryOffset;
  uint32_t geometrySize;

  Buffer *index;
  uint32_t indexOffset;
  uint32_t indexSize;

  Buffer *transform;
  uint32_t transformOffset;
};

#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query) || 1
struct RaytraceStructureBuildData
{
  VkAccelerationStructureTypeKHR type;
  VkBuildAccelerationStructureFlagsKHR flags;
  VkBool32 update;
  RaytraceAccelerationStructure *dst;
  BufferRef scratchBuf;
  union
  {
    struct
    {
      uint32_t instanceCount;
      BufferRef instanceBuffer;
    } tlas;
    struct
    {
      uint32_t geometryCount;
      uint32_t firstGeometry;
      BufferRef compactionSizeBuffer;
    } blas;
  };
};
#endif

class ExecutionContext;
struct RenderWork
{
  static bool cleanUpMemoryEveryWorkItem;

  size_t id = 0;
  uint32_t frontFrameIndex = 0;
  uint32_t userSignalCount = 0;

  dag::Vector<BufferCopyInfo> bufferUploads;
  dag::Vector<VkBufferCopy> bufferUploadCopies;

  dag::Vector<BufferCopyInfo> orderedBufferUploads;
  dag::Vector<VkBufferCopy> orderedBufferUploadCopies;
  dag::Vector<ReorderedBufferCopy> reorderedBufferCopies;

  // flushed into a separate cmd buffer executed before the graphics/compute cmd buffer
  dag::Vector<ImageCopyInfo> imageUploads;
  dag::Vector<VkBufferImageCopy> imageUploadCopies;
  dag::Vector<char> charStore;
  dag::Vector<VkImageCopy> imageCopyInfos;
  dag::Vector<CmdCopyImage> unorderedImageCopies;
  dag::Vector<CmdClearColorTexture> unorderedImageColorClears;
  dag::Vector<CmdClearDepthStencilTexture> unorderedImageDepthStencilClears;
  dag::Vector<BindlessTexUpdateInfo> bindlessTexUpdates;
  dag::Vector<BindlessTexSwap> bindlessTexSwaps;
  dag::Vector<BindlessBufUpdateInfo> bindlessBufUpdates;
  dag::Vector<BindlessSamplerUpdateInfo> bindlessSamplerUpdates;
  dag::Vector<uint32_t> nativeRPDrawCounter;
  dag::Vector<VkSparseMemoryBind> sparseMemoryBinds;
  dag::Vector<VkSparseImageMemoryBind> sparseImageMemoryBinds;
  dag::Vector<VkSparseImageOpaqueMemoryBindInfo> sparseImageOpaqueBinds;
  dag::Vector<VkSparseImageMemoryBindInfo> sparseImageBinds;

#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  dag::Vector<VkAccelerationStructureBuildRangeInfoKHR> raytraceBuildRangeInfoKHRStore;
  dag::Vector<VkAccelerationStructureGeometryKHR> raytraceGeometryKHRStore;
  dag::Vector<RaytraceBLASBufferRefs> raytraceBLASBufferRefsStore;
  dag::Vector<RaytraceStructureBuildData> raytraceStructureBuildStore;
#endif
  dag::Vector<ShaderModuleUse> shaderModuleUses;
  AnyCommandStore commandStream;

  QueryBlock *timestampQueryBlock = nullptr;
  QueryBlock *occlusionQueryBlock = nullptr;
  BatchedReadbacks *readbacks = nullptr;
  BatchedReadbacks *nextReadbacks = nullptr;
  BatchedReadbacks *prevReadbacks = nullptr;

  bool generateFaultReport = false;

  size_t getMemorySize() const
  {
    size_t size = 0;
#define CALC_VEC_BYTES(Name) (Name.capacity() * sizeof(decltype(Name)::value_type))
    size += CALC_VEC_BYTES(bufferUploads);
    size += CALC_VEC_BYTES(bufferUploadCopies);
    size += CALC_VEC_BYTES(orderedBufferUploads);
    size += CALC_VEC_BYTES(orderedBufferUploadCopies);
    size += CALC_VEC_BYTES(imageUploads);
    size += CALC_VEC_BYTES(imageUploadCopies);
    size += CALC_VEC_BYTES(charStore);
    size += CALC_VEC_BYTES(imageCopyInfos);
    size += CALC_VEC_BYTES(unorderedImageCopies);
    size += CALC_VEC_BYTES(unorderedImageColorClears);
    size += CALC_VEC_BYTES(unorderedImageDepthStencilClears);
    size += CALC_VEC_BYTES(bindlessTexUpdates);
    size += CALC_VEC_BYTES(bindlessTexSwaps);
    size += CALC_VEC_BYTES(bindlessBufUpdates);
    size += CALC_VEC_BYTES(bindlessSamplerUpdates);
    size += CALC_VEC_BYTES(nativeRPDrawCounter);
    size += CALC_VEC_BYTES(reorderedBufferCopies);
    size += CALC_VEC_BYTES(sparseMemoryBinds);
    size += CALC_VEC_BYTES(sparseImageMemoryBinds);
    size += CALC_VEC_BYTES(sparseImageOpaqueBinds);
    size += CALC_VEC_BYTES(sparseImageBinds);
#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
    size += CALC_VEC_BYTES(raytraceBuildRangeInfoKHRStore);
    size += CALC_VEC_BYTES(raytraceGeometryKHRStore);
    size += CALC_VEC_BYTES(raytraceBLASBufferRefsStore);
    size += CALC_VEC_BYTES(raytraceStructureBuildStore);
#endif
    size += CALC_VEC_BYTES(shaderModuleUses);
#undef CALC_VEC_BYTES
    size += commandStream.capacity();
    return size;
  }

  template <typename T>
  void pushCommand(const T &cmd)
  {
    commandStream.push_back(cmd);
    recordCommandCaller();
  }

#if DAGOR_DBGLEVEL > 0
  dag::Vector<uint64_t> commandCallers;
  void recordCommandCaller()
  {
    if (Globals::cfg.bits.recordCommandCaller)
      commandCallers.push_back(backtrace::get_hash(1));
  }
#else
  void recordCommandCaller() {}
#endif
  void dumpCommands(FaultReportDump &dump);
  void dumpData(FaultReportDump &dump) const;
  RenderWork() {}

  // timeline methods

  void submit();
  void acquire(size_t timeline_abs_idx);
  void wait();
  void cleanup();
  void process();
  void shutdown();

  void processCommands(ExecutionContext &ctx);
};

} // namespace drv3d_vulkan