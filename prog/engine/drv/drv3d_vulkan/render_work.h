// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

// replay work that is executed in worker thread

#include <EASTL/array.h>
#include <EASTL/stack.h>
#include <EASTL/string.h>
#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING
#include <fsr_args.h>

#include "query_pools.h"
#include "timestamp_queries.h"
#include "device_context_cmd.h"
#include "util/variant_vector.h"
#include "cleanup_queue.h"
#include "util/fault_report.h"
#include "globals.h"
#include "driver_config.h"

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

struct BufferFlushInfo
{
  Buffer *buffer = nullptr;
  uint32_t offset = 0;
  uint32_t range = 0;
};

struct BufferCopyInfo
{
  Buffer *src;
  Buffer *dst;
  uint32_t copyIndex;
  uint32_t copyCount;

  static void optimizeBufferCopies(eastl::vector<BufferCopyInfo> &info, eastl::vector<VkBufferCopy> &copies);
};

struct ImageCopyInfo
{
  Image *image;
  Buffer *buffer;
  uint32_t copyIndex;
  uint32_t copyCount;

  static void deduplicate(eastl::vector<ImageCopyInfo> &info, eastl::vector<VkBufferImageCopy> &copies);
};

struct BindlessTexUpdateInfo
{
  uint32_t index;
  uint32_t count;
  Image *img;
  ImageViewState viewState;
};

struct BindlessBufUpdateInfo
{
  uint32_t index;
  uint32_t count;
  BufferRef bref;
};

struct BindlessSamplerUpdateInfo
{
  uint32_t index;
  SamplerState sampler;
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

#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query) || 1
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

  CleanupQueue cleanups;

  size_t id = 0;
  uint32_t frontFrameIndex = 0;
  uint32_t userSignalCount = 0;

  eastl::vector<BufferCopyInfo> bufferUploads;
  eastl::vector<VkBufferCopy> bufferUploadCopies;

  eastl::vector<BufferCopyInfo> orderedBufferUploads;
  eastl::vector<VkBufferCopy> orderedBufferUploadCopies;
  eastl::vector<ReorderedBufferCopy> reorderedBufferCopies;

  eastl::vector<BufferCopyInfo> bufferDownloads;
  eastl::vector<VkBufferCopy> bufferDownloadCopies;
  eastl::vector<BufferFlushInfo> bufferToHostFlushes;
  // flushed into a separate cmd buffer executed before the graphics/compute cmd buffer
  eastl::vector<ImageCopyInfo> imageUploads;
  eastl::vector<VkBufferImageCopy> imageUploadCopies;
  eastl::vector<ImageCopyInfo> imageDownloads;
  eastl::vector<VkBufferImageCopy> imageDownloadCopies;
  eastl::vector<char> charStore;
  eastl::vector<VkImageCopy> imageCopyInfos;
  eastl::vector<CmdCopyImage> unorderedImageCopies;
  eastl::vector<CmdClearColorTexture> unorderedImageColorClears;
  eastl::vector<CmdClearDepthStencilTexture> unorderedImageDepthStencilClears;
  eastl::vector<BindlessTexUpdateInfo> bindlessTexUpdates;
  eastl::vector<BindlessBufUpdateInfo> bindlessBufUpdates;
  eastl::vector<BindlessSamplerUpdateInfo> bindlessSamplerUpdates;
  eastl::vector<uint32_t> nativeRPDrawCounter;

#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  eastl::vector<VkAccelerationStructureBuildRangeInfoKHR> raytraceBuildRangeInfoKHRStore;
  eastl::vector<VkAccelerationStructureGeometryKHR> raytraceGeometryKHRStore;
  eastl::vector<RaytraceBLASBufferRefs> raytraceBLASBufferRefsStore;
  eastl::vector<RaytraceStructureBuildData> raytraceStructureBuildStore;
#endif
  eastl::vector<ShaderModuleUse> shaderModuleUses;
  AnyCommandStore commandStream;

  TimestampQueryBlock *timestampQueryBlock = nullptr;

  bool generateFaultReport = false;

  size_t getMemorySize() const
  {
    size_t size = 0;
#define CALC_VEC_BYTES(Name) (Name.capacity() * sizeof(decltype(Name)::value_type))
    size += CALC_VEC_BYTES(bufferUploads);
    size += CALC_VEC_BYTES(bufferUploadCopies);
    size += CALC_VEC_BYTES(orderedBufferUploads);
    size += CALC_VEC_BYTES(orderedBufferUploadCopies);
    size += CALC_VEC_BYTES(bufferDownloads);
    size += CALC_VEC_BYTES(bufferDownloadCopies);
    size += CALC_VEC_BYTES(bufferToHostFlushes);
    size += CALC_VEC_BYTES(imageUploads);
    size += CALC_VEC_BYTES(imageUploadCopies);
    size += CALC_VEC_BYTES(imageDownloads);
    size += CALC_VEC_BYTES(imageDownloadCopies);
    size += CALC_VEC_BYTES(charStore);
    size += CALC_VEC_BYTES(imageCopyInfos);
    size += CALC_VEC_BYTES(unorderedImageCopies);
    size += CALC_VEC_BYTES(unorderedImageColorClears);
    size += CALC_VEC_BYTES(unorderedImageDepthStencilClears);
    size += CALC_VEC_BYTES(bindlessTexUpdates);
    size += CALC_VEC_BYTES(bindlessBufUpdates);
    size += CALC_VEC_BYTES(bindlessSamplerUpdates);
    size += CALC_VEC_BYTES(nativeRPDrawCounter);
    size += CALC_VEC_BYTES(reorderedBufferCopies);
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
    size += CALC_VEC_BYTES(raytraceBuildRangeInfoKHRStore);
    size += CALC_VEC_BYTES(raytraceGeometryKHRStore);
    size += CALC_VEC_BYTES(raytraceBLASBufferRefsStore);
    size += CALC_VEC_BYTES(raytraceStructureBuildStore);
#endif
    size += CALC_VEC_BYTES(shaderModuleUses);
#undef CALC_VEC_BYTES
    size += commandStream.capacity();
    size += cleanups.capacity();
    return size;
  }

  template <typename T>
  void pushCommand(const T &cmd)
  {
    commandStream.push_back(cmd);
    recordCommandCaller();
  }

#if DAGOR_DBGLEVEL > 0
  eastl::vector<uint64_t> commandCallers;
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