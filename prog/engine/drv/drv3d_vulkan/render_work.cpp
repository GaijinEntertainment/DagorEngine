// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render_work.h"
#include "execution_context.h"
#include <ska_hash_map/flat_hash_map2.hpp>
#include <EASTL/sort.h>
#include <gui/dag_visualLog.h>
#include <osApiWrappers/dag_files.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <perfMon/dag_statDrv.h>
#include "util/backtrace.h"
#include "memory_heap_resource.h"
#include "backend.h"
#include "execution_async_compile_state.h"
#include "execution_scratch.h"
#include "pipeline_cache.h"
#include "execution_timings.h"
#if _TARGET_ANDROID
#include <unistd.h>
#endif
#include "stacked_profile_events.h"
#include "execution_sync.h"
#include "bindless.h"
#include "vk_to_string.h"
#include "wrapped_command_buffer.h"

using namespace drv3d_vulkan;

#define VULKAN_CONTEXT_COMMAND_IMPLEMENTATION 1

// only for dev builds with profiler
#if DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0

#define VULKAN_BEGIN_CONTEXT_COMMAND(Name)                                                                              \
  inline void Cmd##Name ::execute(ExecutionContext &ctx) const                                                          \
  {                                                                                                                     \
    G_UNUSED(ctx);                                                                                                      \
    static ::da_profiler::desc_id_t cmdMarker = add_description(nullptr, 0, ::da_profiler::Normal, "vulkan_cmd" #Name); \
    Backend::profilerStack.pushChained(cmdMarker);

#else

#define VULKAN_BEGIN_CONTEXT_COMMAND(Name)                     \
  inline void Cmd##Name ::execute(ExecutionContext &ctx) const \
  {                                                            \
    G_UNUSED(ctx);

#endif

#define VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(Name)             \
  inline void Cmd##Name ::execute(ExecutionContext &ctx) const \
  {                                                            \
    G_UNUSED(ctx);

#define VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(Name)          \
  inline void Cmd##Name ::execute(ExecutionContext &ctx) const \
  {                                                            \
    G_UNUSED(ctx);

#define VULKAN_END_CONTEXT_COMMAND }
#define VULKAN_CONTEXT_COMMAND_PARAM(type, name)
#define VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.inc"
#undef VULKAN_BEGIN_CONTEXT_COMMAND
#undef VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT
#undef VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE
#undef VULKAN_END_CONTEXT_COMMAND
#undef VULKAN_CONTEXT_COMMAND_PARAM
#undef VULKAN_CONTEXT_COMMAND_PARAM_ARRAY
#undef VULKAN_CONTEXT_COMMAND_IMPLEMENTATION

namespace
{

struct CmdDumpContext
{
  const RenderWork &renderWork;
  FaultReportDump &dump;
  FaultReportDump::RefId cmdRid;

  void addRef(FaultReportDump::GlobalTag tag, uint64_t id) { dump.addRef(cmdRid, tag, id); }
};

#include "device_context/command_debug_print.inc.cpp"

#define VULKAN_BEGIN_CONTEXT_COMMAND(Name)                                                                            \
  void dumpCommand(const Cmd##Name &cmd, FaultReportDump &dump, const RenderWork &ctx, FaultReportDump::RefId cmdRid) \
  {                                                                                                                   \
    G_UNUSED(ctx);                                                                                                    \
    G_UNUSED(dump);                                                                                                   \
    G_UNUSED(cmd);                                                                                                    \
    G_UNUSED(cmdRid);

#define VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT(Name)                                                                    \
  void dumpCommand(const Cmd##Name &cmd, FaultReportDump &dump, const RenderWork &ctx, FaultReportDump::RefId cmdRid) \
  {                                                                                                                   \
    G_UNUSED(ctx);                                                                                                    \
    G_UNUSED(dump);                                                                                                   \
    G_UNUSED(cmd);                                                                                                    \
    G_UNUSED(cmdRid);

#define VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE(Name)                                                                 \
  void dumpCommand(const Cmd##Name &cmd, FaultReportDump &dump, const RenderWork &ctx, FaultReportDump::RefId cmdRid) \
  {                                                                                                                   \
    G_UNUSED(ctx);                                                                                                    \
    G_UNUSED(dump);                                                                                                   \
    G_UNUSED(cmd);                                                                                                    \
    G_UNUSED(cmdRid);

#define VULKAN_END_CONTEXT_COMMAND }
#define VULKAN_CONTEXT_COMMAND_PARAM(type, Name)                                                                                \
  {                                                                                                                             \
    String paramValue = dumpCmdParam(cmd.Name, {ctx, dump, cmdRid});                                                            \
    FaultReportDump::RefId rid =                                                                                                \
      dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_PARAM, uint64_t(&cmd.Name), String(64, "%s = %s", #Name, paramValue)); \
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_CMD, uint64_t(&cmd));                                                      \
  }
#define VULKAN_CONTEXT_COMMAND_PARAM_ARRAY(type, Name, size)                                                                    \
  {                                                                                                                             \
    String paramValue = dumpCmdParam(cmd.Name, {ctx, dump, cmdRid});                                                            \
    FaultReportDump::RefId rid =                                                                                                \
      dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_PARAM, uint64_t(&cmd.Name), String(64, "%s = %s", #Name, paramValue)); \
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_CMD, uint64_t(&cmd));                                                      \
  }

#include "device_context_cmd.inc"

#undef VULKAN_BEGIN_CONTEXT_COMMAND
#undef VULKAN_BEGIN_CONTEXT_COMMAND_TRANSIT
#undef VULKAN_BEGIN_CONTEXT_COMMAND_NO_PROFILE
#undef VULKAN_END_CONTEXT_COMMAND
#undef VULKAN_CONTEXT_COMMAND_PARAM
#undef VULKAN_CONTEXT_COMMAND_PARAM_ARRAY

} // namespace

bool RenderWork::cleanUpMemoryEveryWorkItem = false;

void RenderWork::dumpData(FaultReportDump &dump) const
{
  for (auto &&bu : bufferUploads)
  {
    auto at = bufferUploadCopies.begin() + bu.copyIndex;
    auto copyEnd = at + bu.copyCount;
    for (; at != copyEnd; ++at)
    {
      FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&at,
        String(64, "buffer upload %u bytes from 0x%p[%u] to 0x%p[%u]", at->size, bu.src, at->srcOffset, bu.dst, at->dstOffset));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bu.src));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bu.dst));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
    }
  }

  for (auto &&bu : orderedBufferUploads)
  {
    auto at = orderedBufferUploadCopies.begin() + bu.copyIndex;
    auto copyEnd = at + bu.copyCount;
    for (; at != copyEnd; ++at)
    {
      FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&at,
        String(64, "ordered buffer upload %u bytes from 0x%p[%u] to 0x%p[%u]", at->size, bu.src, at->srcOffset, bu.dst,
          at->dstOffset));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bu.src));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bu.dst));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
    }
  }

  for (auto &&bd : readbacks->buffers.info)
  {
    auto at = readbacks->buffers.copies.begin() + bd.copyIndex;
    auto copyEnd = at + bd.copyCount;
    for (; at != copyEnd; ++at)
    {
      FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&at,
        String(64, "buffer download %u bytes from 0x%p[%u] to 0x%p[%u]", at->size, bd.src, at->srcOffset, bd.dst, at->dstOffset));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bd.src));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bd.dst));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
    }
  }

  for (auto &&bf : readbacks->bufferFlushes)
  {
    FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&bf,
      String(64, "buffer flush to host 0x%p[%u] %u bytes", bf.buffer, bf.offset, bf.range));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, uint64_t(bf.buffer));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
  }

  for (auto &&iu : readbacks->images.info)
  {
    auto at = readbacks->images.copies.begin() + iu.copyIndex;
    auto copyEnd = at + iu.copyCount;
    for (; at != copyEnd; ++at)
    {
      FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&at,
        String(64,
          "image upload from 0x%p [%u]"
          " to 0x%p [%u-%u][%u]",
          iu.buffer, at->bufferOffset, iu.image, at->imageSubresource.baseArrayLayer,
          at->imageSubresource.baseArrayLayer + at->imageSubresource.layerCount, at->imageSubresource.mipLevel));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)(iu.image));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)(iu.buffer));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
    }
  }

  for (const CmdClearColorTexture &i : unorderedImageColorClears)
  {
    FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&i,
      String(64, "unordered color clear 0x" PTR_LIKE_HEX_FMT " [%u-%u][%u-%u]", i.image, i.area.baseMipLevel,
        i.area.baseMipLevel + i.area.levelCount, i.area.baseArrayLayer, i.area.baseArrayLayer + i.area.layerCount));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i.image);
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
  }

  for (const CmdClearDepthStencilTexture &i : unorderedImageDepthStencilClears)
  {
    FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&i,
      String(64, "unordered depth clear 0x" PTR_LIKE_HEX_FMT " [%u-%u][%u-%u]", i.image, i.area.baseMipLevel,
        i.area.baseMipLevel + i.area.levelCount, i.area.baseArrayLayer, i.area.baseArrayLayer + i.area.layerCount));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i.image);
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
  }

  for (const CmdCopyImage &i : unorderedImageCopies)
  {
    auto at = imageCopyInfos.begin() + i.regionIndex;
    auto copyEnd = at + i.regionCount;
    for (; at != copyEnd; ++at)
    {
      FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD_DATA, (uint64_t)&at,
        String(64,
          "unordered image copy 0x" PTR_LIKE_HEX_FMT "[m%u l%u-%u o%u-%u-%u] -> 0x" PTR_LIKE_HEX_FMT
          "[m%u l%u-%u o%u-%u-%u] %u mips, extend [%u-%u-%u]",
          i.src, i.srcMip, at->srcSubresource.baseArrayLayer, at->srcSubresource.baseArrayLayer + at->srcSubresource.layerCount,
          at->srcOffset.x, at->srcOffset.y, at->srcOffset.z, i.dst, i.dstMip, at->dstSubresource.baseArrayLayer,
          at->dstSubresource.baseArrayLayer + at->dstSubresource.layerCount, at->dstOffset.x, at->dstOffset.y, at->dstOffset.z,
          i.mipCount, at->extent.width, at->extent.height, at->extent.depth));
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i.dst);
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_OBJECT, (uint64_t)i.src);
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
    }
  }
}

void RenderWork::submit() {}

void RenderWork::acquire(size_t timeline_abs_idx) { id = timeline_abs_idx; }

void RenderWork::wait() {}

void RenderWork::cleanup()
{
  bufferUploads.clear();
  bufferUploadCopies.clear();
  orderedBufferUploads.clear();
  orderedBufferUploadCopies.clear();
  imageUploads.clear();
  imageUploadCopies.clear();
  charStore.clear();
  imageCopyInfos.clear();
  unorderedImageCopies.clear();
  unorderedImageColorClears.clear();
  unorderedImageDepthStencilClears.clear();
  bindlessTexUpdates.clear();
  bindlessTexSwaps.clear();
  bindlessBufUpdates.clear();
  bindlessSamplerUpdates.clear();
  nativeRPDrawCounter.clear();
  reorderedBufferCopies.clear();
  sparseMemoryBinds.clear();
  sparseImageMemoryBinds.clear();
  sparseImageOpaqueBinds.clear();
  sparseImageBinds.clear();
#if VULKAN_HAS_RAYTRACING && (VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query)
  raytraceBuildRangeInfoKHRStore.clear();
  raytraceGeometryKHRStore.clear();
  raytraceBLASBufferRefsStore.clear();
  raytraceStructureBuildStore.clear();
#endif
  shaderModuleUses.clear();
  commandStream.clear();
#if DAGOR_DBGLEVEL > 0
  commandCallers.clear();
#endif
  generateFaultReport = false;
  userSignalCount = 0;
}

void RenderWork::processCommands(ExecutionContext &ctx)
{
  {
    using namespace da_profiler;
    static desc_id_t frameCoreMarker = add_description(nullptr, 0, Normal, "vulkan_frameCore");
    Backend::profilerStack.push(frameCoreMarker);
  }
  commandStream.visitAll([&ctx](auto &&value) //
    {
      ctx.beginCmd(&value);
      value.execute(ctx);
      ctx.endCmd();
    });
  // avoid breaking profiler if we lost frame end somehow
  Backend::profilerStack.finish();
}

void RenderWork::process()
{
  TIME_PROFILE(vulkan_render_work_process);

  if (RenderWork::cleanUpMemoryEveryWorkItem || Globals::Mem::res.isOutOfMemorySignalReceived())
  {
    ExecutionContext cleanupExecCtx(*this);
    cleanupExecCtx.cleanupMemory();
  }

  ExecutionContext executionContext(*this);
  executionContext.prepareFrameCore();
  processCommands(executionContext);
}

void RenderWork::shutdown() { cleanup(); }

void RenderWork::dumpCommands(FaultReportDump &dump)
{
#if DAGOR_DBGLEVEL > 0
  uint32_t idx = 0;

  if (Globals::cfg.bits.recordCommandCaller)
  {
    commandStream.visitAll([&](auto &&) {
      uint64_t callerHash = commandCallers[idx++];

      if (dump.hasEntry(FaultReportDump::GlobalTag::TAG_CALLER_HASH, callerHash))
        return;

      dump.addTagged(FaultReportDump::GlobalTag::TAG_CALLER_HASH, callerHash,
        String(128, "caller stack\n%s", backtrace::get_stack_by_hash(callerHash)));
    });

    idx = 0;
  }
#endif

  size_t workItemId = id;

  commandStream.visitAll([&](auto &&value) {
    FaultReportDump::RefId rid = dump.addTagged(FaultReportDump::GlobalTag::TAG_CMD, (uint64_t)&value, String(value.getName()));
    dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, workItemId);

    dumpCommand(value, dump, *this, rid);

#if DAGOR_DBGLEVEL > 0
    if (Globals::cfg.bits.recordCommandCaller)
      dump.addRef(rid, FaultReportDump::GlobalTag::TAG_CALLER_HASH, commandCallers[idx++]);
#endif
  });
  const size_t totalSize = getMemorySize();
  const size_t cmdSize = commandStream.size();
  const size_t cmdCap = commandStream.capacity();

  FaultReportDump::RefId rid = dump.add(String(64,
    "work item %016llX cmd buffer stats\n"
    "memory usage: %u Bytes, %u KiBytes, %u MiBytes\n"
    "memory allocated: %u Bytes, %u KiBytes, %u MiBytes\n"
    "efficiency: %f%%\n"
    "total batch memory allocated: %u Bytes, %u KiBytes, %u MiBytes\n",
    id, cmdSize, cmdSize / 1024, cmdSize / 1024 / 1024, cmdCap, cmdCap / 1024, cmdCap / 1024 / 1024,
    cmdCap ? ((double(cmdSize) / cmdCap) * 100.0) : 100.0, totalSize, totalSize / 1024, totalSize / 1024 / 1024));
  dump.addRef(rid, FaultReportDump::GlobalTag::TAG_WORK_ITEM, id);
}
