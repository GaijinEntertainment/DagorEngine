// replay work that is executed in worker thread
#pragma once

#include "query_pools.h"
#include "device_context_cmd.h"
#include "util/variant_vector.h"
#include "cleanup_queue.h"
#include <EASTL/array.h>
#include <EASTL/stack.h>
#include <EASTL/string.h>
#include "util/fault_report.h"

namespace drv3d_vulkan
{

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

struct RaytraceBLASBufferRefs
{
  Buffer *geometry;
  uint32_t geometryOffset;
  uint32_t geometrySize;

  Buffer *index;
  uint32_t indexOffset;
  uint32_t indexSize;
};

struct ContextBackend;
class ExecutionContext;
struct RenderWork
{
  static bool recordCommandCallers;

  CleanupQueue cleanups;

  size_t id = 0;

  eastl::vector<BufferCopyInfo> bufferUploads;
  eastl::vector<VkBufferCopy> bufferUploadCopies;

  eastl::vector<BufferCopyInfo> orderedBufferUploads;
  eastl::vector<VkBufferCopy> orderedBufferUploadCopies;

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
  eastl::vector<Image *> imagesToFillEmptySubresources;

#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  eastl::vector<VkAccelerationStructureBuildRangeInfoKHR> raytraceBuildRangeInfoKHRStore;
  eastl::vector<VkAccelerationStructureGeometryKHR> raytraceGeometryKHRStore;
  eastl::vector<RaytraceBLASBufferRefs> raytraceBLASBufferRefsStore;
#endif
  eastl::vector<ShaderModuleUse> shaderModuleUses;
  AnyCommandStore commandStream;

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
    size += CALC_VEC_BYTES(imagesToFillEmptySubresources);
#if D3D_HAS_RAY_TRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
    size += CALC_VEC_BYTES(raytraceBuildRangeInfoKHRStore);
    size += CALC_VEC_BYTES(raytraceGeometryKHRStore);
    size += CALC_VEC_BYTES(raytraceBLASBufferRefsStore);
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
    if (recordCommandCallers)
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