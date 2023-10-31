#include "device.h"

#include <osApiWrappers/dag_files.h>
#include <ioSys/dag_dataBlock.h>
#include <debug/dag_logSys.h>
#include <perfMon/dag_statDrv.h>
#include <EASTL/sort.h>
#include <3d/dag_lowLatency.h>
#include <drv_returnAddrStore.h>

#if _TARGET_XBOX
#include <osApiWrappers/xbox/app.h> // make_thread_time_sensitive
#endif

#include <util/dag_watchdog.h>
#include <stereoHelper.h>

#define DX12_LOCK_FRONT() WinAutoLock lock(getFrontGuard())

using namespace drv3d_dx12;

#if _TARGET_64BIT
#define PTR_LIKE_HEX_FMT "%016X"
#else
#define PTR_LIKE_HEX_FMT "%08X"
#endif

namespace workcycle_internal
{
extern bool application_active;
}

namespace
{
// Default resolver, just returns original ref
template <typename T, typename U>
U &resolve(T &, U &u)
{
  return u;
}

#define DX12_CONTEXT_COMMAND_IMPLEMENTATION 1
// mark cmd and ctx as unused to avoid errors when the implementation does not use anything
// RenderWork is private to DeviceContext, use template to work around that
#define DX12_BEGIN_CONTEXT_COMMAND(Name)               \
  template <typename T, typename D>                    \
  void execute(const Cmd##Name &cmd, D &debug, T &ctx) \
  {                                                    \
    G_UNUSED(cmd);                                     \
    G_UNUSED(ctx);                                     \
    debug.setCommandData(cmd, "Cmd" #Name);

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(Name, Param0Type, Param0Name)                \
  template <typename T, typename D>                                                   \
  void execute(const ExtendedVariant<Cmd##Name, Param0Type> &param, D &debug, T &ctx) \
  {                                                                                   \
    auto &cmd = param.cmd;                                                            \
    auto &Param0Name = param.p0;                                                      \
    G_UNUSED(Param0Name);                                                             \
    G_UNUSED(cmd);                                                                    \
    G_UNUSED(ctx);                                                                    \
    debug.setCommandData(cmd, "Cmd" #Name);

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(Name, Param0Type, Param0Name, Param1Type, Param1Name)     \
  template <typename T, typename D>                                                                \
  void execute(const ExtendedVariant2<Cmd##Name, Param0Type, Param1Type> &param, D &debug, T &ctx) \
  {                                                                                                \
    auto &cmd = param.cmd;                                                                         \
    auto &Param0Name = param.p0;                                                                   \
    auto &Param1Name = param.p1;                                                                   \
    G_UNUSED(Param0Name);                                                                          \
    G_UNUSED(cmd);                                                                                 \
    G_UNUSED(ctx);                                                                                 \
    debug.setCommandData(cmd, "Cmd" #Name);

#define DX12_END_CONTEXT_COMMAND }
// make an alias so we do not need to write cmd.
#define DX12_CONTEXT_COMMAND_PARAM(type, name)  \
  decltype(auto) name = resolve(ctx, cmd.name); \
  G_UNUSED(name);
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) \
  decltype(auto) name = resolve(ctx, cmd.name);            \
  G_UNUSED(name);
#include "device_context_cmd.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
#undef DX12_CONTEXT_COMMAND_IMPLEMENTATION
#undef HANDLE_RETURN_ADDRESS
} // namespace

void DeviceContext::replayCommands()
{
  ExecutionContext executionContext //
    {*this, back.sharedContextState};
  executionContext.beginWork();
  commandStream.tryVisitAllIndexAndDestroy([this, &executionContext](auto &&value, auto index) //
    {
      logCommand(value);
      executionContext.prepareCommandExecution();
      executionContext.beginCmd(index);
      execute(value, back.sharedContextState, executionContext);
      executionContext.endCmd();
    });
}

// don't profile in release build
#define PROFILE_DX12_PROCESS_COMMAND_STREAM (DA_PROFILER_ENABLED && DAGOR_DBGLEVEL > 0)

void DeviceContext::replayCommandsConcurrently(volatile int &terminate)
{
  ExecutionContext executionContext //
    {*this, back.sharedContextState};
  executionContext.beginWork();
  while (!terminate && !worker->terminateIncoming)
  {
    commandStream.waitToVisit();
    bool tryVisitAgain = true;
    while (tryVisitAgain)
    {
#if PROFILE_DX12_PROCESS_COMMAND_STREAM
      TIME_PROFILE_DEV(DX12_processCommandStream);
      uint32_t startIndex = executionContext.cmdIndex;
#endif
      tryVisitAgain = commandStream.tryVisitAllIndexAndDestroy([this, &executionContext](auto &&value, auto index) {
        logCommand(value);
        executionContext.prepareCommandExecution();
        executionContext.beginCmd(index);
        execute(value, back.sharedContextState, executionContext);
        executionContext.endCmd();
      });
#if PROFILE_DX12_PROCESS_COMMAND_STREAM
      DA_PROFILE_TAG(processed_entries, "%u", executionContext.cmdIndex - startIndex);
#endif
      // just try again, tryVisitAllIndexAndDestroy grabs atomic only at the beginning so extra work
      // could be available at its end.
    }
  }

  do
  {
    // one last time to clean up
    while (commandStream.tryVisitAllIndexAndDestroy([this, &executionContext](auto &&value, auto index) //
      {
        logCommand(value);
        executionContext.prepareCommandExecution();
        executionContext.beginCmd(index);
        execute(value, back.sharedContextState, executionContext);
        executionContext.endCmd();
      }))
    {
      // run until we are out of command stream entries
    }
    // run cleanup w/o wait until real terminate is signaled
  } while (!terminate);
}

void DeviceContext::frontFlush()
{
  front.recordingLatchedFrame->progress = front.recordingWorkItemProgress;
  front.recordingWorkItemProgress = front.nextWorkItemProgress++;
  manageLatchedState();

  front.renderTargetIndices.clear();
}

void DeviceContext::manageLatchedState()
{
  front.recordingLatchedFrame = nullptr;
  FrontendFrameLatchedData *nextWait = nullptr;
  while (!front.recordingLatchedFrame)
  {
    if (nextWait)
    {
      wait_for_frame_progress_with_event(device.queues, nextWait->progress, front.frameWaitEvent.get(), "manageLatchedState");
    }
    for (auto &&frame : front.latchedFrameSet)
    {
      if (frame.progress > device.queues.checkFrameProgress())
      {
        if (!nextWait || nextWait->progress > frame.progress)
        {
          nextWait = &frame;
        }
        continue;
      }

      if (frame.progress)
      {
        tidyFrame(frame);
      }

      front.recordingLatchedFrame = &frame;
    }
  }

  ResourceMemoryHeap::BeginFrameRecordingInfo frameRecodingInfo;
  frameRecodingInfo.historyIndex = front.recordingLatchedFrame - front.latchedFrameSet;
  front.recordingLatchedFrame->frameIndex = front.frameIndex;
  device.resources.beginFrameRecording(frameRecodingInfo);

#if DX12_RECORD_TIMING_DATA

#if DX12_REPORT_LONG_FRAMES
#if _TARGET_XBOX
  // There is no need to report long frames when game is in constrained state
  if (workcycle_internal::application_active)
  {
#endif

    auto &completedTimings = front.timingHistory[front.completedFrameIndex % timing_history_length];
    auto frameTimeUsec = ref_time_delta_to_usec(completedTimings.frontendUpdateScreenInterval);
    auto frameLatency = ref_time_delta_to_usec(completedTimings.frontendToBackendUpdateScreenLatency);
    auto frameFrontBackWait = ref_time_delta_to_usec(completedTimings.frontendBackendWaitDuration);
    auto frameBackFrontWait = ref_time_delta_to_usec(completedTimings.backendFrontendWaitDuration);
    auto frameGPUWait = ref_time_delta_to_usec(completedTimings.gpuWaitDuration);
    auto swapSwapWait = ref_time_delta_to_usec(completedTimings.presentDuration);
    auto getSwapWait = ref_time_delta_to_usec(completedTimings.backbufferAcquireDuration);
    auto frontSwapchainWait = ref_time_delta_to_usec(completedTimings.frontendWaitForSwapchainDuration);
    constexpr int64_t dur_th = 40 * 1000;
    if ((frameTimeUsec > dur_th) || (frameLatency > dur_th) || (frameFrontBackWait > dur_th) || (frameBackFrontWait > dur_th) ||
        (frameGPUWait > dur_th) || (swapSwapWait > dur_th) || (getSwapWait > dur_th))
    {
      debug("DX12: Frame interval was %uus", frameTimeUsec);
      debug("DX12: Frame latency was %uus", frameLatency);
      debug("DX12: Frame front to back wait was %uus", frameFrontBackWait);
      debug("DX12: Frame back to front wait was %uus", frameBackFrontWait);
      debug("DX12: Frame GPU wait was %uus", frameGPUWait);
      debug("DX12: Frame Swapchain swap was %uus", swapSwapWait);
      debug("DX12: Frame Get Swapchain buffer was %uus", getSwapWait);
      debug("DX12: Frame Wait For Swapchain waitable object was %uus", frontSwapchainWait);
    }

#if _TARGET_XBOX
  }
#endif
#endif

#endif
}

void DeviceContext::makeReadyForFrame(OutputMode mode)
{
#if DX12_RECORD_TIMING_DATA
  auto now = ref_time_ticks();
#endif
  back.sharedContextState.frames.advance();
#if _TARGET_PC_WIN
  // On PC, if we execute stuff on the worker thread, we need to wait for the main thread to send
  // the message to acquire the swap image, otherwise we probably get the same swap index as before
  // and use the swap image of the last frame
  const bool shouldAcquireSwapImageNow = !device.getContext().hasWorkerThread() && OutputMode::PRESENT == mode;
#else
  constexpr bool shouldAcquireSwapImageNow = true;
  G_UNUSED(mode);
#endif
  if (shouldAcquireSwapImageNow)
  {
    back.swapchain.onFrameBegin(device.device.get());
  }
#if DX12_RECORD_TIMING_DATA
  back.acquireBackBufferDuration = ref_time_ticks() - now;
#endif

  FrameInfo &frame = back.sharedContextState.getFrameData();
#if DX12_RECORD_TIMING_DATA
  back.gpuWaitDuration =
#endif
    frame.beginFrame(device, device.queues, device.pipeMan);

  back.sharedContextState.onFrameStateInvalidate(device.nullResourceTable.table[NullResourceTable::RENDER_TARGET_VIEW]);
}

WinCritSec &DeviceContext::getFrontGuard() { return mutex; }

#if FIXED_EXECUTION_MODE
void DeviceContext::initMode()
#else
void DeviceContext::initMode(ExecutionMode mode)
#endif
{
  const DataBlock *dxCfg = ::dgs_get_settings()->getBlockByNameEx("dx12");
  bool recordResourceStates = dxCfg->getBool("recordResourceStates", false);
  uint32_t framesToInitiallyRecord = dxCfg->getInt("recordResourceStatesFrames", 0);
  ResourceUsageHistoryDataSetDebugger::configure(recordResourceStates, framesToInitiallyRecord);
  debug::call_stack::Generator::configure(::dgs_get_settings()->getBlockByNameEx("dx12"));

  commandStream.resize((1024 * 1024) / 4);

#if !FIXED_EXECUTION_MODE
  executionMode = mode;
  if (ExecutionMode::CONCURRENT == executionMode)
#endif
  {
#if _TARGET_XBOX
    enteredSuspendedStateEvent.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    resumeExecutionEvent.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
#endif

    worker.reset(new WorkerThread{*this});
    worker->start();
    worker->setAffinity(WORKER_THREADS_AFFINITY_MASK); // Prevent preemption of main thread on
                                                       // worker wake ups
  }
}

#if !FIXED_EXECUTION_MODE
void DeviceContext::immediateModeExecute(bool flush)
{
  if (isImmediateMode())
  {
    if (flush && isImmediateFlushMode())
      waitInternal();
    else
      replayCommands();
  }
}

bool DeviceContext::enableImmediateFlush()
{
  if (isImmediateMode())
  {
    executionMode = ExecutionMode::IMMEDIATE_FLUSH;
    G_ASSERT(!isImmediateFlushSuppressed);
    return true;
  }
  else
    logerr("DX12: The immediate flush doesn't work in non immedite execution mode!");
  return false;
}

void DeviceContext::disableImmediateFlush()
{
  if (isImmediateFlushMode())
  {
    executionMode = ExecutionMode::IMMEDIATE;
    G_ASSERT(!isImmediateFlushSuppressed);
  }
}
#endif

void DeviceContext::transitionBuffer(BufferResourceReference buffer, D3D12_RESOURCE_STATES state)
{
  auto cmd = make_command<CmdTransitionBuffer>(buffer, state);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::resizeImageMipMapTransfer(Image *src, Image *dst, MipMapRange mip_map_range, uint32_t src_mip_map_offset,
  uint32_t dst_mip_map_offset)
{
  G_ASSERT(mip_map_range.isValidRange());
  auto cmd = make_command<CmdResizeImageMipMapTransfer>(src, dst, mip_map_range, src_mip_map_offset, dst_mip_map_offset);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::debugBreak()
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdDebugBreak>());
  immediateModeExecute();
}

void DeviceContext::addDebugBreakString(eastl::string_view str)
{
  auto cmd = make_command<CmdAddDebugBreakString>();
  DX12_LOCK_FRONT();
  commandStream.pushBack<CmdAddDebugBreakString, const char>(cmd, str.data(), str.size());
  immediateModeExecute();
}

void DeviceContext::removeDebugBreakString(eastl::string_view str)
{
  auto cmd = make_command<CmdRemoveDebugBreakString>();
  DX12_LOCK_FRONT();
  commandStream.pushBack<CmdRemoveDebugBreakString, const char>(cmd, str.data(), str.size());
  immediateModeExecute();
}

#if !_TARGET_XBOXONE
void DeviceContext::dispatchMesh(uint32_t x, uint32_t y, uint32_t z)
{
  commandStream.pushBack(make_command<CmdDispatchMesh>(x, y, z));
  immediateModeExecute(true);
}

void DeviceContext::dispatchMeshIndirect(BufferResourceReferenceAndOffset args, uint32_t stride,
  BufferResourceReferenceAndOffset count, uint32_t max_count)
{
  commandStream.pushBack(make_command<CmdDispatchMeshIndirect>(args, count, stride, max_count));
  immediateModeExecute(true);
}
#endif

void DeviceContext::addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdAddShaderGroup>(group, dump, null_pixel_shader));
  immediateModeExecute();
}

void DeviceContext::removeShaderGroup(uint32_t group)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdRemoveShaderGroup>(group));
  immediateModeExecute();
}

void DeviceContext::loadComputeShaderFromDump(ProgramID program)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdLoadComputeShaderFromDump>(program));
  immediateModeExecute();
}

void DeviceContext::compilePipelineSet(const DataBlock *feature_sets, DynamicArray<InputLayoutID> &&input_layouts,
  DynamicArray<StaticRenderStateID> &&static_render_states, const DataBlock *output_formats_set,
  const DataBlock *graphics_pipeline_set, const DataBlock *mesh_pipeline_set, const DataBlock *compute_pipeline_set,
  const char *default_format)
{
  // resolve if any feature set is supported by this device
  DynamicArray<bool> featureSetSupported;
  if (feature_sets && feature_sets->blockCount() > 0)
  {
    auto blockCount = feature_sets->blockCount();
    featureSetSupported.resize(blockCount);
    pipeline::DataBlockDecodeEnumarator<pipeline::FeatureSupportResolver> resolver{*feature_sets, 0, d3d::get_driver_desc()};
    for (; !resolver.completed(); resolver.next())
    {
      resolver.decode(featureSetSupported[resolver.index()]);
    }
  }

  DynamicArray<FramebufferLayout> framebufferLayouts;
  if (output_formats_set && output_formats_set->blockCount() > 0)
  {
    auto blockCount = output_formats_set->blockCount();
    framebufferLayouts.resize(blockCount);
    pipeline::DataBlockDecodeEnumarator<pipeline::FramebufferLayoutDecoder> decoder{*output_formats_set, 0, default_format};
    for (; !decoder.completed(); decoder.next())
    {
      auto fi = decoder.index();
      framebufferLayouts[fi] = {};
      decoder.invoke([&framebufferLayouts, fi](auto &info) {
        framebufferLayouts[fi] = info;
        return true;
      });
    }
  }

  DynamicArray<GraphicsPipelinePreloadInfo> graphicsPiplines;
  if (graphics_pipeline_set && graphics_pipeline_set->blockCount() > 0)
  {
    auto blockCount = graphics_pipeline_set->blockCount();
    graphicsPiplines.resize(blockCount);
    uint32_t decodedCount = 0;
    pipeline::DataBlockDecodeEnumarator<pipeline::GraphicsPipelineDecoder> decoder{
      *graphics_pipeline_set, 0, featureSetSupported.data(), featureSetSupported.size()};
    for (; !decoder.completed(); decoder.next())
    {
      if (decoder.decode(graphicsPiplines[decodedCount].base, graphicsPiplines[decodedCount].variants))
      {
        ++decodedCount;
      }
    }
    graphicsPiplines.resize(decodedCount);
  }

  DynamicArray<MeshPipelinePreloadInfo> meshPipelines;
  if (mesh_pipeline_set && mesh_pipeline_set->blockCount() > 0)
  {
    auto blockCount = mesh_pipeline_set->blockCount();
    meshPipelines.resize(blockCount);
    uint32_t decodedCount = 0;
    pipeline::DataBlockDecodeEnumarator<pipeline::MeshPipelineDecoder> decoder{
      *mesh_pipeline_set, 0, featureSetSupported.data(), featureSetSupported.size()};
    for (; !decoder.completed(); decoder.next())
    {
      if (decoder.decode(meshPipelines[decodedCount].base, meshPipelines[decodedCount].variants))
      {
        ++decodedCount;
      }
    }
    meshPipelines.resize(decodedCount);
  }

  DynamicArray<ComputePipelinePreloadInfo> computePipelines;
  if (compute_pipeline_set && compute_pipeline_set->blockCount() > 0)
  {
    auto blockCount = compute_pipeline_set->blockCount();
    computePipelines.resize(blockCount);
    uint32_t decodedCount = 0;
    pipeline::DataBlockDecodeEnumarator<pipeline::ComputePipelineDecoder> decoder{
      *compute_pipeline_set, 0, featureSetSupported.data(), featureSetSupported.size()};
    for (; !decoder.completed(); decoder.next())
    {
      if (decoder.decode(computePipelines[decodedCount].base))
      {
        ++decodedCount;
      }
    }
    meshPipelines.resize(decodedCount);
  }

  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdCompilePipelineSet>(input_layouts.releaseAsSpan(), static_render_states.releaseAsSpan(),
    framebufferLayouts.releaseAsSpan(), graphicsPiplines.releaseAsSpan(), meshPipelines.releaseAsSpan(),
    computePipelines.releaseAsSpan()));
  immediateModeExecute();
}

void DeviceContext::resizeSwapchain(Extent2D size)
{
  front.swapchain.prepareForShutdown(device);
  back.swapchain.prepareForShutdown(device);
  // finish all outstanding frames
  back.sharedContextState.frames.walkAll([=](auto &frame) //
    { frame.beginFrame(device, device.queues, device.pipeMan); });
  back.swapchain.bufferResize(device, size, front.swapchain.getColorFormat());
  front.swapchain.bufferResize(size);
  makeReadyForFrame(OutputMode::PRESENT);
}

void DeviceContext::waitInternal()
{
  EventPointer event;

  if (!front.eventsPool.empty())
  {
    event = eastl::move(front.eventsPool.back());
    front.eventsPool.pop_back();
  }
  if (!event)
  {
    event.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
  }

  uint64_t progressToWaitFor = front.recordingWorkItemProgress;
  auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush();

  wait_for_frame_progress_with_event(device.queues, progressToWaitFor, event.get(), "wait");
  front.eventsPool.push_back(eastl::move(event));
}

void DeviceContext::finishInternal()
{
  waitInternal();

  manageLatchedState();

  closeFrameEndCallbacks();
}

void DeviceContext::blitImageInternal(Image *src, Image *dst, const ImageBlit &region, bool disable_predication)
{
  auto cmd = make_command<CmdBlitImage>();
  cmd.disablePedication = disable_predication;

  // setup dst related params
  cmd.dst = dst;

  cmd.dstView.isArray = dst->getArrayLayers().count() > 1;
  cmd.dstView.isCubemap = 0;
  cmd.dstView.setFormat(dst->getFormat());
  cmd.dstView.setMipBase(region.dstSubresource.mipLevel);
  cmd.dstView.setMipCount(1);
  cmd.dstView.setArrayBase(region.dstSubresource.baseArrayLayer);
  cmd.dstView.setArrayCount(1);
  cmd.dstView.setRTV();
  cmd.dstViewDescriptor = device.getImageView(dst, cmd.dstView);

  cmd.dstRect.left = region.dstOffsets[0].x;
  cmd.dstRect.top = region.dstOffsets[0].y;
  cmd.dstRect.right = region.dstOffsets[1].x;
  cmd.dstRect.bottom = region.dstOffsets[1].y;

  // setup src related params
  cmd.src = src;

  cmd.srcView.isArray = src->getArrayLayers().count() > 1;
  cmd.srcView.isCubemap = 0;
  cmd.srcView.setFormat(src->getFormat());
  cmd.srcView.setMipBase(region.srcSubresource.mipLevel);
  cmd.srcView.setMipCount(1);
  cmd.srcView.setArrayBase(region.srcSubresource.baseArrayLayer);
  cmd.srcView.setArrayCount(1);
  cmd.srcView.setSRV();
  cmd.srcView.sampleStencil = 0;
  cmd.srcViewDescriptor = device.getImageView(src, cmd.srcView);

  cmd.srcRect.left = region.srcOffsets[0].x;
  cmd.srcRect.top = region.srcOffsets[0].y;
  cmd.srcRect.right = region.srcOffsets[1].x;
  cmd.srcRect.bottom = region.srcOffsets[1].y;

  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::tidyFrame(FrontendFrameLatchedData &frame)
{
  if (!frame.deletedQueries.empty())
  {
    device.frontendQueryManager.removeDeletedQueries(frame.deletedQueries);
    frame.deletedQueries.clear();
  }

  // keep track of the progress that frontend has completed including resource cleanup
  front.frontendFinishedWorkItem.store(max(frame.progress, front.frontendFinishedWorkItem.load(std::memory_order_relaxed)),
    std::memory_order_release);

  ResourceMemoryHeap::CompletedFrameExecutionInfo frameCompleteInfo;
  frameCompleteInfo.historyIndex = &frame - front.latchedFrameSet;
  frameCompleteInfo.progressIndex = frame.progress;
  frameCompleteInfo.bindlessManager = &device.bindlessManager;
  frameCompleteInfo.device = device.device.get();
#if _TARGET_PC_WIN
  frameCompleteInfo.adapter = device.adapter.Get();
#endif

  device.resources.completeFrameExecution(frameCompleteInfo);

  front.completedFrameProgress = max(frame.progress, front.completedFrameProgress);
#if DX12_RECORD_TIMING_DATA
  front.completedFrameIndex = max(frame.frameIndex, front.completedFrameIndex);
#endif
  // progress of 0 indicates the frame info was cleared
  frame.progress = 0;
}

#if _TARGET_PC_WIN
void DeviceContext::onDeviceError(HRESULT remove_reason)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdOnDeviceError>(remove_reason));
  immediateModeExecute();
}
#endif

void DeviceContext::initFrameStates()
{
  back.sharedContextState.frames.iterate([=](auto &frame) //
    { frame.init(device.device.get()); });

  back.sharedContextState.bindlessSetManager.init(device.device.get());

  front.frameWaitEvent.reset(CreateEvent(nullptr, false, false, nullptr));
  front.recordingLatchedFrame = &front.latchedFrameSet[0];

  ResourceMemoryHeap::BeginFrameRecordingInfo frameRecodingInfo;
  frameRecodingInfo.historyIndex = 0;
  device.resources.beginFrameRecording(frameRecodingInfo);

  int tempBufferShrinkThresholdSize =
    ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("tempBufferShrinkThresholdSize", 16777216); // 16 MB
  device.resources.setTempBufferShrinkThresholdSize(tempBufferShrinkThresholdSize);

  back.constBufferStreamDescriptors.init(device.device.get());
}

namespace
{
inline const char *get_prefix_name(uint32_t index)
{
  static const char *unitTable[] = {"", "K", "M", "G"};
  return unitTable[index];
}

inline uint32_t count_to_prefix_index(uint64_t sz)
{
  uint32_t unitIndex = 0;
  unitIndex += sz >= (1000 * 1000 * 1000);
  unitIndex += sz >= (1000 * 1000);
  unitIndex += sz >= (1000);
  return unitIndex;
}

inline float compute_prefix_units(uint64_t sz, uint32_t unit_index) { return static_cast<float>(sz) / (powf(1000, unit_index)); }


class PrefixedUnits
{
  uint64_t size = 0;

public:
  PrefixedUnits() = default;

  PrefixedUnits(const PrefixedUnits &) = default;
  PrefixedUnits &operator=(const PrefixedUnits &) = default;


  PrefixedUnits(uint64_t v) : size{v} {}
  PrefixedUnits &operator=(uint64_t v)
  {
    size = v;
    return *this;
  }

  PrefixedUnits &operator+=(PrefixedUnits o)
  {
    size += o.size;
    return *this;
  }
  PrefixedUnits &operator-=(PrefixedUnits o)
  {
    size -= o.size;
    return *this;
  }

  friend PrefixedUnits operator+(PrefixedUnits l, PrefixedUnits r) { return {l.size + r.size}; }
  friend PrefixedUnits operator-(PrefixedUnits l, PrefixedUnits r) { return {l.size - r.size}; }

  uint64_t value() const { return size; }
  float units() const { return compute_prefix_units(size, count_to_prefix_index(size)); }
  const char *name() const { return get_prefix_name(count_to_prefix_index(size)); }
};

template <typename A>
void report_metrics(VariantContainerVisitMetricsCollector<A> &metrics)
{
  struct CommandVisitCount
  {
    size_t cmdIndex;
    PrefixedUnits visitCount;
  };

  CommandVisitCount commandArray[metrics.command_count];

  PrefixedUnits totalCount = metrics.getAllVisitCounts([&](auto index, auto count) {
    commandArray[index].cmdIndex = index;
    commandArray[index].visitCount = count;
  });

  if (totalCount.value())
  {
    static const char *cmdNames[metrics.command_count]{
#define DX12_BEGIN_CONTEXT_COMMAND(name)                                                       "Cmd" #name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(name, param0Type, param0Name)                         "Cmd" #name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(name, param0Type, param0Name, param1Type, param1Name) "Cmd" #name,
#define DX12_END_CONTEXT_COMMAND
#define DX12_CONTEXT_COMMAND_PARAM(type, name)
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
      "void"};

    auto frameCount = metrics.template getVisitCountFor<CmdPresent>();

    // -1 because last one is void as terminating entry
    eastl::sort(eastl::begin(commandArray), eastl::end(commandArray) - 1, [](auto &l, auto &r) {
      return r.visitCount.value() < l.visitCount.value() ||
             ((r.visitCount.value() == l.visitCount.value()) && (l.cmdIndex < r.cmdIndex));
    });

    debug("DX12: Command Stream visited commands statistics:");
    debug("%3s, [%3s] %-45s, %10s, %6s, %9s", "#", "ID", "name", "count", "%", "frame avg");
    for (auto at = eastl::begin(commandArray), ed = eastl::end(commandArray) - 1; at != ed; ++at)
    {
      auto perc = at->visitCount.value() * 100.0 / totalCount.value();
      auto perFrame = double(at->visitCount.value()) / frameCount;
      debug("%3u, [%3u] %-45s, %8.2f %1s, %5.2f%%, %9.2f", 1 + (at - eastl::begin(commandArray)), at->cmdIndex, cmdNames[at->cmdIndex],
        at->visitCount.units(), at->visitCount.name(), perc, perFrame);
    }
    debug("%.2f %s total visits", totalCount.units(), totalCount.name());
  }
}

template <typename A>
void report_metrics(VariantContainerNullMetricsCollector<A> &)
{}
}

void DeviceContext::shutdownFrameStates()
{
  back.sharedContextState.frames.walkAll([=](auto &frame) //
    { frame.shutdown(device.queues, device.pipeMan); });

  back.sharedContextState.bindlessSetManager.shutdown();

  back.sharedContextState.drawIndirectSignatures.reset();
  back.sharedContextState.drawIndexedIndirectSignatures.reset();
  back.sharedContextState.dispatchIndirectSignature.Reset();
  back.constBufferStreamDescriptors.shutdown();

  front.recordingLatchedFrame = nullptr;

  report_metrics(commandStream);
}

void DeviceContext::clearRenderTargets(ViewportState vp, uint32_t clear_mask, const E3DCOLOR *clear_color, float clear_depth,
  uint8_t clear_stencil)
{
  auto cmd = make_command<CmdClearRenderTargets>(vp, clear_mask, clear_depth, clear_stencil);
  memcpy(cmd.clearColor, clear_color, sizeof(cmd.clearColor));
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::pushConstRegisterData(uint32_t stage, eastl::span<const ConstRegisterType> data)
{
  auto update = device.resources.allocatePushMemory(device.getDXGIAdapter(), device.device.get(),
    sizeof(ConstRegisterType) * data.size(), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
  if (!update)
  {
    return;
  }
  eastl::copy(data.begin(), data.end(), update.as<ConstRegisterType>());
  update.flush();
  auto cmd = make_command<CmdSetConstRegisterBuffer>(update, stage);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setSRVTexture(uint32_t stage, size_t unit, BaseTex *texture, ImageViewState view, bool as_const_ds)
{
  // partially ready textures can return null here...
  Image *image = texture ? texture->getDeviceImage() : nullptr;
  // also check texture again to help static analyzer to see that texture can not be null when not taking this branch
  if (nullptr == image || nullptr == texture)
  {
    auto cmd = make_command<CmdSetSRVTexture>(stage, static_cast<uint32_t>(unit), nullptr, ImageViewState{},
      D3D12_CPU_DESCRIPTOR_HANDLE{}, as_const_ds);
    commandStream.pushBack(cmd, false /*wake executor*/);
    immediateModeExecute();
    return;
  }

  commandStream.pushBack(
    make_command<CmdSetSRVTexture>(stage, static_cast<uint32_t>(unit), image, view, device.getImageView(image, view), as_const_ds),
    false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setSampler(uint32_t stage, size_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler)
{
  commandStream.pushBack(make_command<CmdSetSampler>(stage, static_cast<uint32_t>(unit), sampler), false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setSamplerHandle(uint32_t stage, size_t unit, d3d::SamplerHandle sampler)
{
  commandStream.pushBack(make_command<CmdSetSampler>(stage, static_cast<uint32_t>(unit), device.getSampler(sampler)), false);
  immediateModeExecute();
}

void DeviceContext::setUAVTexture(uint32_t stage, size_t unit, Image *image, ImageViewState view_state)
{
  auto cmd = make_command<CmdSetUAVTexture>(stage, static_cast<uint32_t>(unit), image, view_state);
  if (image)
  {
    cmd.viewDescriptor = device.getImageView(image, view_state);
  }
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setSRVBuffer(uint32_t stage, size_t unit, BufferResourceReferenceAndShaderResourceView buffer)
{
  auto cmd = make_command<CmdSetSRVBuffer>(stage, static_cast<uint32_t>(unit), buffer);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setUAVBuffer(uint32_t stage, size_t unit, BufferResourceReferenceAndUnorderedResourceView buffer)
{
  auto cmd = make_command<CmdSetUAVBuffer>(stage, static_cast<uint32_t>(unit), buffer);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setConstBuffer(uint32_t stage, size_t unit, BufferResourceReferenceAndAddressRange buffer)
{
  auto cmd = make_command<CmdSetUniformBuffer>(stage, static_cast<uint32_t>(unit), buffer);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setSRVNull(uint32_t stage, uint32_t unit)
{
  auto cmd = make_command<CmdSetSRVNull>(stage, unit);
  commandStream.pushBack(cmd, false);
  immediateModeExecute();
}

void DeviceContext::setUAVNull(uint32_t stage, uint32_t unit)
{
  auto cmd = make_command<CmdSetUAVNull>(stage, unit);
  commandStream.pushBack(cmd, false);
  immediateModeExecute();
}

void DeviceContext::setBlendConstant(E3DCOLOR color)
{
  auto cmd = make_command<CmdSetBlendConstantFactor>(color);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setDepthBoundsRange(float from, float to)
{
  auto cmd = make_command<CmdSetDepthBoundsRange>(from, to);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setPolygonLine(bool enable)
{
  auto cmd = make_command<CmdSetPolygonLineEnable>(enable);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setStencilRef(uint8_t ref)
{
  auto cmd = make_command<CmdSetStencilRef>(ref);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setScissorEnable(bool enabled)
{
  auto cmd = make_command<CmdSetScissorEnable>(enabled);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setScissorRects(dag::ConstSpan<D3D12_RECT> rects)
{
  commandStream.pushBack(make_command<CmdSetScissorRects>(), rects.data(), rects.size());
  immediateModeExecute();
}

void DeviceContext::bindVertexDecl(InputLayoutID ident)
{
  auto cmd = make_command<CmdSetInputLayout>(ident);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setIndexBuffer(BufferResourceReferenceAndAddressRange buffer, DXGI_FORMAT type)
{
  auto cmd = make_command<CmdSetIndexBuffer>(buffer, type);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::bindVertexBuffer(uint32_t stream, BufferResourceReferenceAndAddressRange buffer, uint32_t stride)
{
  auto cmd = make_command<CmdSetVertexBuffer>(stream, buffer, stride);
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
  auto cmd = make_command<CmdDispatch>(x, y, z);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::dispatchIndirect(BufferResourceReferenceAndOffset buffer)
{
  auto cmd = make_command<CmdDispatchIndirect>(buffer);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::drawIndirect(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, BufferResourceReferenceAndOffset buffer,
  uint32_t stride)
{
  auto cmd = make_command<CmdDrawIndirect>(top, count, buffer, stride);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::drawIndexedIndirect(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, BufferResourceReferenceAndOffset buffer,
  uint32_t stride)
{
  auto cmd = make_command<CmdDrawIndexedIndirect>(top, count, buffer, stride);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::draw(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t start, uint32_t count, uint32_t first_instance,
  uint32_t instance_count)
{
  auto cmd = make_command<CmdDraw>(top, start, count, first_instance, instance_count);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::drawUserData(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, uint32_t stride, const void *vertex_data)
{
  auto vertexDataBytes = count * stride;
  // vertex data needs to be dword aligned or some GPUs will render garbage
  auto vertexData =
    device.resources.allocatePushMemory(device.getDXGIAdapter(), device.device.get(), vertexDataBytes, sizeof(uint32_t));
  if (!vertexData)
  {
    return;
  }
  memcpy(vertexData.pointer, vertex_data, vertexDataBytes);
  vertexData.flush();

  auto cmd = make_command<CmdDrawUserData>(top, count, stride, vertexData);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::drawIndexed(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t index_start, uint32_t count, int32_t vertex_base,
  uint32_t first_instance, uint32_t instance_count)
{
  auto cmd = make_command<CmdDrawIndexed>(top, index_start, count, vertex_base, first_instance, instance_count);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::drawIndexedUserData(D3D12_PRIMITIVE_TOPOLOGY top, uint32_t count, uint32_t vertex_stride, const void *vertex_data,
  uint32_t vertex_count, const void *index_data)
{
  auto vertexDataBytes = vertex_count * vertex_stride;
  // vertex data needs to be dword aligned or some GPUs will render garbage
  auto vertexData =
    device.resources.allocatePushMemory(device.getDXGIAdapter(), device.device.get(), vertexDataBytes, sizeof(uint32_t));
  if (!vertexData)
  {
    return;
  }
  auto indexDataBytes = count * sizeof(uint16_t);
  auto indexData = device.resources.allocatePushMemory(device.getDXGIAdapter(), device.device.get(), indexDataBytes, sizeof(uint16_t));
  if (!indexData)
  {
    return;
  }
  memcpy(vertexData.pointer, vertex_data, vertexDataBytes);
  memcpy(indexData.pointer, index_data, indexDataBytes);
  vertexData.flush();
  indexData.flush();

  auto cmd = make_command<CmdDrawIndexedUserData>(top, count, vertex_stride, vertexData, indexData);
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::setComputePipeline(ProgramID program)
{
  auto cmd = make_command<CmdSetComputeProgram>(program);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setGraphicsPipeline(GraphicsProgramID program)
{
  auto cmd = make_command<CmdSetGraphicsProgram>(program);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

namespace
{
// Consoles can do inter-resource copies of buffers, eg the same buffer can be source and destination of the same
// copy operation. So this can be optimized to only return true if source and destination overlap.
// TODO: Can only enable when regular copy of buffer can correctly handle the case when src and dst are the same resource.
bool needs_two_phase_copy(const BufferResourceReference &src, const BufferResourceReference &dst, uint32_t data_size)
{
  // needed for console version to detect overlapping regions.
  G_UNUSED(data_size);
  return src == dst;
}
}

void DeviceContext::copyBuffer(BufferResourceReferenceAndOffset source, BufferResourceReferenceAndOffset dest, uint32_t data_size)
{
  if (needs_two_phase_copy(source, dest, data_size))
  {
    DX12_LOCK_FRONT();
    auto scratchBuffer = device.resources.getTempScratchBufferSpace(device.getDXGIAdapter(), device.getDevice(), data_size, 1);
    auto cmd = make_command<CmdTwoPhaseCopyBuffer>(source, dest.offset, scratchBuffer, data_size);
    commandStream.pushBack(cmd);
    immediateModeExecute();
  }
  else
  {
    auto cmd = make_command<CmdCopyBuffer>(source, dest, data_size);
    DX12_LOCK_FRONT();
    commandStream.pushBack(cmd);
    immediateModeExecute();
  }
}

void DeviceContext::updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest)
{
  G_ASSERTF(HostDeviceSharedMemoryRegion::Source::TEMPORARY == update.source,
    "DX12: DeviceContext::updateBuffer called with wrong cpu paged gpu memory type %u", static_cast<uint32_t>(update.source));
  // flush writes
  update.flush();

  auto cmd = make_command<CmdUpdateBuffer>(update, dest);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4])
{
  auto cmd = make_command<CmdClearBufferFloat>(buffer);
  memcpy(cmd.values, values, sizeof(cmd.values));

  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearBufferInt(BufferResourceReferenceAndClearView buffer, const unsigned values[4])
{
  auto cmd = make_command<CmdClearBufferInt>(buffer);
  memcpy(cmd.values, values, sizeof(cmd.values));

  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::pushEvent(const char *name)
{
  auto length = strlen(name) + 1;
  device.resources.pushEvent(name, name + length - 1);

  auto cmd = make_command<CmdPushEvent>();
  DX12_LOCK_FRONT();
  commandStream.pushBack<CmdPushEvent, const char>(cmd, name, length);
  immediateModeExecute();
}

void DeviceContext::popEvent()
{
  device.resources.popEvent();
  auto cmd = make_command<CmdPopEvent>();
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::updateViewports(dag::ConstSpan<ViewportState> viewports)
{
  commandStream.pushBack(make_command<CmdSetViewports>(), viewports.data(), viewports.size());
  immediateModeExecute();
}

void DeviceContext::clearDepthStencilImage(Image *image, const ImageSubresourceRange &area, const ClearDepthStencilValue &value)
{
  auto cmd = make_command<CmdClearDepthStencilTexture>(image, ImageViewState{}, D3D12_CPU_DESCRIPTOR_HANDLE{}, value);

  const auto format = image->getFormat();
  cmd.viewState.setDSV(false);
  cmd.viewState.setFormat(format);
  cmd.viewState.setMipCount(1);
  cmd.viewState.setArrayCount(1);
  DX12_LOCK_FRONT();

  for (auto a : area.arrayLayerRange)
  {
    cmd.viewState.setArrayBase(a);
    for (auto m : area.mipMapRange)
    {
      cmd.viewState.setMipBase(m);
      cmd.viewDescriptor = device.getImageView(image, cmd.viewState);
      commandStream.pushBack(cmd);
      immediateModeExecute();
    }
  }
}

void DeviceContext::clearColorImage(Image *image, const ImageSubresourceRange &area, const ClearColorValue &value)
{
  auto cmd = make_command<CmdClearColorTexture>(image, ImageViewState{}, D3D12_CPU_DESCRIPTOR_HANDLE{}, value);

  const auto format = image->getFormat();
  cmd.viewState.setRTV();
  cmd.viewState.setFormat(format);
  cmd.viewState.setMipCount(1);
  cmd.viewState.setArrayCount(1);
  DX12_LOCK_FRONT();

  for (auto a : area.arrayLayerRange)
  {
    cmd.viewState.setArrayBase(a);
    for (auto m : area.mipMapRange)
    {
      cmd.viewState.setMipBase(m);
      cmd.viewDescriptor = device.getImageView(image, cmd.viewState);
      commandStream.pushBack(cmd);
      immediateModeExecute();
    }
  }
}

void DeviceContext::copyImage(Image *src, Image *dst, const ImageCopy &copy)
{
  auto cmd = make_command<CmdCopyImage>(src, dst, copy);

  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::flushDraws()
{
  DX12_LOCK_FRONT();

  auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush();
}

bool DeviceContext::flushDrawWhenNoQueries()
{
  DX12_LOCK_FRONT();
  // If any query is active we don't flush
  if (front.activeRangedQueries > 0)
  {
    return false;
  }

  auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush();

  return true;
}

void DeviceContext::blitImage(Image *src, Image *dst, const ImageBlit &region)
{
  DX12_LOCK_FRONT();
  blitImageInternal(src, dst, region, false);
}

void DeviceContext::wait()
{
  EventPointer event;
  {
    DX12_LOCK_FRONT();
    if (!front.eventsPool.empty())
    {
      event = eastl::move(front.eventsPool.back());
      front.eventsPool.pop_back();
    }
  }
  if (!event)
  {
    event.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
  }

  uint64_t progressToWaitFor;
  {
    DX12_LOCK_FRONT();

    progressToWaitFor = front.recordingWorkItemProgress;
    auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
    commandStream.pushBack(cmd);
    immediateModeExecute();
    frontFlush();
  }
  wait_for_frame_progress_with_event(device.queues, progressToWaitFor, event.get(), "wait");
  {
    DX12_LOCK_FRONT();
    front.eventsPool.push_back(eastl::move(event));
  }
}

void DeviceContext::beginSurvey(int name)
{
  Query *q = device.getQueryManager().getQueryPtrFromId(name);
  PredicateInfo pi = device.getQueryManager().getPredicateInfo(q);
  auto cmd = make_command<CmdBeginSurvey>(pi);
  DX12_LOCK_FRONT();
  ++front.activeRangedQueries;
  commandStream.pushBack(cmd);
  suppressImmediateFlush();
  immediateModeExecute();
}

void DeviceContext::endSurvey(int name)
{
  Query *q = device.getQueryManager().getQueryPtrFromId(name);
  PredicateInfo pi = device.getQueryManager().getPredicateInfo(q);
  auto cmd = make_command<CmdEndSurvey>(pi);
  DX12_LOCK_FRONT();
  --front.activeRangedQueries;
  commandStream.pushBack(cmd);
  restoreImmediateFlush();
  immediateModeExecute();
}

void DeviceContext::destroyBuffer(BufferState buffer, const char *name)
{
  if (buffer)
  {
    bool isStillValidRes = device.checkResourceDeleteState("DeviceContext::destroyBuffer", name);

    DX12_LOCK_FRONT();
#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
    auto cmd = make_command<CmdResetBufferState>(buffer);
#endif
    device.resources.freeBufferOnFrameCompletion(eastl::move(buffer), ResourceMemoryHeap::FreeReason::USER_DELETE, isStillValidRes);

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
    commandStream.pushBack(cmd);
    immediateModeExecute();
#endif
  }
}

BufferState DeviceContext::discardBuffer(BufferState to_discared, DeviceMemoryClass memory_class, FormatStore format,
  uint32_t struct_size, bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name)
{
  BufferState result;
  {
    DX12_LOCK_FRONT();
    result = device.resources.discardBuffer(device.getDXGIAdapter(), device.device.get(), eastl::move(to_discared), memory_class,
      format, struct_size, raw_view, struct_view, flags, cflags, name, front.frameIndex,
      device.config.features.test(DeviceFeaturesConfig::DISABLE_BUFFER_SUBALLOCATION), device.shouldNameObjects());
  }

  return result;
}

void DeviceContext::destroyImage(Image *img)
{
  // TODO this should be moved into the heap manager!
#if DX12_USE_ESRAM
  // ESRam regions can be marked as free right away as it is only accessible during GPU time line
  if (img->isEsramTexture())
  {
    if (!img->isAliased())
    {
      device.resources.unmapResource(img->getEsramResource());
    }
    img->resetEsram();
  }
#endif
  DX12_LOCK_FRONT();
  device.resources.destroyTextureOnFrameCompletion(img);
}

void DeviceContext::present(OutputMode mode)
{
#if DX12_RECORD_TIMING_DATA
  auto now = ref_time_ticks();
#endif

  callFrameEndCallbacks();

  DX12_LOCK_FRONT();

#if DX12_RECORD_TIMING_DATA
  auto &frameTiming = front.timingHistory[front.frameIndex % timing_history_length];
  frameTiming.frontendBackendWaitDuration = 0;
  frameTiming.frontendUpdateScreenInterval = now - front.lastPresentTimeStamp;
  front.lastPresentTimeStamp = now;

#if DX12_CAPTURE_AFTER_LONG_FRAMES
  const int64_t frameInterval = ref_time_delta_to_usec(frameTiming.frontendUpdateScreenInterval);
  auto &capInfo = front.captureAfterLongFrames;
  if ((capInfo.thresholdUS > 0) && !capInfo.ignoreNextFrames && (capInfo.captureCountLimit != 0) &&
      (frameInterval > capInfo.thresholdUS))
  {
    time_t rawtime;
    tm *t;
    time(&rawtime);
    t = localtime(&rawtime);

    debug("DX12: Frame %u: interval was %ld us, threshold is %ld us; capturing next frame", front.frameIndex, frameInterval,
      capInfo.thresholdUS);
    eastl::wstring name =
      eastl::wstring(eastl::wstring::CtorSprintf(), L"CaptureAfterLongFrame-%04d.%02d.%02d-%02d.%02d.%02d-%02d.xpix",
        1900 + t->tm_year, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, capInfo.captureId);
    ++capInfo.captureId;
    commandStream.pushBack(make_command<CmdCaptureNextFrames>(capInfo.flags, capInfo.frameCount), name.c_str(), name.size() + 1);
    // Capture takes a lot of time, so next frames will have a big latency; ignore them
    capInfo.ignoreNextFrames = 4 + capInfo.frameCount;
    if (capInfo.captureCountLimit > 0)
      --capInfo.captureCountLimit;
  }
  else if (capInfo.ignoreNextFrames)
    --capInfo.ignoreNextFrames;
#endif

#endif

  ResourceMemoryHeap::CompletedFrameRecordingInfo frameCompleteInfo;
#if _TARGET_PC_WIN && DX12_SUPPORT_RESOURCE_MEMORY_METRICS
  frameCompleteInfo.adapter = device.getDXGIAdapter();
#endif
  frameCompleteInfo.historyIndex = front.recordingLatchedFrame - front.latchedFrameSet;
  device.resources.completeFrameRecording(frameCompleteInfo);

  auto cmd = make_command<CmdPresent>(front.recordingWorkItemProgress, lowlatency::get_current_frame(), mode
#if DX12_RECORD_TIMING_DATA
    ,
    &frameTiming, now
#endif
  );
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush();
  front.frameIndex++;

#if _TARGET_PC_WIN
  if (hasWorkerThread() && OutputMode::PRESENT == mode)
  {
    back.swapchain.signalPresentSentToBackend();
  }
#endif

#if DX12_REPORT_DISCARD_MEMORY_PER_FRAME
  auto cnt = discardBytes.load();
  while (!discardBytes.compare_exchange_strong(cnt, 0))
    ;
  debug("DX12: Discarded %uBytes / %uKiBytes / %uMiBytes", cnt, cnt / 1024, cnt / 1024 / 1024);
#endif

#if DX12_RECORD_TIMING_DATA
  now = ref_time_ticks();
#endif
  if (OutputMode::PRESENT == mode)
  {
    // block until the swapchain gives its okay for the next frame
    front.swapchain.waitForFrameStart();
  }
#if DX12_RECORD_TIMING_DATA
  frameTiming.frontendWaitForSwapchainDuration = ref_time_ticks() - now;
#endif
}

void DeviceContext::changePresentMode(PresentationMode mode)
{
  if (mode == front.swapchain.getPresentationMode())
  {
    return;
  }

  debug("DX12: Swapchain present mode changed to %u, sending mode change to backend...", (uint32_t)mode);
  auto cmd = make_command<CmdChangePresentMode>(mode);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
  front.swapchain.changePresentMode(mode);
}

void DeviceContext::changeSwapchainExtents(Extent2D size)
{
  if (size == front.swapchain.getExtent())
  {
    return;
  }

  debug("DX12: Swapchain extent changed to %u / %u, sending mode change to backend...", size.width, size.height);

  DX12_LOCK_FRONT();
  finishInternal();
  resizeSwapchain(size);
}

#if _TARGET_PC_WIN
void DeviceContext::changeFullscreenExclusiveMode(bool is_exclusive)
{
  // This check is not just a micro-optimization, it is a workaround for a problem:
  // by exiting the game in ex fs with ALT+F4, Windows first deactivates its window, and
  // somehow makes the swapchain lose its data (swapImage) before exiting the app.
  // If ex fs mode is the same as before (as in deactivate), we simply do not update the swapchain.
  // (accessing back is okay, as exclusive mode switch only happens when joined with backend).
  if (back.swapchain.isInExclusiveFullscreenMode() != is_exclusive)
  {
    DX12_LOCK_FRONT();
    finishInternal();
    auto size = front.swapchain.getExtent();
    back.swapchain.changeFullscreenExclusiveMode(is_exclusive);

    resizeSwapchain(size);
  }
}

void DeviceContext::changeFullscreenExclusiveMode(bool is_exclusive, ComPtr<IDXGIOutput> output)
{
  DX12_LOCK_FRONT();
  finishInternal();

  auto size = front.swapchain.getExtent();
  back.swapchain.changeFullscreenExclusiveMode(is_exclusive, eastl::move(output));
  // "For the flip presentation model, after you transition the display state to full screen, you must
  // call ResizeBuffers to ensure that your call to IDXGISwapChain1::Present1 succeeds."
  // Source: https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-setfullscreenstate
  resizeSwapchain(size);
}

HRESULT DeviceContext::getSwapchainDesc(DXGI_SWAP_CHAIN_DESC *out_desc) { return back.swapchain.getDesc(out_desc); }

IDXGIOutput *DeviceContext::getSwapchainOutput() { return back.swapchain.getOutput(); }
#endif

void DeviceContext::shutdownSwapchain()
{
  DX12_LOCK_FRONT();
  finishInternal();

  back.swapchain.prepareForShutdown(device);
  front.swapchain.prepareForShutdown(device);
  back.swapchain.shutdown();
  front.swapchain.shutdown();
}

void DeviceContext::insertTimestampQuery(Query *query)
{
  query->setIssued();
  auto cmd = make_command<CmdInsertTimestampQuery>(query);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::deleteQuery(Query *query)
{
  DX12_LOCK_FRONT();
  G_ASSERT(query != nullptr);
  if (front.recordingLatchedFrame)
    front.recordingLatchedFrame->deletedQueries.push_back(query);
}

void DeviceContext::generateMipmaps(Image *img)
{
  ImageBlit blit;
  const auto &ext = img->getBaseExtent();
  blit.srcOffsets[0].x = 0;
  blit.srcOffsets[0].y = 0;
  blit.srcOffsets[0].z = 0;
  blit.srcOffsets[1].z = 1;

  blit.dstOffsets[0].x = 0;
  blit.dstOffsets[0].y = 0;
  blit.dstOffsets[0].z = 0;
  blit.dstOffsets[1].z = 1;

  const auto mips = img->getMipLevelRange();
  const auto arrays = img->getArrayLayers();

  DX12_LOCK_FRONT();
  for (auto a : arrays)
  {
    blit.srcOffsets[1].x = ext.width;
    blit.srcOffsets[1].y = ext.height;

    blit.srcSubresource.mipLevel = MipMapIndex::make(0);
    blit.srcSubresource.baseArrayLayer = a;

    blit.dstSubresource.baseArrayLayer = a;

    for (blit.dstSubresource.mipLevel = MipMapIndex::make(1); blit.dstSubresource.mipLevel < mips;
         ++blit.dstSubresource.mipLevel, ++blit.srcSubresource.mipLevel)
    {
      blit.dstOffsets[1].x = max(1, blit.srcOffsets[1].x >> 1);
      blit.dstOffsets[1].y = max(1, blit.srcOffsets[1].y >> 1);

      // this will transition the src for the blit into pixel sampling state
      commandStream.pushBack(make_command<CmdMipMapGenSource>(img, blit.srcSubresource.mipLevel, a));
      immediateModeExecute();

      blitImageInternal(img, img, blit, true);

      blit.srcOffsets[1].x = blit.dstOffsets[1].x;
      blit.srcOffsets[1].y = blit.dstOffsets[1].y;
    }

    // to have a uniform state after mip gen, also transition the last mip level to pixel sampling state
    commandStream.pushBack(make_command<CmdMipMapGenSource>(img, blit.srcSubresource.mipLevel, a));
    immediateModeExecute();
  }
}

void DeviceContext::setFramebuffer(Image **image_list, ImageViewState *view_list, bool read_only_depth)
{
  auto cmd = make_command<CmdSetFramebuffer>();
  memcpy(cmd.imageList, image_list, sizeof(cmd.imageList));
  memcpy(cmd.viewList, view_list, sizeof(cmd.viewList));
  for (uint32_t i = 0; i < (Driver3dRenderTarget::MAX_SIMRT + 1); ++i)
  {
    if (image_list[i])
    {
      cmd.viewDescriptors[i] = device.getImageView(image_list[i], view_list[i]);
    }
    else
    {
      cmd.viewDescriptors[i].ptr = 0;
    }
  }
  cmd.readOnlyDepth = read_only_depth;
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::raytraceBuildBottomAccelerationStructure(RaytraceBottomAccelerationStructure *as,
  RaytraceGeometryDescription *desc, uint32_t count, RaytraceBuildFlags flags, bool update,
  BufferResourceReferenceAndAddress scratch_buf)
{

  DX12_LOCK_FRONT();
  commandStream.pushBack<CmdRaytraceBuildBottomAccelerationStructure, D3D12_RAYTRACING_GEOMETRY_DESC,
    RaytraceGeometryDescriptionBufferResourceReferenceSet>(make_command<CmdRaytraceBuildBottomAccelerationStructure>(scratch_buf,
                                                             reinterpret_cast<RaytraceAccelerationStructure *>(as), flags,
                                                             update), //
    count,
    [desc](auto index, auto first, auto second) //
    {
      auto pair = raytraceGeometryDescriptionToGeometryDesc(desc[index]);
      first(pair.first);
      second(pair.second);
    });
  immediateModeExecute();
}

void DeviceContext::raytraceBuildTopAccelerationStructure(RaytraceTopAccelerationStructure *as, BufferReference index_buffer,
  uint32_t index_count, RaytraceBuildFlags flags, bool update, BufferResourceReferenceAndAddress scratch_buf)
{
  auto cmd = make_command<CmdRaytraceBuildTopAccelerationStructure>();
  cmd.scratchBuffer = scratch_buf;
  cmd.as = reinterpret_cast<RaytraceAccelerationStructure *>(as);
  cmd.indexBuffer = index_buffer;
  cmd.indexCount = index_count;
  cmd.flags = flags;
  cmd.update = update;

  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::deleteRaytraceBottomAccelerationStructure(RaytraceBottomAccelerationStructure *desc)
{
  DX12_LOCK_FRONT();
  device.resources.deleteRaytraceBottomAccelerationStructureOnFrameCompletion(reinterpret_cast<RaytraceAccelerationStructure *>(desc));
}

void DeviceContext::deleteRaytraceTopAccelerationStructure(RaytraceTopAccelerationStructure *desc)
{
  DX12_LOCK_FRONT();
  device.resources.deleteRaytraceTopAccelerationStructureOnFrameCompletion(reinterpret_cast<RaytraceAccelerationStructure *>(desc));
}

void DeviceContext::traceRays(BufferResourceReferenceAndRange ray_gen_table, BufferResourceReferenceAndRange miss_table,
  uint32_t miss_stride, BufferResourceReferenceAndRange hit_table, uint32_t hit_stride, BufferResourceReferenceAndRange callable_table,
  uint32_t callable_stride, uint32_t width, uint32_t height, uint32_t depth)
{
  auto cmd = make_command<CmdTraceRays>();
  cmd.rayGenTable = ray_gen_table;
  cmd.missTable = miss_table;
  cmd.hitTable = hit_table;
  cmd.callableTable = callable_table;
  cmd.missStride = miss_stride;
  cmd.hitStride = hit_stride;
  cmd.callableStride = callable_stride;
  cmd.width = width;
  cmd.height = height;
  cmd.depth = depth;
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
void DeviceContext::setRaytracePipeline(ProgramID program)
{
  auto cmd = make_command<CmdSetRaytraceProgram>(program);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
void DeviceContext::setRaytraceAccelerationStructure(uint32_t stage, size_t unit, RaytraceAccelerationStructure *as)
{
  auto cmd = make_command<CmdSetRaytraceAccelerationStructure>(stage, static_cast<uint32_t>(unit), as);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
#endif

void DeviceContext::beginConditionalRender(int name)
{
  Query *q = device.getQueryManager().getQueryPtrFromId(name);
  if (q)
    q->setIssued();
  PredicateInfo pi = device.getQueryManager().getPredicateInfo(q);
  auto cmd = make_command<CmdBeginConditionalRender>(pi);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::endConditionalRender()
{
  auto cmd = make_command<CmdEndConditionalRender>();
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::addVertexShader(ShaderID id, eastl::unique_ptr<VertexShaderModule> shader)
{
  auto cmd = make_command<CmdAddVertexShader>(id, shader.release());
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::addPixelShader(ShaderID id, eastl::unique_ptr<PixelShaderModule> shader)
{
  auto cmd = make_command<CmdAddPixelShader>(id, shader.release());
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::removeVertexShader(ShaderID blob_id)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdRemoveVertexShader>(blob_id));
  immediateModeExecute();
}

void DeviceContext::removePixelShader(ShaderID blob_id)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdRemovePixelShader>(blob_id));
  immediateModeExecute();
}

void DeviceContext::addGraphicsProgram(GraphicsProgramID program, ShaderID vs, ShaderID ps)
{
  auto cmd = make_command<CmdAddGraphicsProgram>(program, vs, ps);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::addComputeProgram(ProgramID id, eastl::unique_ptr<ComputeShaderModule> csm)
{
  auto cmd = make_command<CmdAddComputeProgram>(id, csm.release());
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
void DeviceContext::removeProgram(ProgramID program)
{
  auto cmd = make_command<CmdDeleteProgram>(program);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::addRaytraceProgram(ProgramID program, uint32_t max_recursion, uint32_t shader_count, const ShaderID *shaders,
  uint32_t group_count, const RaytraceShaderGroup *groups)
{
  auto cmd = make_command<CmdAddRaytraceProgram>(program);
  DX12_LOCK_FRONT();

  cmd.shaders = shaders;
  cmd.shaderGroups = groups;
  cmd.shaderCount = shader_count;
  cmd.groupCount = group_count;
  cmd.maxRecursion = max_recursion;
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::copyRaytraceShaderGroupHandlesToMemory(ProgramID prog, uint32_t first_group, uint32_t group_count, uint32_t size,
  BufferResourceReference buffer, uint32_t offset)
{
  // TODO
  G_UNUSED(prog);
  G_UNUSED(first_group);
  G_UNUSED(group_count);
  G_UNUSED(size);
  G_UNUSED(buffer);
  G_UNUSED(offset);
  /*  BufferSubAllocation tempBuffer;
    void *memory;
    if (buffer->hasMappedMemory())
    {
      memory = buffer->dataPointer(offset);
    }
    else
    {
      // if memory is not host visible then use update buffer temp memory for it
      tempBuffer = allocateTempUpdateBufferBuffer(size);
      memory = tempBuffer.buffer->dataPointer(tempBuffer.offset);
    }
    auto cmd = make_command<CmdCopyRaytraceShaderGroupHandlesToMemory>(prog, first_group, group_count, size, memory);
    commandStream.pushBack(cmd);
    immediateModeExecute();

    if (tempBuffer)
    {
      copyBuffer(tempBuffer.buffer, buffer, tempBuffer.offset, offset, size);
    }*/
}
#endif

void DeviceContext::placeAftermathMarker(const char *name)
{
  auto length = strlen(name) + 1;
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdPlaceAftermathMarker>(), name, length);
  immediateModeExecute();
}

void DeviceContext::updateVertexShaderName(ShaderID shader, const char *name)
{
  auto length = strlen(name) + 1;
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdUpdateVertexShaderName>(shader), name, length);
  immediateModeExecute();
}

void DeviceContext::updatePixelShaderName(ShaderID shader, const char *name)
{
  auto length = strlen(name) + 1;
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdUpdatePixelShaderName>(shader), name, length);
  immediateModeExecute();
}

void DeviceContext::setImageResourceState(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresouceId> range)
{
  DX12_LOCK_FRONT();
  setImageResourceStateNoLock(state, range);
}

void DeviceContext::setImageResourceStateNoLock(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresouceId> range)
{
  auto cmd = make_command<CmdInitializeTextureState>(state, range);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearUAVTexture(Image *image, ImageViewState view, const unsigned values[4])
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdClearUAVTextureI>(image, view, device.getImageView(image, view));
  memcpy(cmd.values, values, sizeof(cmd.values));
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearUAVTexture(Image *image, ImageViewState view, const float values[4])
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdClearUAVTextureF>(image, view, device.getImageView(image, view));
  memcpy(cmd.values, values, sizeof(cmd.values));
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setGamma(float power)
{
  auto cmd = make_command<CmdSetGamma>(power);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setComputeRootConstant(uint32_t offset, uint32_t size)
{
  auto cmd = make_command<CmdSetComputeRootConstant>(offset, size);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setVertexRootConstant(uint32_t offset, uint32_t size)
{
  auto cmd = make_command<CmdSetVertexRootConstant>(offset, size);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::setPixelRootConstant(uint32_t offset, uint32_t size)
{
  auto cmd = make_command<CmdSetPixelRootConstant>(offset, size);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::setRaytraceRootConstant(uint32_t offset, uint32_t size)
{
  auto cmd = make_command<CmdSetRaytraceRootConstant>(offset, size);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
#endif

#if _TARGET_PC_WIN
void DeviceContext::preRecovery()
{
#if DAGOR_DBGLEVEL > 0
  dumpCommandLog();
#endif

  mutex.lock();
  finishInternal();

  // simply set to zero descriptor for now, we don't have a valid one for that
  back.sharedContextState.onFrameStateInvalidate({0});
  back.sharedContextState.purgeAllBindings();

  back.sharedContextState.frames.walkAll([=](auto &frame) //
    { frame.preRecovery(device.queues, device.pipeMan); });

  back.sharedContextState.bindlessSetManager.preRecovery();

  back.sharedContextState.drawIndirectSignatures.reset();
  back.sharedContextState.drawIndexedIndirectSignatures.reset();
  back.sharedContextState.dispatchIndirectSignature.Reset();
  back.constBufferStreamDescriptors.shutdown();

  // clear out all vectors
  for (auto &&frame : front.latchedFrameSet)
    frame = {};
  front.recordingLatchedFrame = nullptr;
  front.recordingWorkItemProgress = 1;
  front.nextWorkItemProgress = 2;
  front.completedFrameProgress = 0;
  front.frontendFinishedWorkItem.store(0);

  front.swapchain.preRecovery();
  back.swapchain.preRecovery();
}

void DeviceContext::recover(const eastl::vector<D3D12_CPU_DESCRIPTOR_HANDLE> &unbounded_samplers)
{
  back.sharedContextState.frames.walkAll([=](auto &frame) //
    { frame.recover(device.device.get()); });
  back.sharedContextState.bindlessSetManager.init(device.device.get());

  front.recordingLatchedFrame = &front.latchedFrameSet[0];
  back.sharedContextState.bindlessSetManager.recover(device.device.get(), unbounded_samplers);
  back.constBufferStreamDescriptors.init(device.device.get());

  ResourceMemoryHeap::BeginFrameRecordingInfo frameRecodingInfo;
  frameRecodingInfo.historyIndex = 0;
  device.resources.beginFrameRecording(frameRecodingInfo);

  makeReadyForFrame(OutputMode::PRESENT);

  mutex.unlock();
}
#endif

void DeviceContext::deleteTexture(BaseTex *tex)
{
  STORE_RETURN_ADDRESS();
  device.bindlessManager.onTextureDeletion(*this, tex, device.nullResourceTable);
  DX12_LOCK_FRONT();
  device.resources.deleteTextureObjectOnFrameCompletion(tex);
}

void DeviceContext::freeMemory(HostDeviceSharedMemoryRegion allocation)
{
  device.resources.freeHostDeviceSharedMemoryRegionOnFrameCompletion(allocation);
}

void DeviceContext::freeMemoryOfUploadBuffer(HostDeviceSharedMemoryRegion allocation)
{
  device.resources.freeHostDeviceSharedMemoryRegionForUploadBufferOnFrameCompletion(allocation);
}

void DeviceContext::uploadToBuffer(BufferResourceReferenceAndRange target, HostDeviceSharedMemoryRegion memory, size_t m_offset)
{
  auto cmd = make_command<CmdHostToDeviceMemoryCopy>(target, memory, m_offset);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
void DeviceContext::readBackFromBuffer(HostDeviceSharedMemoryRegion memory, size_t m_offset, BufferResourceReferenceAndRange source)
{
  auto cmd = make_command<CmdBufferReadBack>(source, memory, m_offset);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::uploadToImage(Image *target, const BufferImageCopy *regions, uint32_t region_count,
  HostDeviceSharedMemoryRegion memory, DeviceQueueType queue, bool is_discard)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdUploadTexture>(target, memory, queue, is_discard), regions, region_count);
  immediateModeExecute();
}

uint64_t DeviceContext::readBackFromImage(HostDeviceSharedMemoryRegion memory, const BufferImageCopy *regions, uint32_t region_count,
  Image *source, DeviceQueueType queue)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdTextureReadBack>(source, memory, queue), regions, region_count);
  immediateModeExecute();

  return front.recordingWorkItemProgress;
}

void DeviceContext::removeGraphicsProgram(GraphicsProgramID program)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdDeleteGraphicsProgram>(program));
  immediateModeExecute();
}

void DeviceContext::registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state)
{
  auto cmd = make_command<CmdRegisterStaticRenderState>(ident, state);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setStaticRenderState(StaticRenderStateID ident)
{
  auto cmd = make_command<CmdSetStaticRenderState>(ident);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::beginVisibilityQuery(Query *q)
{
  q->setIssued();
  auto cmd = make_command<CmdBeginVisibilityQuery>(q);
  DX12_LOCK_FRONT();
  ++front.activeRangedQueries;
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::endVisibilityQuery(Query *q)
{
  auto cmd = make_command<CmdEndVisibilityQuery>(q);
  DX12_LOCK_FRONT();
  --front.activeRangedQueries;
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

#if _TARGET_XBOX
void DeviceContext::suspendExecution()
{
  // block creation of resources
  device.resources.enterSuspended();

  debug("DX12: DeviceContext::suspendExecution called...");
  // First lock the context, so no one else can issue commands
  getFrontGuard().lock();
#if !FIXED_EXECUTION_MODE
  if (ExecutionMode::CONCURRENT == executionMode)
#endif
  {
    debug("DX12: Sending suspend request to worker thread...");
    // Now tell the worker to enter suspended state, it will signal enteredSuspendedStateEvent and
    // wait for resumeExecutionEvent to be signaled
    commandStream.pushBack(make_command<CmdEnterSuspendState>());
    WaitForSingleObject(enteredSuspendedStateEvent.get(), INFINITE);
    debug("DX12: Worker is now in suspend...");
  }

  debug("DX12: Sending suspend command to API...");
  // Engine requests suspended state
  // Tell DX12 driver to safe the current GPU and driver state
  device.queues.enterSuspendedState();
  debug("DX12: API suspended...");
}
void DeviceContext::resumeExecution()
{
  debug("DX12: Sending resume command to API...");
  // If we end up here, we got the resume signal
  device.queues.leaveSuspendedState();
  debug("DX12: API resumed...");

#if _TARGET_XBOX
  debug("DX12: Restoring swapchain configuration...");
  // We have to restore swapchain state
  back.swapchain.restoreAfterSuspend(device.device.get());
#endif

#if !FIXED_EXECUTION_MODE
  if (ExecutionMode::CONCURRENT == executionMode)
#endif
  {
    debug("DX12: Sending worker resume request to worker thread...");
    // Tell the worker to resume execution
    SetEvent(resumeExecutionEvent.get());
  }
  // Unlock context so everyone can continue issuing commands
  getFrontGuard().unlock();

  // unblock creation of resources
  device.resources.leaveSuspended();
}
void DeviceContext::updateFrameInterval(int32_t freq_level)
{
  auto cmd = make_command<CmdUpdateFrameInterval>(freq_level);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
void DeviceContext::resummarizeHtile(ID3D12Resource *depth)
{
  auto cmd = make_command<CmdResummarizeHtile>(depth);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
#endif

// TODO: flushDraws is very similar
void DeviceContext::waitForProgress(uint64_t progress)
{
  bool needsFlush = false;
  EventPointer event;
  {
    DX12_LOCK_FRONT();
    needsFlush = progress == front.recordingWorkItemProgress;
    if (needsFlush && !front.eventsPool.empty())
    {
      event = eastl::move(front.eventsPool.back());
      front.eventsPool.pop_back();
    }
  }
  if (needsFlush)
  {
    if (!event)
    {
      event.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
    }

    DX12_LOCK_FRONT();
    // have to check again, it is possible that a different thread did a flush already.
    if (progress == front.recordingWorkItemProgress)
    {
      auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
      commandStream.pushBack(cmd);
      immediateModeExecute();
      frontFlush();
    }
  }
  wait_for_frame_progress_with_event(device.queues, progress, event.get(), "waitForProgress");
  if (event)
  {
    DX12_LOCK_FRONT();
    front.eventsPool.push_back(eastl::move(event));
  }
}

#if !_TARGET_XBOXONE
void DeviceContext::setVariableRateShading(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner,
  D3D12_SHADING_RATE_COMBINER ps_combiner)
{
  auto cmd = make_command<CmdSetVariableRateShading>(rate, vs_combiner, ps_combiner);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setVariableRateShadingTexture(Image *texture)
{
  auto cmd = make_command<CmdSetVariableRateShadingTexture>(texture);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
#endif

void DeviceContext::registerInputLayout(InputLayoutID ident, const InputLayout &ptr)
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdRegisterInputLayout>(ident, ptr);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

#ifndef _TARGET_XBOX
static void check_dlss_state(DlssState state, const char *setting_name, int quality)
{
  switch (state) // -V785
  {
    case DlssState::SUPPORTED:
      debug("DX12: DLSS is supported on this hardware, but it was not enabled. video/%s: %d", setting_name, quality);
      break;
    case DlssState::READY: debug("DX12: DLSS is supported and is ready to go. video/%s: %d", setting_name, quality); break;
    case DlssState::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE: debug("DX12: DLSS is not supported on this hardware."); break;
    case DlssState::NOT_SUPPORTED_OUTDATED_VGA_DRIVER: debug("DX12: DLSS is not supported on because of outdated VGA drivers."); break;
    case DlssState::NOT_SUPPORTED_32BIT: debug("DX12: DLSS is not supported in 32bit builds."); break;
    case DlssState::DISABLED: debug("DX12 drv: DLSS is disabled for drv3d_DX12 lib."); break;
    case DlssState::NGX_INIT_ERROR_NO_APP_ID:
      debug("DX12: Couldn't initialize NGX because there is no Nvidia AppID set in game specific settings.blk");
      break;
    default: logerr("DX12: Unexpected DLSS state after initialization: %d", int(state)); break;
  }
}
#endif

template <typename T>
static Extent2D to_extent_2d(T size)
{
  return Extent2D({static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height)});
};

void DeviceContext::initXeSS()
{
#if !_TARGET_XBOX
  if (xessWrapper.xessInit(static_cast<void *>(device.device.as<ID3D12Device>())))
  {
    int xessQuality = dgs_get_settings()->getBlockByNameEx("video")->getInt("xessQuality", -1);
    if (xessQuality >= 0)
    {
      Extent2D targetResolution = stereo_config_callback && stereo_config_callback->desiredStereoRender()
                                    ? to_extent_2d(stereo_config_callback->desiredRendererSize())
                                    : front.swapchain.getExtent();
      xessWrapper.xessCreateFeature(xessQuality, targetResolution.width, targetResolution.height);
    }
  }
#endif
}

void DeviceContext::initFsr2()
{
#if !_TARGET_XBOX
  if (fsr2Wrapper.init(static_cast<void *>(device.device.as<ID3D12Device>())))
  {
    int fsr2Quality = dgs_get_settings()->getBlockByNameEx("video")->getInt("fsr2Quality", -1);
    const int FSR2_AA_MODE_IDX = 7;
    bool fsrEnabled = dgs_get_settings()->getBlockByNameEx("video")->getInt("antiAliasingMode", -1) == FSR2_AA_MODE_IDX;
    if (fsrEnabled && fsr2Quality >= 0)
    {
      Extent2D targetResolution = stereo_config_callback && stereo_config_callback->desiredStereoRender()
                                    ? to_extent_2d(stereo_config_callback->desiredRendererSize())
                                    : front.swapchain.getExtent();
      fsr2Wrapper.createContext(targetResolution.width, targetResolution.height, fsr2Quality);
    }
  }
#endif
}

void DeviceContext::initNgx(bool stereo_render)
{
#if _TARGET_XBOX
  G_UNUSED(stereo_render);
#else
  int dlssQuality = dgs_get_settings()->getBlockByNameEx("video")->getInt("dlssQuality", -1);
  if (ngxWrapper.init(static_cast<void *>(device.device.as<ID3D12Device>()), get_log_directory()))
  {
    ngxWrapper.checkDlssSupport();
    if (dlssQuality >= 0)
    {
      Extent2D targetResolution = stereo_config_callback && stereo_config_callback->desiredStereoRender()
                                    ? to_extent_2d(stereo_config_callback->desiredRendererSize())
                                    : front.swapchain.getExtent();
      createDlssFeature(dlssQuality, targetResolution, stereo_render);
      wait();
    }
  }
  check_dlss_state(ngxWrapper.getDlssState(), "dlssQuality", dlssQuality);
#endif
}

void DeviceContext::shutdownNgx()
{
#if !_TARGET_XBOX
  ngxWrapper.shutdown(static_cast<void *>(device.device.as<ID3D12Device>()));
#endif
}

void DeviceContext::shutdownXess() { xessWrapper.xessShutdown(); }

void DeviceContext::shutdownFsr2()
{
#if !_TARGET_XBOX
  fsr2Wrapper.shutdown();
#endif
}

void DeviceContext::createDlssFeature(int dlss_quality, Extent2D output_resolution, bool stereo_render)
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdCreateDlssFeature>(dlss_quality, output_resolution, stereo_render);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::releaseDlssFeature()
{
  auto cmd = make_command<CmdReleaseDlssFeature>();
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::executeDlss(const DlssParams &dlss_params, int view_index)
{
  DlssParamsDx12 dlssParams;
  dlssParams.inColor = cast_to_texture_base(dlss_params.inColor)->getDeviceImage();
  dlssParams.inDepth = cast_to_texture_base(dlss_params.inDepth)->getDeviceImage();
  dlssParams.inMotionVectors = cast_to_texture_base(dlss_params.inMotionVectors)->getDeviceImage();
  dlssParams.inExposure = dlss_params.inExposure ? cast_to_texture_base(dlss_params.inExposure)->getDeviceImage() : nullptr;
  dlssParams.inSharpness = dlss_params.inSharpness;
  dlssParams.inJitterOffsetX = dlss_params.inJitterOffsetX;
  dlssParams.inJitterOffsetY = dlss_params.inJitterOffsetY;
  dlssParams.outColor = cast_to_texture_base(dlss_params.outColor)->getDeviceImage();
  dlssParams.inMVScaleX = dlss_params.inMVScaleX;
  dlssParams.inMVScaleY = dlss_params.inMVScaleY;
  dlssParams.inColorDepthOffsetX = dlss_params.inColorDepthOffsetX;
  dlssParams.inColorDepthOffsetY = dlss_params.inColorDepthOffsetY;

  auto cmd = make_command<CmdExecuteDlss>(dlssParams, view_index);

  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::executeXess(const XessParams &params)
{
  XessParamsDx12 xessParams;
  xessParams.inColor = cast_to_texture_base(params.inColor)->getDeviceImage();
  xessParams.inDepth = cast_to_texture_base(params.inDepth)->getDeviceImage();
  xessParams.inMotionVectors = cast_to_texture_base(params.inMotionVectors)->getDeviceImage();
  xessParams.inJitterOffsetX = params.inJitterOffsetX;
  xessParams.inJitterOffsetY = params.inJitterOffsetY;
  xessParams.outColor = cast_to_texture_base(params.outColor)->getDeviceImage();
  xessParams.inInputWidth = params.inInputWidth;
  xessParams.inInputHeight = params.inInputHeight;
  xessParams.inColorDepthOffsetX = params.inColorDepthOffsetX;
  xessParams.inColorDepthOffsetY = params.inColorDepthOffsetY;

  auto cmd = make_command<CmdExecuteXess>(xessParams);

  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::executeFSR2(const Fsr2Params &params)
{
  auto cast_to_image = [](BaseTexture *src) {
    if (src)
      return cast_to_texture_base(src)->getDeviceImage();
    return (Image *)nullptr;
  };
  Fsr2ParamsDx12 paramsDx12{};
  paramsDx12.inColor = cast_to_image(params.color);
  paramsDx12.inDepth = cast_to_image(params.depth);
  paramsDx12.inMotionVectors = cast_to_image(params.motionVectors);
  paramsDx12.outColor = cast_to_image(params.output);
  paramsDx12.frameTimeDelta = params.frameTimeDelta;
  paramsDx12.sharpness = params.sharpness;
  paramsDx12.jitterOffsetX = params.jitterOffsetX;
  paramsDx12.jitterOffsetY = params.jitterOffsetY;
  paramsDx12.motionVectorScaleX = params.motionVectorScaleX;
  paramsDx12.motionVectorScaleY = params.motionVectorScaleY;
  paramsDx12.renderSizeX = params.renderSizeX;
  paramsDx12.renderSizeY = params.renderSizeY;
  paramsDx12.cameraNear = params.cameraNear;
  paramsDx12.cameraFar = params.cameraFar;
  paramsDx12.cameraFovAngleVertical = params.cameraFovAngleVertical;
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdDispatchFSR2>(paramsDx12);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bufferBarrier(BufferResourceReference buffer, ResourceBarrier barrier, GpuPipeline queue)
{
  auto cmd = make_command<CmdBufferBarrier>(buffer, barrier, queue);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::textureBarrier(Image *tex, SubresourceRange sub_res_range, uint32_t tex_flags, ResourceBarrier barrier,
  GpuPipeline queue, bool force_barrier)
{
  auto cmd = make_command<CmdTextureBarrier>(tex, sub_res_range, tex_flags, barrier, queue, force_barrier);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::beginCapture(UINT flags, LPCWSTR name)
{
  auto length = wcslen(name) + 1;
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdBeginCapture>(flags), name, length);
  immediateModeExecute();
}

void DeviceContext::endCapture()
{
  auto cmd = make_command<CmdEndCapture>();
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::captureNextFrames(UINT flags, LPCWSTR name, int frame_count)
{
  auto length = wcslen(name) + 1;
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdCaptureNextFrames>(flags, frame_count), name, length);
  immediateModeExecute();
}

#if DX12_CAPTURE_AFTER_LONG_FRAMES
void DeviceContext::captureAfterLongFrames(int64_t frame_interval_threshold_us, int frames, int capture_count_limit, UINT flags)
{
  DX12_LOCK_FRONT();
  front.captureAfterLongFrames.thresholdUS = frame_interval_threshold_us;
  front.captureAfterLongFrames.frameCount = frames;
  front.captureAfterLongFrames.captureCountLimit = capture_count_limit;
  front.captureAfterLongFrames.flags = flags;
}
#endif

void DeviceContext::writeDebugMessage(const char *msg, intptr_t msg_length, intptr_t severity)
{
  if (msg_length <= 0)
    msg_length = strlen(msg);
  DX12_LOCK_FRONT();
  // a bit awkward, first allocate space + 1, then later copy over with
  // and append zero terminator to be sure to not have a accidentally out of bounds read.
  commandStream.pushBack(make_command<CmdWriteDebugMessage>(static_cast<int>(severity)), msg_length + 1,
    [msg, msg_length](auto i) -> char //
    {
      if (i < msg_length)
        return msg[i];
      return '\0';
    });
  immediateModeExecute();
}

void DeviceContext::bindlessSetResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  auto cmd = make_command<CmdBindlessSetResourceDescriptor>(slot, descriptor);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bindlessSetResourceDescriptor(uint32_t slot, Image *texture, ImageViewState view)
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdBindlessSetTextureDescriptor>(slot, texture, device.getImageView(texture, view));
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bindlessSetSamplerDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  auto cmd = make_command<CmdBindlessSetSamplerDescriptor>(slot, descriptor);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bindlessCopyResourceDescriptors(uint32_t src, uint32_t dst, uint32_t count)
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdBindlessCopyResourceDescriptors>(src, dst, count);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::finish()
{
  DX12_LOCK_FRONT();
  finishInternal();
}

void drv3d_dx12::DeviceContext::registerFrameCompleteEvent(os_event_t event)
{
  auto cmd = make_command<CmdRegisterFrameCompleteEvent>(event);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void drv3d_dx12::DeviceContext::registerFrameEventCallbacks(FrameEvents *callback, bool useFront)
{
  DX12_LOCK_FRONT();
  if (useFront)
  {
    callback->beginFrameEvent();
    front.frameEventCallbacks.emplace_back(callback);
  }
  else
  {
    auto cmd = make_command<CmdRegisterFrameEventsCallback>(callback);
    commandStream.pushBack(cmd);
    immediateModeExecute();
  }
}

void drv3d_dx12::DeviceContext::callFrameEndCallbacks()
{
  for (FrameEvents *callback : front.frameEventCallbacks)
    callback->endFrameEvent();
  front.frameEventCallbacks.clear();
}

void drv3d_dx12::DeviceContext::closeFrameEndCallbacks()
{
  // signaling end of frame before deleting reference to callback
  // to make sure it doesn't remain open waiting for frame end

  for (FrameEvents *callback : front.frameEventCallbacks)
    callback->endFrameEvent();
  front.frameEventCallbacks.clear();
  for (FrameEvents *callback : back.frameEventCallbacks)
    callback->endFrameEvent();
  back.frameEventCallbacks.clear();
}

void DeviceContext::beginCPUTextureAccess(Image *image)
{
  auto cmd = make_command<CmdBeginCPUTextureAccess>(image);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::endCPUTextureAccess(Image *image)
{
  auto cmd = make_command<CmdEndCPUTextureAccess>(image);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::addSwapchainView(Image *image, ImageViewInfo info)
{
  // caller already holds the lock
  auto cmd = make_command<CmdAddSwapchainView>(image, info);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::pushBufferUpdate(BufferResourceReferenceAndOffset buffer, const void *data, uint32_t data_size)
{
  auto update = device.resources.allocateUploadRingMemory(device.getDXGIAdapter(), device.device.get(), data_size, 1);
  if (!update)
  {
    return;
  }
  memcpy(update.pointer, data, data_size);
  update.flush();
  auto cmd = make_command<CmdUpdateBuffer>(update, buffer);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::updateFenceProgress()
{
  for (auto &&frame : front.latchedFrameSet)
  {
    if (frame.progress > device.queues.checkFrameProgress())
    {
      continue;
    }

    if (frame.progress)
    {
      tidyFrame(frame);
    }
  }
}

#if _TARGET_PC_WIN
void DeviceContext::onSwapchainSwapCompleted()
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdOnSwapchainSwapCompleted>());
  immediateModeExecute();
}
#endif

namespace
{
uint32_t from_byte_offset_to_page_offset(uint64_t offset)
{
  G_ASSERT(offset % D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT == 0);
  return static_cast<uint32_t>(offset / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
}
}

void DeviceContext::mapTileToResource(BaseTex *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count)
{
  ResourceMemory memory;
  if (heap)
  {
    memory = device.resources.getUserHeapMemory(heap);
    G_ASSERTF(memory, "DX12: getUserHeapMemory for user heap %p returned empty memory range", heap);
    if (!memory)
    {
      return;
    }
  }

  DX12_LOCK_FRONT();
#if _TARGET_PC_WIN
  commandStream.pushBack(make_command<CmdBeginTileMapping>(tex->getDeviceImage(), memory.getHeap(),
    from_byte_offset_to_page_offset(memory.getOffset()), mapping_count));
#else
  commandStream.pushBack(make_command<CmdBeginTileMapping>(tex->getDeviceImage(), memory.getAddress(), memory.size(), mapping_count));
#endif
  immediateModeExecute();

  constexpr size_t batchSize = 1000;
  auto cmd = make_command<CmdAddTileMappings>();
  auto fullBatches = mapping_count / batchSize;
  auto extraBatch = mapping_count % batchSize;
  for (size_t i = 0; i < fullBatches; ++i, mapping += batchSize)
  {
    commandStream.pushBack<CmdAddTileMappings, const TileMapping>(cmd, mapping, batchSize);
    immediateModeExecute();
  }
  if (extraBatch)
  {
    commandStream.pushBack<CmdAddTileMappings, const TileMapping>(cmd, mapping, extraBatch);
    immediateModeExecute();
  }

  commandStream.pushBack(make_command<CmdEndTileMapping>());
  immediateModeExecute();
}

void DeviceContext::freeUserHeap(::ResourceHeap *ptr)
{
  DX12_LOCK_FRONT();
  device.resources.freeUserHeapOnFrameCompletion(ptr);
}

void DeviceContext::activateBuffer(BufferResourceReferenceAndAddressRangeWithClearView buffer, const ResourceMemory &memory,
  ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdActivateBuffer>(buffer, memory, action, value, gpu_pipeline));
  immediateModeExecute();
}

namespace
{
ResourceBarrier activation_action_state_target_base_barrier(ResourceActivationAction action)
{
  switch (action)
  {
    case ResourceActivationAction::REWRITE_AS_COPY_DESTINATION: return RB_RW_COPY_DEST;
    case ResourceActivationAction::REWRITE_AS_RTV_DSV:
    case ResourceActivationAction::CLEAR_AS_RTV_DSV:
    case ResourceActivationAction::DISCARD_AS_RTV_DSV: return RB_RW_RENDER_TARGET;
    case ResourceActivationAction::REWRITE_AS_UAV:
    case ResourceActivationAction::CLEAR_F_AS_UAV:
    case ResourceActivationAction::CLEAR_I_AS_UAV:
    case ResourceActivationAction::DISCARD_AS_UAV: return RB_RW_UAV | RB_STAGE_ALL_SHADERS;
  }
  return ResourceBarrier::RB_NONE;
}
bool is_same_state_target(ResourceActivationAction action_a, ResourceActivationAction action_b)
{
  static constexpr ResourceBarrier relevant_bits = RB_RW_COPY_DEST | RB_RW_RENDER_TARGET | RB_RW_UAV;
  // Same target barriers mean same target state
  auto barrierA = activation_action_state_target_base_barrier(action_a);
  auto barrierB = activation_action_state_target_base_barrier(action_b);
  // This way we save one bit wise and we compare to zero which is free
  return 0 == (relevant_bits & (barrierA ^ barrierB));
}
}

void DeviceContext::activateTexture(BaseTex *texture, ResourceActivationAction action, const ResourceClearValue &value,
  GpuPipeline gpu_pipeline)
{
  auto image = texture->getDeviceImage();
  ResourceActivationAction originalAction = action;
  ImageViewState viewState;
  viewState.isCubemap = 0;
  viewState.setMipCount(1);

  // When a texture can be used as RTV and a discard as UAV is requested, we need to patch it to RTV and then add a barrier to
  // transition to UAV state. This is because DX12 expected RTV resources to be discarded as RTV on the graphics queue.
  if ((GpuPipeline::GRAPHICS == gpu_pipeline) && texture->isRenderTarget())
  {
    if (ResourceActivationAction::DISCARD_AS_UAV == action)
    {
      action = ResourceActivationAction::DISCARD_AS_RTV_DSV;
    }
  }

  DX12_LOCK_FRONT();
  if ((ResourceActivationAction::CLEAR_F_AS_UAV == action) || (ResourceActivationAction::CLEAR_I_AS_UAV == action))
  {
    viewState.setUAV();

    viewState.setFormat(image->getFormat().getLinearVariant());
    viewState.setArrayRange(image->getArrayLayers());
    viewState.isArray = image->getArrayLayers().count() > 1;

    for (auto m : image->getMipLevelRange())
    {
      viewState.setMipBase(m);
      commandStream.pushBack(
        make_command<CmdActivateTexture>(image, action, value, viewState, device.getImageView(image, viewState), gpu_pipeline));
      immediateModeExecute();
    }
  }
  else if (ResourceActivationAction::CLEAR_AS_RTV_DSV == action)
  {
    auto format = image->getFormat();
    if (format.isColor())
    {
      viewState.setRTV();
    }
    else
    {
      viewState.setDSV(false);
    }

    viewState.setFormat(format);

    for (auto a : image->getArrayLayers())
    {
      viewState.setSingleArrayRange(a);
      for (auto m : image->getMipLevelRange())
      {
        viewState.setMipBase(m);
        commandStream.pushBack(
          make_command<CmdActivateTexture>(image, action, value, viewState, device.getImageView(image, viewState), gpu_pipeline));
        immediateModeExecute();
      }
    }
  }
  else
  {
    viewState.setSingleArrayRange(ArrayLayerIndex::make(0));
    viewState.setMipBase(MipMapIndex::make(0));
    commandStream.pushBack(
      make_command<CmdActivateTexture>(image, action, value, viewState, D3D12_CPU_DESCRIPTOR_HANDLE{}, gpu_pipeline));
    immediateModeExecute();
  }

  // keep the context lock in this section as textureBarrier does not take one by it self
  if (originalAction != action)
  {
    if (!is_same_state_target(originalAction, action))
    {
      auto barrier = activation_action_state_target_base_barrier(originalAction);

      textureBarrier(image, image->getSubresourceRange(), texture->cflg, barrier, gpu_pipeline, true);
    }
  }
}


void DeviceContext::deactivateBuffer(BufferResourceReferenceAndAddressRange buffer, const ResourceMemory &memory,
  GpuPipeline gpu_pipeline)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdDeactivateBuffer>(buffer, memory, gpu_pipeline));
  immediateModeExecute();
}

void DeviceContext::deactivateTexture(Image *tex, GpuPipeline gpu_pipeline)
{
  G_ASSERTF_RETURN(nullptr != tex, , "DX12: Seems that a deleted texture was deactivated.");
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdDeactivateTexture>(tex, gpu_pipeline));
  immediateModeExecute();
}

void DeviceContext::aliasFlush(GpuPipeline gpu_pipeline)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdAliasFlush>(gpu_pipeline));
  immediateModeExecute();
}

HostDeviceSharedMemoryRegion DeviceContext::allocatePushMemory(uint32_t size, uint32_t alignment)
{
  DX12_LOCK_FRONT();
  return device.resources.allocatePushMemory(device.getDXGIAdapter(), device.device.get(), size, alignment);
}

void DeviceContext::WorkerThread::execute()
{
  TIME_PROFILE_THREAD(getCurrentThreadName());
#if _TARGET_XBOX
  xbox::make_thread_time_sensitive();
#endif

  ctx.replayCommandsConcurrently(terminating);
}

namespace
{
struct BufferWriter
{
  char *buffer = nullptr;
  size_t offset = 0;
  size_t size = 0;

  BufferWriter(char *buf, size_t sz) : buffer{buf}, size{sz} {}


  void write(eastl::string_view text)
  {
    size_t spaceLeft = size - offset - 1;
    auto copyCount = min(text.length(), spaceLeft);
    memcpy(&buffer[offset], text.data(), copyCount);
    offset += copyCount;
  }

  void write(const char *text) { write(eastl::string_view{text}); }

  size_t finish()
  {
    size_t len = min(offset, size - 1);
    buffer[len] = '\0';
    return len;
  }
};
}

stackhelp::ext::CallStackResolverCallbackAndSizePair DeviceContext::ExecutionContext::on_ext_call_stack_capture(
  stackhelp::CallStackInfo stack, void *context)
{
  auto self = reinterpret_cast<DeviceContext::ExecutionContext *>(context);

  stackhelp::ext::CallStackResolverCallbackAndSizePair result{};
  if (stack.stackSize < extended_call_stack_capture_data_size_in_pointers)
  {
    return result;
  }

  ExtendedCallStackCaptureData data;
  data.deviceContext = &self->self;
  data.callStack = self->contextState.getCommandData();
  data.lastCommandName = self->contextState.getLastCommandName();
  if (self->device.isPersistent())
  {
    data.lastMarker = self->device.currentMarker();
    data.lastEventPath = self->device.currentEventPath();
  }
  else
  {
    data.lastMarker = {};
    data.lastEventPath = {};
  }

  memcpy(&stack.stack[0], &data, sizeof(data));

  result.resolver = &ExecutionContext::on_ext_call_stack_resolve;
  result.size = extended_call_stack_capture_data_size_in_pointers;

  // TODO we could support chaining of prev capture contexts with the remaining storage

  return result;
}

unsigned DeviceContext::ExecutionContext::on_ext_call_stack_resolve(char *buf, unsigned max_buf, stackhelp::CallStackInfo stack)
{
  if (stack.stackSize < extended_call_stack_capture_data_size_in_pointers)
  {
    return 0;
  }

  ExtendedCallStackCaptureData data;
  memcpy(&data, &stack.stack[0], sizeof(data));

  // We are going to write:
  // Extended call stack for command <command>:
  // Last marker <event>
  // Last event <event>
  // Issued by:
  // <call stack>
  BufferWriter writer{buf, max_buf};
  if (data.lastCommandName)
  {
    writer.write("Extended call stack for command <");
    writer.write(data.lastCommandName);
    writer.write(">:\n");
  }
  else
  {
    writer.write("Extended call stack for unknown command:\n");
  }

  if (!data.lastMarker.empty())
  {
    writer.write("Last marker <");
    writer.write(data.lastMarker);
    writer.write(">\n");
  }
  else
  {
    writer.write("No last marker information\n");
  }

  if (!data.lastEventPath.empty())
  {
    writer.write("Last event <");
    writer.write(data.lastEventPath);
    writer.write(">\n");
  }
  else
  {
    writer.write("No event path information\n");
  }

  auto callStack = data.deviceContext->device.resolve(data.callStack);
  if (!callStack.empty())
  {
    writer.write("Issued by:\n");
    writer.write(callStack);
  }
  else
  {
    writer.write("No issuer call stack information\n");
  }
  return writer.finish();
}

void DeviceContext::ExecutionContext::beginWork() { cmdIndex = 0; }
void DeviceContext::ExecutionContext::beginCmd(size_t icmd) { cmd = icmd; }
void DeviceContext::ExecutionContext::endCmd() { ++cmdIndex; }

FramebufferState &DeviceContext::ExecutionContext::getFramebufferState() { return contextState.graphicsState.framebufferState; }

void DeviceContext::ExecutionContext::setUniformBuffer(uint32_t stage, uint32_t unit, BufferResourceReferenceAndAddressRange buffer)
{
  contextState.stageState[stage].setConstBuffer(unit, buffer);
  if (buffer)
  {
    contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);
  }
}

void DeviceContext::ExecutionContext::setSRVTexture(uint32_t stage, uint32_t unit, Image *image, ImageViewState view_state,
  bool as_const_ds, D3D12_CPU_DESCRIPTOR_HANDLE view)
{
#if _TARGET_XBOX
  // When using depth stencil for sampling we _need_ to set the read bit or we may hang the GPU
  // This behavior was introduced in barrier rework of GDK 201101
  as_const_ds = as_const_ds || (image && (image->getFormat().isDepth() || image->getFormat().isDepth()));
#endif

  contextState.stageState[stage].setSRVTexture(unit, image, view_state, as_const_ds, view);
}

void DeviceContext::ExecutionContext::setSampler(uint32_t stage, uint32_t unit, D3D12_CPU_DESCRIPTOR_HANDLE sampler)
{
  contextState.stageState[stage].setSampler(unit, sampler);
}

void DeviceContext::ExecutionContext::setUAVTexture(uint32_t stage, uint32_t unit, Image *image, ImageViewState view_state,
  D3D12_CPU_DESCRIPTOR_HANDLE view)
{
  contextState.stageState[stage].setUAVTexture(unit, image, view_state, view);
}

void DeviceContext::ExecutionContext::setSRVBuffer(uint32_t stage, uint32_t unit, BufferResourceReferenceAndShaderResourceView buffer)
{
  contextState.stageState[stage].setSRVBuffer(unit, buffer);
  if (buffer)
  {
    contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);
  }
}

void DeviceContext::ExecutionContext::setUAVBuffer(uint32_t stage, uint32_t unit,
  BufferResourceReferenceAndUnorderedResourceView buffer)
{
  contextState.stageState[stage].setUAVBuffer(unit, buffer);
  if (buffer)
  {
    contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);
  }
}
#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::setRaytraceAccelerationStructureAtT(uint32_t stage, uint32_t unit,
  RaytraceAccelerationStructure *as)
{
  contextState.stageState[stage].setRaytraceAccelerationStructureAtT(unit, as);
}
#endif

void DeviceContext::ExecutionContext::setSRVNull(uint32_t stage, uint32_t unit) { contextState.stageState[stage].setSRVNull(unit); }

void DeviceContext::ExecutionContext::setUAVNull(uint32_t stage, uint32_t unit) { contextState.stageState[stage].setUAVNull(unit); }

void DeviceContext::ExecutionContext::invalidateActiveGraphicsPipeline() { contextState.graphicsState.pipeline = nullptr; }

void DeviceContext::ExecutionContext::setBlendConstantFactor(E3DCOLOR constant)
{
  const float values[] = //
    {constant.r / 255.f, constant.g / 255.f, constant.b / 255.f, constant.a / 255.f};
  contextState.cmdBuffer.setBlendConstantFactor(values);
}

void DeviceContext::ExecutionContext::setDepthBoundsRange(float from, float to)
{
  contextState.cmdBuffer.setDepthBoundsRange(from, to);
}

void DeviceContext::ExecutionContext::setStencilRef(uint8_t ref) { contextState.cmdBuffer.setStencilReference(ref); }

void DeviceContext::ExecutionContext::setScissorEnable(bool enable)
{
  contextState.graphicsState.statusBits.set(GraphicsState::SCISSOR_ENABLED, enable);
  contextState.graphicsState.statusBits.set(GraphicsState::SCISSOR_ENABLED_DIRTY);
}

void DeviceContext::ExecutionContext::setScissorRects(ScissorRectListRef::RangeType rects)
{
  eastl::copy_n(rects.begin(), rects.size(), contextState.graphicsState.scissorRects);
  contextState.graphicsState.scissorRectCount = rects.size();
  contextState.graphicsState.statusBits.set(GraphicsState::SCISSOR_RECT_DIRTY);
}

void DeviceContext::ExecutionContext::setIndexBuffer(BufferResourceReferenceAndAddressRange buffer, DXGI_FORMAT type)
{
  if (contextState.graphicsState.indexBuffer != buffer || contextState.graphicsState.indexBufferFormat != type)
  {
    contextState.graphicsState.indexBuffer = buffer;
    contextState.graphicsState.indexBufferFormat = type;
    contextState.graphicsState.statusBits.set(GraphicsState::INDEX_BUFFER_DIRTY);
    contextState.graphicsState.statusBits.set(GraphicsState::INDEX_BUFFER_STATE_DIRTY);
  }
}

void DeviceContext::ExecutionContext::setVertexBuffer(uint32_t stream, BufferResourceReferenceAndAddressRange buffer, uint32_t stride)
{
  if (contextState.graphicsState.vertexBuffers[stream] != buffer || contextState.graphicsState.vertexBufferStrides[stream] != stride)
  {
    contextState.graphicsState.vertexBuffers[stream] = buffer;
    contextState.graphicsState.vertexBufferStrides[stream] = stride;
    contextState.graphicsState.statusBits.set(GraphicsState::VERTEX_BUFFER_0_DIRTY + stream);
    contextState.graphicsState.statusBits.set(GraphicsState::VERTEX_BUFFER_STATE_0_DIRTY + stream);
  }
}

bool DeviceContext::ExecutionContext::isPartOfFramebuffer(Image *image)
{
  if (contextState.graphicsState.framebufferState.frontendFrameBufferInfo.depthStencilAttachment.image == image)
    return true;
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
    if (contextState.graphicsState.framebufferState.frontendFrameBufferInfo.colorAttachments[i].image == image)
      return true;
  return false;
}

bool DeviceContext::ExecutionContext::isPartOfFramebuffer(Image *image, MipMapRange mip_range, ArrayLayerRange array_range)
{
  if (contextState.graphicsState.framebufferState.frontendFrameBufferInfo.depthStencilAttachment.image == image)
  {
    auto mipIndex = contextState.graphicsState.framebufferState.frontendFrameBufferInfo.depthStencilAttachment.view.getMipBase();
    auto arrayIndex = contextState.graphicsState.framebufferState.frontendFrameBufferInfo.depthStencilAttachment.view.getArrayBase();
    if (mip_range.isInside(mipIndex) && array_range.isInside(arrayIndex))
      return true;
  }
  for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT; ++i)
  {
    if (contextState.graphicsState.framebufferState.frontendFrameBufferInfo.colorAttachments[i].image == image)
    {
      auto mipIndex = contextState.graphicsState.framebufferState.frontendFrameBufferInfo.colorAttachments[i].view.getMipBase();
      auto arrayIndex = contextState.graphicsState.framebufferState.frontendFrameBufferInfo.colorAttachments[i].view.getArrayBase();
      if (mip_range.isInside(mipIndex) && array_range.isInside(arrayIndex))
        return true;
    }
  }
  return false;
}

// returns true if the current pass encapsulates the draw area of the viewport
void DeviceContext::ExecutionContext::updateViewports(ViewportListRef::RangeType new_vps)
{
  auto cmp = [](const ViewportState &from, const ViewportState &to) {
    return classify_viewport_diff(from, to) == RegionDifference::EQUAL;
  };

  if (contextState.graphicsState.viewportCount != new_vps.size() ||
      !eastl::equal(new_vps.begin(), new_vps.end(), contextState.graphicsState.viewports, cmp))
  {
    eastl::copy_n(new_vps.begin(), new_vps.size(), contextState.graphicsState.viewports);
    contextState.graphicsState.viewportCount = new_vps.size();
    contextState.graphicsState.statusBits.set(GraphicsState::VIEWPORT_DIRTY);
  }
}

D3D12_CPU_DESCRIPTOR_HANDLE DeviceContext::ExecutionContext::getNullColorTarget()
{
  return device.nullResourceTable.table[NullResourceTable::RENDER_TARGET_VIEW];
}

void DeviceContext::ExecutionContext::setStaticRenderState(StaticRenderStateID ident)
{
  contextState.graphicsState.staticRenderStateIdent = ident;
}

void DeviceContext::ExecutionContext::setInputLayout(InputLayoutID ident) { contextState.graphicsState.inputLayoutIdent = ident; }

void DeviceContext::ExecutionContext::setWireFrame(bool wf)
{
  contextState.graphicsState.statusBits.set(GraphicsState::USE_WIREFRAME, wf);
}

bool DeviceContext::ExecutionContext::readyReadBackCommandList()
{
  if (!contextState.activeReadBackCommandList)
  {
    contextState.activeReadBackCommandList = contextState.getFrameData().readBackCommands.allocateList(device.device.get());
  }
#if _TARGET_PC_WIN
  return static_cast<bool>(contextState.activeReadBackCommandList);
#else
  // on console activeReadBackCommandList can never be null
  return true;
#endif
}

bool DeviceContext::ExecutionContext::readyEarlyUploadCommandList()
{
  if (!contextState.activeEarlyUploadCommandList)
  {
    contextState.activeEarlyUploadCommandList = contextState.getFrameData().earlyUploadCommands.allocateList(device.device.get());
  }
#if _TARGET_PC_WIN
  return static_cast<bool>(contextState.activeEarlyUploadCommandList);
#else
  // on console activeEarlyUploadCommandList can never be null
  return true;
#endif
}

bool DeviceContext::ExecutionContext::readyLateUploadCommandList()
{
  if (!contextState.activeLateUploadCommandList)
  {
    contextState.activeLateUploadCommandList = contextState.getFrameData().lateUploadCommands.allocateList(device.device.get());
  }
#if _TARGET_PC_WIN
  return static_cast<bool>(contextState.activeLateUploadCommandList);
#else
  // on console activeLateUploadCommandList can never be null
  return true;
#endif
}

AnyCommandListPtr DeviceContext::ExecutionContext::allocAndBeginCommandBuffer()
{
  FrameInfo &frame = contextState.getFrameData();
  return frame.genericCommands.allocateList(device.device.get());
}

void DeviceContext::ExecutionContext::readBackImagePrePhase()
{
  if (contextState.textureReadBackSplitBarrierEnds.empty())
    return;

  for (auto &&readBack : contextState.textureReadBackSplitBarrierEnds)
  {
    contextState.resourceStates.readyTextureAsReadBack(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, readBack.first, readBack.second);
  }
  contextState.textureReadBackSplitBarrierEnds.clear();
}

bool DeviceContext::ExecutionContext::checkDrawCallHasOutput(eastl::span<const char> info)
{
#if DX12_DROP_NOT_USEFUL_DRAW_CALLS
  auto colorTargets = contextState.graphicsState.framebufferState.framebufferLayout.colorTargetMask;
  auto hasDepthStencil = contextState.graphicsState.framebufferState.framebufferLayout.hasDepth != 0;
  auto hasUAVs = contextState.graphicsState.basePipeline->getSignature().hasUAVAccess();
  auto psOutputs = color_channel_mask_to_render_target_mask(contextState.graphicsState.basePipeline->getPSHeader().inOutSemanticMask);
  bool hasAnyOutput = (0 != (colorTargets & psOutputs)) || hasDepthStencil || hasUAVs;
  if (!hasAnyOutput && readyCommandList())
  {
    // Write a marker, with it we can see in pix when the driver dropped a dead draw command
    contextState.debugMarkerSet(device, contextState.cmdBuffer.getHandle(), {info.data(), info.size()});
  }
  return hasAnyOutput;
#else
  G_UNUSED(info);
  return true;
#endif
}

DeviceContext::ExecutionContext::ExecutionContext(DeviceContext &ctx, ContextState &css) :
  ScopedCallStackContext{&DeviceContext::ExecutionContext::on_ext_call_stack_capture, this},
  self{ctx},
  contextState{css},
  device{ctx.device}
{}

void DeviceContext::ExecutionContext::prepareCommandExecution()
{
  // Every command expects that the current command buffer is ready for recording
  FrameInfo &frame = contextState.getFrameData();
  if (!contextState.cmdBuffer.isReadyForRecording())
  {
    auto buffer = frame.genericCommands.allocateList(device.device.get());
    contextState.cmdBuffer.makeReadyForRecroding(buffer, device.hasDepthBoundsTest(), device.getVariableShadingRateTier());

    contextState.debugBeginCommandBuffer(device, device.device.get(), buffer.get());

    auto &resourceUsageDebugger = static_cast<ResourceUsageHistoryDataSetDebugger &>(self);
    contextState.resourceStates.setRecordingState(resourceUsageDebugger.shouldRecordResourceUsage());
  }
}

void DeviceContext::ExecutionContext::setConstRegisterBuffer(uint32_t stage, HostDeviceSharedMemoryRegion update)
{
  contextState.stageState[stage].constRegisterLastBuffer.BufferLocation = update.gpuPointer;
  contextState.stageState[stage].constRegisterLastBuffer.SizeInBytes = update.range.size();
  contextState.stageState[stage].bRegisterValidMask &= ~1ul;
}

void DeviceContext::ExecutionContext::writeToDebug(StringIndexRef::RangeType info)
{
  if (!readyCommandList())
  {
    return;
  }
  contextState.debugMarkerSet(device, contextState.cmdBuffer.getHandle(), {info.data(), info.size()});
  contextState.resourceStates.marker(info);
}

int64_t DeviceContext::ExecutionContext::flush(bool present, uint64_t progress, OutputMode mode)
{
  int64_t swapDur = 0;
  FrameInfo &frame = contextState.getFrameData();

  // adds barriers for read back access
  readBackImagePrePhase();

  if (present && OutputMode::PRESENT == mode)
  {
    contextState.resourceStates.useTextureAsSwapImageOutput(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, self.back.swapchain.getColorImage());
  }

  // have to reverse the order how stuff is construct the command buffers
  // a command pool can only supply one command buffer at a time and
  // we don't need to have a extra one if we can avoid it by building stuff in the correct order
  auto frameCore = contextState.cmdBuffer.releaseBufferForSubmit();
  if (frameCore)
  {
    contextState.debugEndCommandBuffer(device, frameCore.get());

    contextState.resourceStates.implicitUAVFlushAll(device.currentEventPath().data(), device.validatesUserBarriers());
    // TODO not ideal to wrap it here again
    GraphicsCommandList<AnyCommandListPtr> reWrappedCore{frameCore};
    contextState.graphicsCommandListBarrierBatch.execute(reWrappedCore);

    contextState.getFrameData().backendQueryManager.resolve(frameCore.get());
    DX12_CHECK_RESULT(frameCore->Close());
  }


  // TODO with compute queue support, we need to submit preFrameSet.copySyncGeneric to the graphics queue
  //      then sync graphics progress with compute progress, so that we are sure that compute can only
  //      access stuff after its in the right state


  // Uploads of resources that are already in a state where the upload queue can write to them
  bool hasEarlyUpload = static_cast<bool>(contextState.activeEarlyUploadCommandList);
  if (hasEarlyUpload)
  {
    auto cmdList = contextState.activeEarlyUploadCommandList.release();

    DX12_CHECK_RESULT(cmdList->Close());
    ID3D12CommandList *submit = cmdList.get();
    device.queues[DeviceQueueType::UPLOAD].enqueueCommands(1, &submit);
  }

  // Transition resources into a state where the upload queue can write to them
  if (contextState.uploadBarrierBatch.hasBarriers())
  {
    if (auto cmdList = frame.genericCommands.allocateList(device.device.get()))
    {
      GraphicsCommandList<AnyCommandListPtr> wrappedCmdList{cmdList};
      contextState.uploadBarrierBatch.execute(wrappedCmdList);

      DX12_CHECK_RESULT(cmdList->Close());
      ID3D12CommandList *submit = cmdList.get();
      device.queues[DeviceQueueType::GRAPHICS].enqueueCommands(1, &submit);
    }
  }

  // Upload of resources that where just transitioned into the needed state to write with the upload queue
  bool hasLateUpload = static_cast<bool>(contextState.activeLateUploadCommandList);
  if (hasLateUpload)
  {
    device.queues.synchronizeUploadWithGraphics();

    auto cmdList = contextState.activeLateUploadCommandList.release();

    DX12_CHECK_RESULT(cmdList->Close());
    ID3D12CommandList *submit = cmdList.get();
    device.queues[DeviceQueueType::UPLOAD].enqueueCommands(1, &submit);
  }

  ID3D12CommandList *submits[2];
  uint32_t submitCount = 0;

  // Synchronize uploads with current frame
  if (hasEarlyUpload || hasLateUpload)
  {
    device.queues.synchronizeGraphicsWithUpload();

    // Transition resources written by upload queue into a state the graphics (and later compute)
    // queue can use them as source for anything
    if (contextState.postUploadBarrierBatch.hasBarriers())
    {
      if (auto cmdList = frame.genericCommands.allocateList(device.device.get()))
      {
        GraphicsCommandList<AnyCommandListPtr> wrappedCmdList{cmdList};
        contextState.postUploadBarrierBatch.execute(wrappedCmdList);

        DX12_CHECK_RESULT(cmdList->Close());
        // gets submitted together with the frame it self
        submits[submitCount++] = cmdList.get();
      }
    }
  }
  else
  {
    G_ASSERTF(!contextState.postUploadBarrierBatch.hasBarriers(), "No texture uploads but barriers "
                                                                  "to flush some?");
  }

  if (frameCore)
  {
    // recorded frame
    submits[submitCount++] = frameCore.get();
  }

  // we have to wait for previous frame to avoid overlapping previous frame read back and current frame graphics
  device.queues.synchronizeWithPreviousFrame();

  if (submitCount)
  {
    // flush the frame and possible uploaded resource flush
    device.queues[DeviceQueueType::GRAPHICS].enqueueCommands(submitCount, submits);
  }

  if (present)
  {
    for (FrameEvents *callback : self.back.frameEventCallbacks)
      callback->endFrameEvent();
    self.back.frameEventCallbacks.clear();

    auto now = ref_time_ticks();
    if (OutputMode::PRESENT == mode)
    {
      self.back.swapchain.present(device);
    }
    swapDur = ref_time_ticks() - now;
    frame.progress = progress;

    contextState.debugFramePresent(device);
  }

  // handle read backs
  if (contextState.activeReadBackCommandList)
  {
    auto cmdList = contextState.activeReadBackCommandList.release();
    DX12_CHECK_RESULT(cmdList->Close());

    device.queues.synchronizeReadBackWithGraphics();
    ID3D12CommandList *submit = cmdList.get();
    device.queues[DeviceQueueType::READ_BACK].enqueueCommands(1, &submit);

    device.queues.updateFrameProgress(progress);
  }
  else
  {
    device.queues.updateFrameProgressOnGraphics(progress);
  }


  for (os_event_t &event : self.back.frameCompleteEvents)
    device.queues.waitForFrameProgress(progress, event);
  self.back.frameCompleteEvents.clear();

  auto &resourceUsageDebugger = static_cast<ResourceUsageHistoryDataSetDebugger &>(self);
  contextState.resourceStates.addTo(resourceUsageDebugger);
  if (present)
  {
    resourceUsageDebugger.finishFrame();
  }
  contextState.resourceStates.onFlush(contextState.initialResourceStateSet);
  contextState.resourceActivationTracker.reset();
  contextState.bufferAccessTracker.advance();
  contextState.onFlush();

  ++self.back.frameProgress;
  return swapDur;
}

void DeviceContext::ExecutionContext::pushEvent(StringIndexRef::RangeType name)
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.debugEventBegin(device, contextState.cmdBuffer.getHandle(), {name.data(), name.size()});

  contextState.resourceStates.beginEvent(name);
}

void DeviceContext::ExecutionContext::popEvent()
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.debugEventEnd(device, contextState.cmdBuffer.getHandle());

  contextState.resourceStates.endEvent();
}
void DeviceContext::ExecutionContext::beginSurvey(PredicateInfo pi)
{
  if (readyCommandList())
  {
    contextState.cmdBuffer.beginQuery(pi.heap, D3D12_QUERY_TYPE_BINARY_OCCLUSION, pi.index);
  }
}
void DeviceContext::ExecutionContext::endSurvey(PredicateInfo pi)
{
  if (readyCommandList())
  {
    BufferResourceReferenceAndOffset buffer{pi.buffer, pi.bufferOffset()};
    contextState.cmdBuffer.endQuery(pi.heap, D3D12_QUERY_TYPE_BINARY_OCCLUSION, pi.index);
    copyQueryResult(pi.heap, D3D12_QUERY_TYPE_BINARY_OCCLUSION, pi.index, 1, buffer);
  }
}
void DeviceContext::ExecutionContext::writeTimestamp(Query *query)
{
  if (readyCommandList())
  {
    contextState.getFrameData().backendQueryManager.makeTimeStampQuery(query, device.device.get(), contextState.cmdBuffer.getHandle());
  }
}

void DeviceContext::ExecutionContext::beginConditionalRender(PredicateInfo pi)
{
  if (device.ignorePredication())
  {
    return;
  }
  contextState.graphicsState.predicationBuffer = BufferResourceReferenceAndOffset{pi.buffer, pi.bufferOffset()};
}

void DeviceContext::ExecutionContext::endConditionalRender() { contextState.graphicsState.predicationBuffer = {}; }

void DeviceContext::ExecutionContext::addVertexShader(ShaderID id, VertexShaderModule *sci)
{
  device.pipeMan.addVertexShader(id, sci);
}

void DeviceContext::ExecutionContext::addPixelShader(ShaderID id, PixelShaderModule *sci) { device.pipeMan.addPixelShader(id, sci); }

void DeviceContext::ExecutionContext::addComputePipeline(ProgramID id, ComputeShaderModule *csm)
{
  // if optimization pass is used, then it has to handle this, as it might needs the data for later
  // commands
  eastl::unique_ptr<ComputeShaderModule> sm{csm};
  // module id is the compute program id, no need for extra param
  device.pipeMan.addCompute(device.device.get(), device.pipelineCache, id, eastl::move(*csm),
    get_recover_behvior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
      device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
    device.shouldNameObjects());
}

void DeviceContext::ExecutionContext::addGraphicsPipeline(GraphicsProgramID program, ShaderID vs, ShaderID ps)
{
  device.pipeMan.addGraphics(device.device.get(), device.pipelineCache, contextState.framebufferLayouts, program, vs, ps,
    get_recover_behvior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
      device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
    device.shouldNameObjects());
}

void DeviceContext::ExecutionContext::registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state)
{
  device.pipeMan.registerStaticRenderState(ident, state);
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::addRaytracePipeline(ProgramID program, uint32_t max_recursion, uint32_t shader_count,
  const ShaderID *shaders, uint32_t group_count, const RaytraceShaderGroup *groups)
{ // TODO
  G_UNUSED(program);
  G_UNUSED(max_recursion);
  G_UNUSED(shader_count);
  G_UNUSED(shaders);
  G_UNUSED(group_count);
  G_UNUSED(groups);
  G_ASSERTF(false, "ExecutionContext::addRaytracePipeline called on API without support");
}
void DeviceContext::ExecutionContext::copyRaytraceShaderGroupHandlesToMemory(ProgramID program, uint32_t first_group,
  uint32_t group_count, uint32_t size, void *ptr)
{ // TODO
  G_UNUSED(program);
  G_UNUSED(first_group);
  G_UNUSED(group_count);
  G_UNUSED(size);
  G_UNUSED(ptr);
  G_ASSERTF(false, "ExecutionContext::copyRaytraceShaderGroupHandlesToMemory called on API without support");
}

void DeviceContext::ExecutionContext::buildBottomAccelerationStructure(
  D3D12_RAYTRACING_GEOMETRY_DESC_ListRef::RangeType geometry_descriptions,
  RaytraceGeometryDescriptionBufferResourceReferenceSetListRef::RangeType resource_refs,
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update, ID3D12Resource *dst, ID3D12Resource *src,
  BufferResourceReferenceAndAddress scratch)
{
  if (!readyCommandList())
  {
    return;
  }

  for (auto &&ref : resource_refs)
  {
    contextState.resourceStates.useBufferAsRTASBuildSource(contextState.graphicsCommandListBarrierBatch, ref.vertexOrAABBBuffer);
    contextState.bufferAccessTracker.updateLastFrameAccess(ref.vertexOrAABBBuffer.resourceId);
    dirtyBufferState(ref.vertexOrAABBBuffer.resourceId);
    if (ref.indexBuffer)
    {
      contextState.resourceStates.useBufferAsRTASBuildSource(contextState.graphicsCommandListBarrierBatch, ref.indexBuffer);
      contextState.bufferAccessTracker.updateLastFrameAccess(ref.indexBuffer.resourceId);
      dirtyBufferState(ref.vertexOrAABBBuffer.resourceId);
    }
    if (ref.transformBuffer)
    {
      contextState.resourceStates.useBufferAsRTASBuildSource(contextState.graphicsCommandListBarrierBatch, ref.transformBuffer);
      contextState.bufferAccessTracker.updateLastFrameAccess(ref.transformBuffer.resourceId);
      dirtyBufferState(ref.transformBuffer.resourceId);
    }
  }

  if (src)
  {
    contextState.resourceStates.useRTASAsUpdateSource(contextState.graphicsCommandListBarrierBatch, src);
  }

  contextState.resourceStates.useRTASAsBuildTarget(contextState.graphicsCommandListBarrierBatch, dst);

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
  bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  bottomLevelInputs.Flags = flags;
  if (update)
    bottomLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  bottomLevelInputs.NumDescs = geometry_descriptions.size();
  bottomLevelInputs.pGeometryDescs = geometry_descriptions.data();

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
  bottomLevelBuildDesc.Inputs = bottomLevelInputs;
  bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch.gpuPointer;
  bottomLevelBuildDesc.DestAccelerationStructureData = dst->GetGPUVirtualAddress();
  if (src)
  {
    bottomLevelBuildDesc.SourceAccelerationStructureData = src->GetGPUVirtualAddress();
  }
  else if (update)
  {
    bottomLevelBuildDesc.SourceAccelerationStructureData = bottomLevelBuildDesc.DestAccelerationStructureData;
  }

  // TODO: not 100% sure if needed or wanted, have to see
  disablePredication();

  contextState.cmdBuffer.buildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

  // Always flush scratch buffer, probably okay without it, but not correct.
  contextState.graphicsCommandListBarrierBatch.flushUAV(scratch.buffer);
  // Ensure writes are completed before we can reference it in TLAS build
  contextState.graphicsCommandListBarrierBatch.flushUAV(dst);
}

void DeviceContext::ExecutionContext::buildTopAccelerationStructure(uint32_t instance_count,
  BufferResourceReferenceAndAddress instance_buffer, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update,
  ID3D12Resource *dst, ID3D12Resource *src, BufferResourceReferenceAndAddress scratch)
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.useBufferAsRTASBuildSource(contextState.graphicsCommandListBarrierBatch, instance_buffer);
  contextState.bufferAccessTracker.updateLastFrameAccess(instance_buffer.resourceId);
  dirtyBufferState(instance_buffer.resourceId);

  if (src)
  {
    contextState.resourceStates.useRTASAsUpdateSource(contextState.graphicsCommandListBarrierBatch, src);
  }

  contextState.resourceStates.useRTASAsBuildTarget(contextState.graphicsCommandListBarrierBatch, dst);

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs = {};
  topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  topLevelInputs.Flags = flags;
  if (update)
    topLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
  topLevelInputs.NumDescs = instance_count;
  topLevelInputs.InstanceDescs = instance_buffer.gpuPointer;

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc = {};
  topLevelBuildDesc.Inputs = topLevelInputs;
  topLevelBuildDesc.DestAccelerationStructureData = dst->GetGPUVirtualAddress();
  topLevelBuildDesc.ScratchAccelerationStructureData = scratch.gpuPointer;
  if (src)
  {
    topLevelBuildDesc.SourceAccelerationStructureData = src->GetGPUVirtualAddress();
  }
  else if (update)
  {
    topLevelBuildDesc.SourceAccelerationStructureData = dst->GetGPUVirtualAddress();
  }

  // TODO: not 100% sure if needed or wanted, have to see
  disablePredication();

  contextState.cmdBuffer.buildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

  // Always flush scratch buffer, probably okay without it, but not correct.
  contextState.graphicsCommandListBarrierBatch.flushUAV(scratch.buffer);
  // Ensure writes are completed before we can use it as AS in shaders
  contextState.graphicsCommandListBarrierBatch.flushUAV(dst);
}

void DeviceContext::ExecutionContext::traceRays(BufferResourceReferenceAndRange ray_gen_table,
  BufferResourceReferenceAndRange miss_table, BufferResourceReferenceAndRange hit_table,
  BufferResourceReferenceAndRange callable_table, uint32_t miss_stride, uint32_t hit_stride, uint32_t callable_stride, uint32_t width,
  uint32_t height, uint32_t depth)
{ // TODO
  G_UNUSED(ray_gen_table);
  G_UNUSED(miss_table);
  G_UNUSED(hit_table);
  G_UNUSED(callable_table);
  G_UNUSED(miss_stride);
  G_UNUSED(hit_stride);
  G_UNUSED(callable_stride);
  G_UNUSED(width);
  G_UNUSED(height);
  G_UNUSED(depth);

  if (!readyCommandList())
  {
    return;
  }

  tranistionPredicationBuffer();

  // auto &registers = contextState.raytraceState.pipeline->getSignature().registers;
  // raytrace is basically compute, so flush uav too
  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    false);
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  applyPredicationBuffer();
  /*
    contextState.stageState[STAGE_RAYTRACE].apply(
      device->device, device->dummyResourceTable, self->getDescriptorSetMode(),
      contextState.frameIndex % array_size(contextState.frames),
      contextState.raytraceState.pipeline->getLayout()->registers,
      contextState.raytraceStageRefTable,
      [=](VulkanDescriptorSetHandle set, const uint32_t *offsets, uint32_t offset_count) //
      {
        this->contextState.cmdBuffer.bindRaytraceDescriptorSet(set, offsets,
    offset_count);
      });

    auto callableBufferHandle = callable_table ? callable_table.getHandle() : VulkanBufferHandle{};
    auto callableBufferOffset = callable_table ? callable_table.dataOffset(callable_offset) : 0;

    contextState.cmdBuffer.traceRays(
      device->device, ray_gen_table.getHandle(), ray_gen_table.dataOffset(ray_gen_offset),
      miss_table.getHandle(), miss_table.dataOffset(miss_offset), miss_stride,
    hit_table.getHandle(), hit_table.dataOffset(hit_offset), hit_stride, callableBufferHandle,
    callableBufferOffset, callable_stride, width, height, depth); if
    (contextState.cmdBufferDebug)
    {
      // TODO record draw command
    }*/
}
#endif

void DeviceContext::ExecutionContext::present(uint64_t progress, Drv3dTimings *timing_data, int64_t kickoff_stamp,
  uint32_t latency_frame, OutputMode mode)
{
#if DX12_RECORD_TIMING_DATA
  auto now = ref_time_ticks();
  timing_data->frontendToBackendUpdateScreenLatency = now - kickoff_stamp;
  timing_data->gpuWaitDuration = self.back.gpuWaitDuration;
  timing_data->backendFrontendWaitDuration = self.back.workWaitDuration;
  timing_data->backbufferAcquireDuration = self.back.acquireBackBufferDuration;

  self.back.workWaitDuration = 0;
#else
  G_UNUSED(timing_data);
  G_UNUSED(kickoff_stamp);
#endif

  {
    SCOPED_LATENCY_MARKER(latency_frame, RENDERSUBMIT_START, RENDERSUBMIT_END);
    auto swapDur = flush(true, progress, mode);
#if DX12_RECORD_TIMING_DATA
    timing_data->presentDuration = swapDur;
#else
    G_UNUSED(swapDur);
#endif
  }

  /*  uint32_t deltaAlloc = device->memory.getAllocationCounter() - device->lastAllocationCount;
    uint32_t deltaFree = device->memory.getFreeCounter() - device->lastFreeCount;

    if ((deltaAlloc + deltaFree) > 10)
    {
      device->memory.printStats();
      debug("Allocation count delta: %u", deltaAlloc);
      debug("Free count delta: %u", deltaFree);
      device->lastAllocationCount += deltaAlloc;
      device->lastFreeCount += deltaFree;
    }*/

  self.makeReadyForFrame(mode);
#if DAGOR_DBGLEVEL > 0
  self.initNextFrameLog();
#endif
  device.pipeMan.evictDecompressionCache();
}

void DeviceContext::ExecutionContext::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
  if (!contextState.computeState.pipeline || !readyCommandList())
  {
    return;
  }

  auto &pipelineHeader = contextState.computeState.pipeline->getHeader();

  contextState.stageState[STAGE_CS].enumerateUAVResources(pipelineHeader.resourceUsageTable.uRegisterUseMask,
    [this](auto res) //
    { contextState.resourceStates.beginImplicitUAVAccess(res); });

  tranistionPredicationBuffer();

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  applyPredicationBuffer();

  contextState.cmdBuffer.dispatch(x, y, z);

  contextState.debugDispatch(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_CS],
    *contextState.computeState.pipeline, x, y, z);
}

void DeviceContext::ExecutionContext::dispatchIndirect(BufferResourceReferenceAndOffset buffer)
{
  if (!contextState.computeState.pipeline || !readyCommandList())
  {
    return;
  }

  contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, buffer);
  contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);

  auto &pipelineHeader = contextState.computeState.pipeline->getHeader();

  contextState.stageState[STAGE_CS].enumerateUAVResources(pipelineHeader.resourceUsageTable.uRegisterUseMask,
    [this](auto res) //
    { contextState.resourceStates.beginImplicitUAVAccess(res); });

  tranistionPredicationBuffer();

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  applyPredicationBuffer();

  // just lazy create signature here
  if (!contextState.dispatchIndirectSignature)
  {
    D3D12_INDIRECT_ARGUMENT_DESC arg;
    D3D12_COMMAND_SIGNATURE_DESC desc;
    desc.NumArgumentDescs = 1;
    desc.pArgumentDescs = &arg;
    desc.NodeMask = 0;

    arg.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;
    desc.ByteStride = sizeof(D3D12_DISPATCH_ARGUMENTS);
    if (!DX12_CHECK_OK(device.device->CreateCommandSignature(&desc, nullptr, COM_ARGS(&contextState.dispatchIndirectSignature))))
    {
      return;
    }
  }
  contextState.cmdBuffer.dispatchIndirect(contextState.dispatchIndirectSignature.Get(), buffer.buffer, buffer.offset);

  contextState.debugDispatchIndirect(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_CS],
    *contextState.computeState.pipeline, buffer);
}

void DeviceContext::ExecutionContext::copyBuffer(BufferResourceReferenceAndOffset src, BufferResourceReferenceAndOffset dst,
  uint32_t size)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  // Technically on XBOX this is allowed, but as we do not use buffer suballocation, this should
  // never happen.
  G_ASSERTF(src.resourceId != dst.resourceId, "It is invalid to copy with the source and the "
                                              "destination to be the same buffer");
  contextState.resourceStates.useBufferAsCopySource(contextState.graphicsCommandListBarrierBatch, src);

  contextState.resourceStates.useBufferAsCopyDestination(contextState.graphicsCommandListBarrierBatch, dst);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  contextState.cmdBuffer.copyBuffer(dst.buffer, dst.offset, src.buffer, src.offset, size);

  contextState.bufferAccessTracker.updateLastFrameAccess(src.resourceId);
  contextState.bufferAccessTracker.updateLastFrameAccess(dst.resourceId);

  dirtyBufferState(src.resourceId);
  dirtyBufferState(dst.resourceId);
}

void DeviceContext::ExecutionContext::updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  contextState.resourceStates.useBufferAsCopyDestination(contextState.graphicsCommandListBarrierBatch, dest);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  contextState.cmdBuffer.copyBuffer(dest.buffer, dest.offset, update.buffer, update.range.front(), update.range.size());

  contextState.bufferAccessTracker.updateLastFrameAccess(dest.resourceId);

  dirtyBufferState(dest.resourceId);
}

void DeviceContext::ExecutionContext::clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4])
{
  if (!readyCommandList())
  {
    return;
  }

  tranistionPredicationBuffer();
  applyPredicationBuffer();

  contextState.resourceStates.useBufferAsUAVForClear(contextState.graphicsCommandListBarrierBatch, buffer);

  // TODO only flush UAV for the buffer, any other UAV resource can still be accessed without issues
  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  auto &frame = contextState.getFrameData();
  auto index = frame.resourceViewHeaps.findInUAVScratchSegment(buffer.clearView);
  if (!index)
  {
    // reserve space for one UAV descriptor
    frame.resourceViewHeaps.reserveSpace(device.device.get(), 0, 0, 1, 0, 0);
    index = frame.resourceViewHeaps.appendToUAVScratchSegment(device.device.get(), buffer.clearView);
  }
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());
  contextState.cmdBuffer.clearUnorderedAccessViewFloat(frame.resourceViewHeaps.getGpuAddress(index), buffer.clearView, buffer.buffer,
    values, 0, nullptr);

  contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);

  dirtyBufferState(buffer.resourceId);
}

void DeviceContext::ExecutionContext::clearBufferUint(BufferResourceReferenceAndClearView buffer, const uint32_t values[4])
{
  if (!readyCommandList())
  {
    return;
  }

  tranistionPredicationBuffer();
  applyPredicationBuffer();

  contextState.resourceStates.useBufferAsUAVForClear(contextState.graphicsCommandListBarrierBatch, buffer);

  // TODO only flush UAV for the buffer, any other UAV resource can still be accessed without issues
  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  auto &frame = contextState.getFrameData();
  auto index = frame.resourceViewHeaps.findInUAVScratchSegment(buffer.clearView);
  if (!index)
  {
    // reserve space for one UAV descriptor
    frame.resourceViewHeaps.reserveSpace(device.device.get(), 0, 0, 1, 0, 0);
    index = frame.resourceViewHeaps.appendToUAVScratchSegment(device.device.get(), buffer.clearView);
  }
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());
  contextState.cmdBuffer.clearUnorderedAccessViewUint(frame.resourceViewHeaps.getGpuAddress(index), buffer.clearView, buffer.buffer,
    values, 0, nullptr);

  contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);

  dirtyBufferState(buffer.resourceId);
}

void DeviceContext::ExecutionContext::clearDepthStencilImage(Image *image, ImageViewState view,
  D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const ClearDepthStencilValue &value)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  contextState.resourceStates.useTextureAsDSVForClear(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image, view);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  contextState.cmdBuffer.clearDepthStencilView(view_descriptor, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, value.depth,
    value.stencil, 0, nullptr);

  dirtyTextureState(image);
}

void DeviceContext::ExecutionContext::clearColorImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
  const ClearColorValue &value)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  contextState.resourceStates.useTextureAsRTVForClear(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image, view);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  contextState.cmdBuffer.clearRenderTargetView(view_descriptor, value.float32, 0, nullptr);

  dirtyTextureState(image);
}

void DeviceContext::ExecutionContext::copyImage(Image *src, Image *dst, const ImageCopy &copy)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  if (is_whole_resource_copy_info(copy))
  {
    contextState.resourceStates.useTextureAsCopySourceForWholeCopy(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, src);

    contextState.resourceStates.useTextureAsCopyDestinationForWholeCopy(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, dst);

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    contextState.cmdBuffer.copyResource(dst->getHandle(), src->getHandle());

    contextState.resourceStates.finishUseTextureAsCopyDestinationForWholeCopy(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, dst);
  }
  else
  {
    contextState.resourceStates.useTextureAsCopySource(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, src, copy.srcSubresource);

    contextState.resourceStates.useTextureAsCopyDestination(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, dst, copy.dstSubresource);

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    D3D12_TEXTURE_COPY_LOCATION srcInfo{src->getHandle(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    srcInfo.SubresourceIndex = copy.srcSubresource.index();

    D3D12_TEXTURE_COPY_LOCATION dstInfo{dst->getHandle(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX};
    dstInfo.SubresourceIndex = copy.dstSubresource.index();

    auto srcPlaneWidth = src->getSubresourcesPerPlane();
    auto srcPlaneCount = src->getPlaneCount();
    auto dstPlaneWidth = dst->getSubresourcesPerPlane();
    auto dstPlaneCount = dst->getPlaneCount();

    for (auto p : min(srcPlaneCount, dstPlaneCount))
    {
      G_UNUSED(p);
      contextState.cmdBuffer.copyTexture(&dstInfo, copy.dstOffset.x, copy.dstOffset.y, copy.dstOffset.z, &srcInfo, &copy.srcBox);
      srcInfo.SubresourceIndex += srcPlaneWidth.count();
      dstInfo.SubresourceIndex += dstPlaneWidth.count();
    }

    contextState.resourceStates.finishUseTextureAsCopyDestination(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, dst, copy.dstSubresource);
  }

  src->updateLastFrameAccess(self.back.frameProgress);

  dirtyTextureState(src);
  dirtyTextureState(dst);
}

void DeviceContext::ExecutionContext::blitImage(Image *src, Image *dst, ImageViewState src_view, ImageViewState dst_view,
  D3D12_CPU_DESCRIPTOR_HANDLE src_view_descroptor, D3D12_CPU_DESCRIPTOR_HANDLE dst_view_descriptor, D3D12_RECT src_rect,
  D3D12_RECT dst_rect, bool disable_predication)
{
  if (!readyCommandList())
  {
    return;
  }

  union
  {
    float floatRegs[4];
    DWORD dwordRegs[4];
  };

  contextState.resourceStates.useTextureAsBlitSource(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, src, src_view);

  contextState.resourceStates.useTextureAsBlitDestination(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, dst, dst_view);

  if (!disable_predication)
  {
    tranistionPredicationBuffer();
  }

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  D3D12_VIEWPORT dstViewport;
  dstViewport.TopLeftX = dst_rect.left;
  dstViewport.TopLeftY = dst_rect.top;
  dstViewport.Width = dst_rect.right - dst_rect.left;
  dstViewport.Height = dst_rect.bottom - dst_rect.top;
  dstViewport.MinDepth = 0.01f;
  dstViewport.MaxDepth = 1.f;

  auto srcSize = src->getMipExtents2D(src_view.getMipBase());
  floatRegs[0] = float(src_rect.left) / srcSize.width;
  floatRegs[1] = float(src_rect.top) / srcSize.height;
  floatRegs[2] = float(src_rect.right - src_rect.left) / srcSize.width;
  floatRegs[3] = float(src_rect.bottom - src_rect.top) / srcSize.height;

  auto &frame = contextState.getFrameData();

  auto srcViewGpuIndex = frame.resourceViewHeaps.findInSRVScratchSegment(src_view_descroptor);
  if (!srcViewGpuIndex)
  {
    // reserve space for one SRV descriptor
    frame.resourceViewHeaps.reserveSpace(device.device.get(), 0, 1, 0, 0, 0);
    srcViewGpuIndex = frame.resourceViewHeaps.appendToSRVScratchSegment(device.device.get(), src_view_descroptor);
  }
  auto srcViewGpuHandle = frame.resourceViewHeaps.getGpuAddress(srcViewGpuIndex);

  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());
  auto blitPipeline = device.pipeMan.getBlitPipeline(device.device.get(), dst->getFormat().asDxGiFormat(), device.shouldNameObjects());
  if (!blitPipeline)
  {
    return;
  }

  if (disable_predication)
  {
    disablePredication();
  }
  else
  {
    applyPredicationBuffer();
  }

  // blit pipeline signature can not be null when blitPipeline is not null, as blitPipeline requires the signature to be created
  contextState.cmdBuffer.blitExecute(device.pipeMan.getBlitSignature(), blitPipeline, dwordRegs, srcViewGpuHandle, dst_view_descriptor,
    dstViewport, dst_rect);

  src->updateLastFrameAccess(self.back.frameProgress);

  dirtyTextureState(src);
  dirtyTextureState(dst);

  contextState.debugBlit(device, contextState.cmdBuffer.getHandle());
}

void DeviceContext::ExecutionContext::copyQueryResult(ID3D12QueryHeap *heap, D3D12_QUERY_TYPE type, uint32_t index, uint32_t count,
  BufferResourceReferenceAndOffset buffer)
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.useBufferAsQueryResultDestination(contextState.graphicsCommandListBarrierBatch, buffer);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  contextState.cmdBuffer.resolveQueryData(heap, type, index, count, buffer.buffer, buffer.offset);

  contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);

  dirtyBufferState(buffer.resourceId);
}

void DeviceContext::ExecutionContext::clearRenderTargets(ViewportState vp, uint32_t clear_mask, const E3DCOLOR *clear_color,
  float clear_depth, uint8_t clear_stencil)
{
  if (!readyCommandList())
  {
    return;
  }

  flushRenderTargetStates();
  if (FramebufferLayoutID::Null() == contextState.graphicsState.framebufferLayoutID)
  {
    flushRenderTargets();
  }

  uint8_t ccMask = 0;
  // ms forgot to add a none flag for 0 init....
  constexpr D3D12_CLEAR_FLAGS null_clear_flag = static_cast<D3D12_CLEAR_FLAGS>(0);
  D3D12_CLEAR_FLAGS dscMask = null_clear_flag;
  if (clear_mask & CLEAR_TARGET)
  {
    ccMask = contextState.graphicsState.framebufferState.framebufferLayout.colorTargetMask;
  }
  // clearing null target is invalid
  if (contextState.graphicsState.framebufferState.frameBufferInfo.depthStencilAttachment.ptr != 0)
  {
    if (clear_mask & CLEAR_ZBUFFER)
    {
      dscMask = D3D12_CLEAR_FLAG_DEPTH;
    }
    if (clear_mask & CLEAR_STENCIL)
    {
      dscMask |= D3D12_CLEAR_FLAG_STENCIL;
    }
  }

  if (!ccMask && (null_clear_flag == dscMask))
  {
#if DX12_WARN_EMPTY_CLEARS
    // Warn if a clear request did nothing
    logwarn("DX12: Clear was requested but no corresponding targets where set, active color target mask "
            "0x%02X, depth was set %u, clear request mask 0x%02X",
      ccMask, 0 != contextState.graphicsState.framebufferState.frameBufferInfo.depthStencilAttachment.ptr, clear_mask);
#endif
  }
  else
  {
    tranistionPredicationBuffer();

    contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
      device.validatesUserBarriers());
    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();

    float cc[4];

    D3D12_RECT dx12Area = vp.asRect();
    for (uint32_t i = 0; i < Driver3dRenderTarget::MAX_SIMRT && ccMask; ++i, ccMask >>= 1)
    {
      if (1 & ccMask)
      {
        cc[0] = clear_color[i].r / 255.f;
        cc[1] = clear_color[i].g / 255.f;
        cc[2] = clear_color[i].b / 255.f;
        cc[3] = clear_color[i].a / 255.f;
        contextState.cmdBuffer.clearRenderTargetView(contextState.graphicsState.framebufferState.frameBufferInfo.colorAttachments[i],
          cc, 1, &dx12Area);
      }
    }
    if (null_clear_flag != dscMask)
    {
      contextState.cmdBuffer.clearDepthStencilView(contextState.graphicsState.framebufferState.frameBufferInfo.depthStencilAttachment,
        dscMask, clear_depth, clear_stencil, 1, &dx12Area);
    }
  }
}

void DeviceContext::ExecutionContext::invalidateFramebuffer()
{
  contextState.graphicsState.framebufferLayoutID = FramebufferLayoutID::Null();
}

void DeviceContext::ExecutionContext::flushRenderTargets()
{
  const auto &framebufferLayout = contextState.graphicsState.framebufferState.framebufferLayout;
  uint32_t cnt = 0;
  cnt = framebufferLayout.colorTargetMask > 0x7F   ? 8
        : framebufferLayout.colorTargetMask > 0x3F ? 7
        : framebufferLayout.colorTargetMask > 0x1F ? 6
        : framebufferLayout.colorTargetMask > 0x0F ? 5
        : framebufferLayout.colorTargetMask > 0x07 ? 4
        : framebufferLayout.colorTargetMask > 0x03 ? 3
        : framebufferLayout.colorTargetMask > 0x01 ? 2
        : framebufferLayout.colorTargetMask > 0x00 ? 1
                                                   : 0;

  contextState.cmdBuffer.setRenderTargets(contextState.graphicsState.framebufferState.frameBufferInfo.colorAttachments, cnt,
    contextState.graphicsState.framebufferState.frameBufferInfo.depthStencilAttachment);

  contextState.graphicsState.framebufferLayoutID = contextState.framebufferLayouts.getLayoutID(framebufferLayout);
  invalidateActiveGraphicsPipeline();
}

void DeviceContext::ExecutionContext::flushRenderTargetStates()
{
  auto &frameBufferState = contextState.graphicsState.framebufferState;
  auto &framebufferLayout = frameBufferState.framebufferLayout;
  auto &passImages = frameBufferState.frontendFrameBufferInfo;

  if (frameBufferState.framebufferDirtyState.hasDepthStencilAttachment)
  {
    auto &attachment = passImages.depthStencilAttachment;
    if (0 == frameBufferState.framebufferDirtyState.isConstantDepthstencilAttachment)
    {
      contextState.resourceStates.useTextureAsDSV(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, attachment.image, attachment.view);
    }
    else
    {
      contextState.resourceStates.useTextureAsDSVConst(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, attachment.image, attachment.view);
    }
  }

  frameBufferState.framebufferDirtyState.iterateColorAttachments(framebufferLayout.colorTargetMask,
    [this, &passImages](uint32_t index) //
    {
      auto &attachment = passImages.colorAttachments[index];
      contextState.resourceStates.useTextureAsRTV(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, attachment.image, attachment.view);
    });

  frameBufferState.framebufferDirtyState.hasDepthStencilAttachment = 0;
  frameBufferState.framebufferDirtyState.colorAttachmentMask &= ~framebufferLayout.colorTargetMask;
}

void DeviceContext::ExecutionContext::dirtyTextureStateForFramebufferAttachmentUse(Image *texture)
{
  for (auto &stage : contextState.stageState)
  {
    stage.dirtyTextureState(texture);
  }
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::setRaytracePipeline(ProgramID program)
{ // TODO
  auto &newPipeline = device.pipeMan.getRaytraceProgram(program);
  auto oldPipeline = contextState.raytraceState.pipeline;
  contextState.raytraceState.pipeline = &newPipeline;
  auto &newSignature = newPipeline.getSignature();
  // if layout did change, we need to submit all descriptor sets
  // if layout did not change, we only need to submit changed descriptor sets
  if (!oldPipeline || (&newSignature != &oldPipeline->getSignature()))
  { /*
     contextState.raytraceStageRefTable =
       contextState.stageState[STAGE_RAYTRACE].getRegisterRefTable(newSignature->registers.header);*/
    contextState.stageState[STAGE_RAYTRACE].invalidateState();
  }

  contextState.cmdBuffer.bindRaytracePipeline(&newSignature, newPipeline.getHandle());
}
#endif

void DeviceContext::ExecutionContext::setGraphicsPipeline(GraphicsProgramID program)
{
  auto newPipeline = device.pipeMan.getGraphics(program);
  auto oldPipeline = contextState.graphicsState.basePipeline;
  contextState.graphicsState.basePipeline = newPipeline;

  invalidateActiveGraphicsPipeline();
  if (newPipeline)
  {
    if (!oldPipeline || (&oldPipeline->getSignature() != &newPipeline->getSignature()))
    {
      contextState.stageState[STAGE_VS].invalidateState();
      contextState.stageState[STAGE_PS].invalidateState();
    }
  }
}

void DeviceContext::ExecutionContext::setComputePipeline(ProgramID program)
{
  auto newPipeline = device.pipeMan.getCompute(program);
  auto oldPipeline = contextState.computeState.pipeline;
  contextState.computeState.pipeline = newPipeline;

  if (newPipeline)
  {
    // if layout did change, we need to submit all descriptor sets
    // if layout did not change, we only need to submit changed descriptor sets
    if (!oldPipeline || (&newPipeline->getSignature() != &oldPipeline->getSignature()))
    {
      contextState.stageState[STAGE_CS].invalidateState();
    }

    contextState.cmdBuffer.bindComputePipeline(&newPipeline->getSignature(),
      newPipeline->loadAndGetHandle(device.device.get(), device.pipelineCache,
        get_recover_behvior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
          device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
        device.shouldNameObjects()));
  }
}

void DeviceContext::ExecutionContext::bindVertexUserData(HostDeviceSharedMemoryRegion bsa, uint32_t stride)
{
  if (readyCommandList())
  {
    D3D12_VERTEX_BUFFER_VIEW view{bsa.gpuPointer, static_cast<UINT>(bsa.range.size()), stride};
    contextState.cmdBuffer.iaSetVertexBuffers(0, 1, &view);
    // on next non user draw this will then set the vertex buffer again, no need to set state bit dirty though
    contextState.graphicsState.statusBits.set(GraphicsState::VERTEX_BUFFER_0_DIRTY);
  }
}

void DeviceContext::ExecutionContext::drawIndirect(BufferResourceReferenceAndOffset buffer, uint32_t count, uint32_t stride)
{
  if (!contextState.graphicsState.basePipeline || !readyCommandList())
  {
    return;
  }

  if (checkDrawCallHasOutput("Skipped: drawIndirect"))
  {
    contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, buffer);
    contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);

    tranistionPredicationBuffer();

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();

    auto indirectSignature =
      contextState.drawIndirectSignatures.getSignatureForStride(device.device.get(), stride, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW);
    if (!indirectSignature)
    {
      // error while trying to create a signature
      return;
    }
    contextState.cmdBuffer.drawIndirect(indirectSignature, buffer.buffer, buffer.offset, count);

    contextState.debugDrawIndirect(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_VS],
      contextState.stageState[STAGE_PS], *contextState.graphicsState.basePipeline, *contextState.graphicsState.pipeline, buffer);
  }
}

void DeviceContext::ExecutionContext::drawIndexedIndirect(BufferResourceReferenceAndOffset buffer, uint32_t count, uint32_t stride)
{
  if (!contextState.graphicsState.basePipeline || !readyCommandList())
  {
    return;
  }

  if (checkDrawCallHasOutput("Skipped: drawIndexedIndirect"))
  {
    contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, buffer);
    contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);

    tranistionPredicationBuffer();

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();

    auto &signature = contextState.graphicsState.basePipeline->getSignature();
    auto indirectSignature = contextState.drawIndexedIndirectSignatures.getSignatureForStride(device.device.get(), stride,
      D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, signature);
    if (!indirectSignature)
    {
      // error while trying to create a signature
      return;
    }
    contextState.cmdBuffer.drawIndexedIndirect(indirectSignature, buffer.buffer, buffer.offset, count);

    contextState.debugDrawIndexedIndirect(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_VS],
      contextState.stageState[STAGE_PS], *contextState.graphicsState.basePipeline, *contextState.graphicsState.pipeline, buffer);
  }
}

void DeviceContext::ExecutionContext::draw(uint32_t count, uint32_t instance_count, uint32_t start, uint32_t first_instance)
{
  if (!contextState.graphicsState.basePipeline || !readyCommandList())
  {
    return;
  }

  if (checkDrawCallHasOutput("Skipped: draw"))
  {
    tranistionPredicationBuffer();

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();

    contextState.cmdBuffer.draw(count, instance_count, start, first_instance);

    contextState.debugRecordDraw(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_VS],
      contextState.stageState[STAGE_PS], *contextState.graphicsState.basePipeline, *contextState.graphicsState.pipeline, count,
      instance_count, start, first_instance, contextState.cmdBuffer.getPrimitiveTopology());
  }
}

void DeviceContext::ExecutionContext::drawIndexed(uint32_t count, uint32_t instance_count, uint32_t index_start, int32_t vertex_base,
  uint32_t first_instance)
{
  if (!contextState.graphicsState.basePipeline || !readyCommandList())
  {
    return;
  }

  if (checkDrawCallHasOutput("Skipped: drawIndexed"))
  {
    tranistionPredicationBuffer();

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();

    contextState.cmdBuffer.drawIndexed(count, instance_count, index_start, vertex_base, first_instance);

    contextState.debugRecordDrawIndexed(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_VS],
      contextState.stageState[STAGE_PS], *contextState.graphicsState.basePipeline, *contextState.graphicsState.pipeline, count,
      instance_count, index_start, vertex_base, first_instance, contextState.cmdBuffer.getPrimitiveTopology());
  }
}

void DeviceContext::ExecutionContext::bindIndexUser(HostDeviceSharedMemoryRegion bsa)
{
  if (readyCommandList())
  {
    D3D12_INDEX_BUFFER_VIEW view{bsa.gpuPointer, static_cast<UINT>(bsa.range.size()), DXGI_FORMAT_R16_UINT};
    contextState.cmdBuffer.iaSetIndexBuffer(&view);
    // on next non user draw this will then set the index buffer again
    contextState.graphicsState.statusBits.set(GraphicsState::INDEX_BUFFER_DIRTY);
  }
}

void DeviceContext::ExecutionContext::flushViewportAndScissor()
{
  if (contextState.graphicsState.statusBits.test(GraphicsState::SCISSOR_ENABLED) &&
      (contextState.graphicsState.statusBits.test(GraphicsState::SCISSOR_RECT_DIRTY) ||
        contextState.graphicsState.statusBits.test(GraphicsState::SCISSOR_ENABLED_DIRTY)))
  {
    contextState.cmdBuffer.setScissorRects(
      make_span_const(contextState.graphicsState.scissorRects, contextState.graphicsState.scissorRectCount));
    contextState.graphicsState.statusBits.reset(GraphicsState::SCISSOR_RECT_DIRTY);
    contextState.graphicsState.statusBits.reset(GraphicsState::SCISSOR_ENABLED_DIRTY);
  }
  else if (contextState.graphicsState.statusBits.test(GraphicsState::VIEWPORT_DIRTY) ||
           contextState.graphicsState.statusBits.test(GraphicsState::SCISSOR_ENABLED_DIRTY))
  {
    D3D12_RECT rects[Viewport::MAX_VIEWPORT_COUNT];
    for (uint32_t vp = 0; vp < contextState.graphicsState.viewportCount; ++vp)
      rects[vp] = asRect(contextState.graphicsState.viewports[vp]);
    contextState.cmdBuffer.setScissorRects(make_span_const(rects, contextState.graphicsState.viewportCount));
    contextState.graphicsState.statusBits.reset(GraphicsState::SCISSOR_ENABLED_DIRTY);
  }

  if (contextState.graphicsState.statusBits.test(GraphicsState::VIEWPORT_DIRTY))
  {
    contextState.cmdBuffer.setViewports(
      make_span_const(contextState.graphicsState.viewports, contextState.graphicsState.viewportCount));
    contextState.graphicsState.statusBits.reset(GraphicsState::VIEWPORT_DIRTY);
  }
}

void DeviceContext::ExecutionContext::flushGraphicsResourceBindings()
{
  auto &signature = contextState.graphicsState.basePipeline->getSignature();
  auto &psHeader = contextState.graphicsState.basePipeline->getPSHeader();

  auto &frame = contextState.getFrameData();

  auto &vsStageState = contextState.stageState[STAGE_VS];
  auto &psStageState = contextState.stageState[STAGE_PS];

  auto vsBRegisterDescriptorCount = vsStageState.cauclateBRegisterDescriptorCount(signature.vsCombinedBRegisterMask);
  auto vsTRegisterDescriptorCount = vsStageState.cauclateTRegisterDescriptorCount(signature.vsCombinedTRegisterMask);
  auto vsURegisterDescriptorCount = vsStageState.cauclateURegisterDescriptorCount(signature.vsCombinedURegisterMask);

  auto psBRegisterDescriptorCount = psStageState.cauclateBRegisterDescriptorCount(psHeader.resourceUsageTable.bRegisterUseMask);
  auto psTRegisterDescriptorCount = psStageState.cauclateTRegisterDescriptorCount(psHeader.resourceUsageTable.tRegisterUseMask);
  auto psURegisterDescriptorCount = psStageState.cauclateURegisterDescriptorCount(psHeader.resourceUsageTable.uRegisterUseMask);

  auto totalBRegisterDescriptorCount = vsBRegisterDescriptorCount + psBRegisterDescriptorCount;
  auto totalTRegisterDescriptorCount = vsTRegisterDescriptorCount + psTRegisterDescriptorCount;
  auto totalURegisterDescriptorCount = vsURegisterDescriptorCount + psURegisterDescriptorCount;

  uint32_t bindlessRev = 0;
  uint32_t bindlessCount = 0;
  if (signature.allCombinedBindlessMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
  {
    bindlessRev = contextState.bindlessSetManager.getResourceDescriptorRevision();
    bindlessCount = contextState.bindlessSetManager.getResourceDescriptorCount();
  }

  bool needsFullFlush = frame.resourceViewHeaps.reserveSpace(device.device.get(), totalBRegisterDescriptorCount,
    totalTRegisterDescriptorCount, totalURegisterDescriptorCount, bindlessCount, bindlessRev);

  if (needsFullFlush)
  {
    vsStageState.onResourceDescriptorHeapChanged();
    psStageState.onResourceDescriptorHeapChanged();
  }

  if (signature.allCombinedBindlessMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
  {
    contextState.bindlessSetManager.reserveSamplerHeap(device.device.get(), frame.samplerHeaps);
  }

  psStageState.pushSamplers(device.device.get(), frame.samplerHeaps, device.nullResourceTable.table[NullResourceTable::SAMPLER],
    device.nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE], psHeader.resourceUsageTable.sRegisterUseMask,
    psHeader.sRegisterCompareUseMask);

  vsStageState.pushSamplers(device.device.get(), frame.samplerHeaps, device.nullResourceTable.table[NullResourceTable::SAMPLER],
    device.nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE], signature.vsCombinedSRegisterMask,
    contextState.graphicsState.basePipeline->getVertexShaderSamplerCompareMask());

  psStageState.pushShaderResourceViews(device.device.get(), frame.resourceViewHeaps, device.nullResourceTable,
    psHeader.resourceUsageTable.tRegisterUseMask, psHeader.tRegisterTypes);

  psStageState.pushUnorderedViews(device.device.get(), frame.resourceViewHeaps, device.nullResourceTable,
    psHeader.resourceUsageTable.uRegisterUseMask, psHeader.uRegisterTypes);

  vsStageState.pushShaderResourceViews(device.device.get(), frame.resourceViewHeaps, device.nullResourceTable,
    signature.vsCombinedTRegisterMask, contextState.graphicsState.basePipeline->getVertexShaderCombinedTRegisterTypes());

  vsStageState.pushUnorderedViews(device.device.get(), frame.resourceViewHeaps, device.nullResourceTable,
    signature.vsCombinedURegisterMask, contextState.graphicsState.basePipeline->getVertexShaderCombinedTRegisterTypes());

  auto constBufferMode = signature.def.psLayout.usesConstBufferRootDescriptors(signature.def.vsLayout, signature.def.gsLayout,
                           signature.def.hsLayout, signature.def.dsLayout)
                           ? PipelineStageStateBase::ConstantBufferPushMode::ROOT_DESCRIPTOR
                           : PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP;

  auto nullConstBufferView = device.getNullConstBufferView();

  vsStageState.pushConstantBuffers(device.device.get(), frame.resourceViewHeaps, self.back.constBufferStreamDescriptors,
    nullConstBufferView, signature.vsCombinedBRegisterMask, contextState.cmdBuffer, STAGE_VS, constBufferMode);

  psStageState.pushConstantBuffers(device.device.get(), frame.resourceViewHeaps, self.back.constBufferStreamDescriptors,
    nullConstBufferView, psHeader.resourceUsageTable.bRegisterUseMask, contextState.cmdBuffer, STAGE_PS, constBufferMode);

  for (auto s : {STAGE_VS, STAGE_PS})
  {
    contextState.stageState[s].migrateAllSamplers(device.device.get(), frame.samplerHeaps);
  }

  if (signature.allCombinedBindlessMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
  {
    contextState.bindlessSetManager.pushToSamplerHeap(device.device.get(), frame.samplerHeaps);
  }

  if (signature.allCombinedBindlessMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
  {
    contextState.bindlessSetManager.pushToResourceHeap(device.device.get(), frame.resourceViewHeaps);
  }

  contextState.cmdBuffer.setSamplerHeap(frame.samplerHeaps.getActiveHandle(), frame.samplerHeaps.getBindlessGpuAddress());
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());

  contextState.cmdBuffer.setVertexSamplers(frame.samplerHeaps.getGpuAddress(vsStageState.sRegisterDescriptorRange));
  contextState.cmdBuffer.setPixelSamplers(frame.samplerHeaps.getGpuAddress(psStageState.sRegisterDescriptorRange));

  contextState.cmdBuffer.setVertexSRVs(frame.resourceViewHeaps.getGpuAddress(vsStageState.tRegisterDescriptorRange));
  contextState.cmdBuffer.setPixelSRVs(frame.resourceViewHeaps.getGpuAddress(psStageState.tRegisterDescriptorRange));

  contextState.cmdBuffer.setVertexUAVs(frame.resourceViewHeaps.getGpuAddress(vsStageState.uRegisterDescriptorRange));
  contextState.cmdBuffer.setPixelUAVs(frame.resourceViewHeaps.getGpuAddress(psStageState.uRegisterDescriptorRange));

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  if (PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP == constBufferMode)
  {
    contextState.cmdBuffer.setVertexConstantBufferDescriptors(
      frame.resourceViewHeaps.getGpuAddress(vsStageState.bRegisterDescribtorRange));

    contextState.cmdBuffer.setPixelConstantBufferDescriptors(
      frame.resourceViewHeaps.getGpuAddress(psStageState.bRegisterDescribtorRange));
  }
#endif

  vsStageState.flushResourceStates(signature.vsCombinedBRegisterMask, signature.vsCombinedTRegisterMask,
    signature.vsCombinedURegisterMask, STAGE_VS, contextState.resourceStates, contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker);

  psStageState.flushResourceStates(psHeader.resourceUsageTable.bRegisterUseMask, psHeader.resourceUsageTable.tRegisterUseMask,
    psHeader.resourceUsageTable.uRegisterUseMask, STAGE_PS, contextState.resourceStates, contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker);
}

void DeviceContext::ExecutionContext::flushGraphicsMeshState()
{
  flushRenderTargetStates();
  if (FramebufferLayoutID::Null() == contextState.graphicsState.framebufferLayoutID)
  {
    flushRenderTargets();
  }

  if (!contextState.graphicsState.pipeline)
  {
    contextState.graphicsState.pipeline =
      &contextState.graphicsState.basePipeline->getMeshVariantFromConfiguration(contextState.graphicsState.staticRenderStateIdent,
        contextState.graphicsState.framebufferLayoutID, contextState.graphicsState.statusBits.test(GraphicsState::USE_WIREFRAME));

    if (!contextState.graphicsState.pipeline->isReady())
    {
      auto &staticRenderState = device.pipeMan.getStaticRenderState(contextState.graphicsState.staticRenderStateIdent);

      contextState.graphicsState.pipeline->loadMesh(device.device.get(), device.pipeMan, *contextState.graphicsState.basePipeline,
        device.pipelineCache, contextState.graphicsState.statusBits.test(GraphicsState::USE_WIREFRAME), staticRenderState,
        contextState.graphicsState.framebufferState.framebufferLayout,
        get_recover_behvior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
          device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
        device.shouldNameObjects());
    }

    contextState.cmdBuffer.bindGraphicsMeshPipeline(&contextState.graphicsState.basePipeline->getSignature(),
      contextState.graphicsState.pipeline->get(), contextState.graphicsState.pipeline->getUsedOptionalDynamicStateMask());
  }

  flushViewportAndScissor();
  flushGraphicsResourceBindings();
}

void DeviceContext::ExecutionContext::flushGraphicsState(D3D12_PRIMITIVE_TOPOLOGY top)
{
  flushRenderTargetStates();
  if (FramebufferLayoutID::Null() == contextState.graphicsState.framebufferLayoutID)
  {
    flushRenderTargets();
  }

  if (!contextState.graphicsState.basePipeline)
  {
    return;
  }

  // for hull shader it overrides the final topology type
  if (contextState.graphicsState.basePipeline->hasTessellationStage())
  {
    top = pimitive_type_to_primtive_topology(contextState.graphicsState.basePipeline->getHullInputPrimitiveType(), top);
  }

  auto topType = topology_to_topology_type(top);
  if (!contextState.graphicsState.pipeline || contextState.graphicsState.topology != topType)
  {
    contextState.graphicsState.topology = topType;

    auto inputLayoutMask = contextState.graphicsState.basePipeline->getVertexShaderInputMask();
    auto internlInputLayout = device.pipeMan.remapInputLayout(contextState.graphicsState.inputLayoutIdent, inputLayoutMask);

    contextState.graphicsState.pipeline = &contextState.graphicsState.basePipeline->getVariantFromConfiguration(internlInputLayout,
      contextState.graphicsState.staticRenderStateIdent, contextState.graphicsState.framebufferLayoutID, topType,
      contextState.graphicsState.statusBits.test(GraphicsState::USE_WIREFRAME));

    if (!contextState.graphicsState.pipeline->isReady())
    {
      auto &inputLayout = device.pipeMan.getInputLayout(internlInputLayout);

      auto &staticRenderState = device.pipeMan.getStaticRenderState(contextState.graphicsState.staticRenderStateIdent);

      contextState.graphicsState.pipeline->load(device.device.get(), device.pipeMan, *contextState.graphicsState.basePipeline,
        device.pipelineCache, inputLayout, contextState.graphicsState.statusBits.test(GraphicsState::USE_WIREFRAME), staticRenderState,
        contextState.graphicsState.framebufferState.framebufferLayout, topType,
        get_recover_behvior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
          device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
        device.shouldNameObjects());
    }
    contextState.cmdBuffer.bindGraphicsPipeline(&contextState.graphicsState.basePipeline->getSignature(),
      contextState.graphicsState.pipeline->get(), contextState.graphicsState.basePipeline->hasGeometryStage(),
      contextState.graphicsState.basePipeline->hasTessellationStage(),
      contextState.graphicsState.pipeline->getUsedOptionalDynamicStateMask());
  }

  contextState.cmdBuffer.setPrimitiveTopology(top);

  flushViewportAndScissor();
  flushGraphicsResourceBindings();
}

void DeviceContext::ExecutionContext::flushIndexBuffer()
{
  auto &buffer = contextState.graphicsState.indexBuffer;

  if (contextState.graphicsState.statusBits.test(GraphicsState::INDEX_BUFFER_DIRTY))
  {
    contextState.graphicsState.statusBits.reset(GraphicsState::INDEX_BUFFER_DIRTY);

    auto type = contextState.graphicsState.indexBufferFormat;

    if (buffer && readyCommandList())
    {
      D3D12_INDEX_BUFFER_VIEW view{buffer.gpuPointer, buffer.size, type};
      contextState.cmdBuffer.iaSetIndexBuffer(&view);
    }
  }

  if (contextState.graphicsState.statusBits.test(GraphicsState::INDEX_BUFFER_STATE_DIRTY))
  {
    contextState.graphicsState.statusBits.reset(GraphicsState::INDEX_BUFFER_STATE_DIRTY);
    contextState.resourceStates.useBufferAsIB(contextState.graphicsCommandListBarrierBatch, buffer);
    contextState.bufferAccessTracker.updateLastFrameAccess(buffer.resourceId);
  }
}

void DeviceContext::ExecutionContext::flushVertexBuffers()
{
  using eastl::begin;
  using eastl::end;

  // find range of which buffers we need to update
  uint32_t first = contextState.graphicsState.statusBits.test(GraphicsState::VERTEX_BUFFER_0_DIRTY) ? 0 : MAX_VERTEX_INPUT_STREAMS;
  uint32_t last = contextState.graphicsState.statusBits.test(GraphicsState::VERTEX_BUFFER_0_DIRTY) ? 1 : 0;
  for (uint32_t i = 1; i < MAX_VERTEX_INPUT_STREAMS; ++i)
  {
    if (contextState.graphicsState.statusBits.test(GraphicsState::VERTEX_BUFFER_0_DIRTY + i))
    {
      first = min(first, i);
      last = i + 1;
    }
  }

  // now create the view table
  D3D12_VERTEX_BUFFER_VIEW views[4];
  for (uint32_t i = first; i < last; ++i)
  {
    auto &buffer = contextState.graphicsState.vertexBuffers[i];
    auto &view = views[i];

    view.BufferLocation = buffer.gpuPointer;
    view.SizeInBytes = buffer.size;
    view.StrideInBytes = contextState.graphicsState.vertexBufferStrides[i];
  }

  // if anything needs to be set, set it now
  if (first < last && readyCommandList())
  {
    auto count = last - first;
    contextState.cmdBuffer.iaSetVertexBuffers(first, count, &views[first]);
  }

  // check buffer state and reset status bits
  for (uint32_t i = 0; i < MAX_VERTEX_INPUT_STREAMS; ++i)
  {
    if (contextState.graphicsState.statusBits.test(GraphicsState::VERTEX_BUFFER_STATE_0_DIRTY + i) &&
        contextState.graphicsState.vertexBuffers[i].resourceId)
    {
      contextState.resourceStates.useBufferAsVB(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsState.vertexBuffers[i]);
      contextState.bufferAccessTracker.updateLastFrameAccess(contextState.graphicsState.vertexBuffers[i].resourceId);
    }

    contextState.graphicsState.statusBits.reset(GraphicsState::VERTEX_BUFFER_0_DIRTY + i);
    contextState.graphicsState.statusBits.reset(GraphicsState::VERTEX_BUFFER_STATE_0_DIRTY + i);
  }
}

void DeviceContext::ExecutionContext::flushGraphicsStateRessourceBindings()
{
  if (!contextState.graphicsState.basePipeline)
  {
    return;
  }

  auto &signature = contextState.graphicsState.basePipeline->getSignature();

  auto &pixelShaderRegisters = contextState.graphicsState.basePipeline->getPSHeader();

  // we can do this here as graphics never flush during a pass
  contextState.stageState[STAGE_VS].enumerateUAVResources(signature.vsCombinedURegisterMask,
    [this](auto res) //
    { contextState.resourceStates.beginImplicitUAVAccess(res); });

  contextState.stageState[STAGE_PS].enumerateUAVResources(pixelShaderRegisters.resourceUsageTable.uRegisterUseMask,
    [this](auto res) //
    { contextState.resourceStates.beginImplicitUAVAccess(res); });
}

void DeviceContext::ExecutionContext::ensureActivePass() {}

void DeviceContext::ExecutionContext::changePresentMode(PresentationMode mode) { self.back.swapchain.changePresentMode(mode); }

void DeviceContext::ExecutionContext::updateVertexShaderName(ShaderID shader, StringIndexRef::RangeType name)
{
  device.pipeMan.setVertexShaderName(shader, name);
}

void DeviceContext::ExecutionContext::updatePixelShaderName(ShaderID shader, StringIndexRef::RangeType name)
{
  device.pipeMan.setPixelShaderName(shader, name);
}

void DeviceContext::ExecutionContext::clearUAVTextureI(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
  const uint32_t values[4])
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.useTextureAsUAVForClear(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image, view);

  tranistionPredicationBuffer();

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  auto &frame = contextState.getFrameData();
  auto gpuIndex = frame.resourceViewHeaps.findInUAVScratchSegment(view_descriptor);
  if (!gpuIndex)
  {
    // reserve space for one UAV descriptor
    frame.resourceViewHeaps.reserveSpace(device.device.get(), 0, 0, 1, 0, 0);
    gpuIndex = frame.resourceViewHeaps.appendToUAVScratchSegment(device.device.get(), view_descriptor);
  }
  auto gpuHandle = frame.resourceViewHeaps.getGpuAddress(gpuIndex);
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());

  applyPredicationBuffer();

  contextState.cmdBuffer.clearUnorderedAccessViewUint(gpuHandle, view_descriptor, image->getHandle(), values, 0, nullptr);

  dirtyTextureState(image);
}

void DeviceContext::ExecutionContext::clearUAVTextureF(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
  const float values[4])
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.useTextureAsUAVForClear(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image, view);

  tranistionPredicationBuffer();

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  auto &frame = contextState.getFrameData();
  auto gpuIndex = frame.resourceViewHeaps.findInUAVScratchSegment(view_descriptor);
  if (!gpuIndex)
  {
    // reserve space for one UAV descriptor
    frame.resourceViewHeaps.reserveSpace(device.device.get(), 0, 0, 1, 0, 0);
    gpuIndex = frame.resourceViewHeaps.appendToUAVScratchSegment(device.device.get(), view_descriptor);
  }
  auto gpuHandle = frame.resourceViewHeaps.getGpuAddress(gpuIndex);
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());

  applyPredicationBuffer();

  contextState.cmdBuffer.clearUnorderedAccessViewFloat(gpuHandle, view_descriptor, image->getHandle(), values, 0, nullptr);

  dirtyTextureState(image);
}

void DeviceContext::ExecutionContext::setGamma(float power)
{
#if DX12_HAS_GAMMA_CONTROL
  self.back.swapchain.setGamma(power);
#else
  G_UNUSED(power);
#endif
}

void DeviceContext::ExecutionContext::setComputeRootConstant(uint32_t offset, uint32_t value)
{
  contextState.cmdBuffer.updateComputeRootConstant(offset, value);
}

void DeviceContext::ExecutionContext::setVertexRootConstant(uint32_t offset, uint32_t value)
{
  contextState.cmdBuffer.updateVertexRootConstant(offset, value);
}

void DeviceContext::ExecutionContext::setPixelRootConstant(uint32_t offset, uint32_t value)
{
  contextState.cmdBuffer.updatePixelRootConstant(offset, value);
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::setRaytraceRootConstant(uint32_t offset, uint32_t value)
{
  contextState.cmdBuffer.updateRaytraceRootConstant(offset, value);
}
#endif

void DeviceContext::ExecutionContext::beginVisibilityQuery(Query *q)
{
  if (readyCommandList())
  {
    contextState.getFrameData().backendQueryManager.makeVisibilityQuery(q, device.device.get(), contextState.cmdBuffer.getHandle());
  }
}

void DeviceContext::ExecutionContext::endVisibilityQuery(Query *q)
{
  if (readyCommandList())
  {
    contextState.getFrameData().backendQueryManager.endVisibilityQuery(q, contextState.cmdBuffer.getHandle());
  }
}

void DeviceContext::ExecutionContext::flushComputeState()
{
  if (!contextState.computeState.pipeline)
  {
    return;
  }

  auto &pipelineHeader = contextState.computeState.pipeline->getHeader();
  auto &signature = contextState.computeState.pipeline->getSignature();

  auto &frame = contextState.getFrameData();
  auto &csStageState = contextState.stageState[STAGE_CS];

  auto constBufferMode = signature.def.csLayout.usesConstBufferRootDescriptors()
                           ? PipelineStageStateBase::ConstantBufferPushMode::ROOT_DESCRIPTOR
                           : PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP;

  auto bRegisterCount = csStageState.cauclateBRegisterDescriptorCount(pipelineHeader.resourceUsageTable.bRegisterUseMask);
  auto tRegisterCount = csStageState.cauclateTRegisterDescriptorCount(pipelineHeader.resourceUsageTable.tRegisterUseMask);
  auto uRegisterCount = csStageState.cauclateURegisterDescriptorCount(pipelineHeader.resourceUsageTable.uRegisterUseMask);

  // TODO needs only top be done if the shaders use bindless
  uint32_t bindlessCount = contextState.bindlessSetManager.getResourceDescriptorCount();
  uint32_t bindlessRev = contextState.bindlessSetManager.getResourceDescriptorRevision();

  bool needsFullFlush = frame.resourceViewHeaps.reserveSpace(device.device.get(), bRegisterCount, tRegisterCount, uRegisterCount,
    bindlessCount, bindlessRev);

  if (needsFullFlush)
  {
    csStageState.onResourceDescriptorHeapChanged();
  }

  // TODO needs only top be done if the shaders use bindless
  contextState.bindlessSetManager.reserveSamplerHeap(device.device.get(), frame.samplerHeaps);

  csStageState.pushSamplers(device.device.get(), frame.samplerHeaps, device.nullResourceTable.table[NullResourceTable::SAMPLER],
    device.nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE], pipelineHeader.resourceUsageTable.sRegisterUseMask,
    pipelineHeader.sRegisterCompareUseMask);

  csStageState.pushShaderResourceViews(device.device.get(), frame.resourceViewHeaps, device.nullResourceTable,
    pipelineHeader.resourceUsageTable.tRegisterUseMask, pipelineHeader.tRegisterTypes);

  csStageState.pushUnorderedViews(device.device.get(), frame.resourceViewHeaps, device.nullResourceTable,
    pipelineHeader.resourceUsageTable.uRegisterUseMask, pipelineHeader.uRegisterTypes);

  // TODO needs only top be done if the shaders use bindless
  contextState.bindlessSetManager.pushToSamplerHeap(device.device.get(), frame.samplerHeaps);
  contextState.bindlessSetManager.pushToResourceHeap(device.device.get(), frame.resourceViewHeaps);

  auto nullConstBufferView = device.getNullConstBufferView();

  csStageState.pushConstantBuffers(device.device.get(), frame.resourceViewHeaps, self.back.constBufferStreamDescriptors,
    nullConstBufferView, pipelineHeader.resourceUsageTable.bRegisterUseMask, contextState.cmdBuffer, STAGE_CS, constBufferMode);

  csStageState.migrateAllSamplers(device.device.get(), frame.samplerHeaps);

  contextState.cmdBuffer.setSamplerHeap(frame.samplerHeaps.getActiveHandle(), frame.samplerHeaps.getBindlessGpuAddress());
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());

  contextState.cmdBuffer.setComputeSamplers(frame.samplerHeaps.getGpuAddress(csStageState.sRegisterDescriptorRange));

  contextState.cmdBuffer.setComputeSRVs(frame.resourceViewHeaps.getGpuAddress(csStageState.tRegisterDescriptorRange));

  contextState.cmdBuffer.setComputeUAVs(frame.resourceViewHeaps.getGpuAddress(csStageState.uRegisterDescriptorRange));

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  if (PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP == constBufferMode)
  {
    contextState.cmdBuffer.setComputeConstantBufferDescriptors(
      frame.resourceViewHeaps.getGpuAddress(csStageState.bRegisterDescribtorRange));
  }
#endif

  csStageState.flushResourceStates(pipelineHeader.resourceUsageTable.bRegisterUseMask,
    pipelineHeader.resourceUsageTable.tRegisterUseMask, pipelineHeader.resourceUsageTable.uRegisterUseMask, STAGE_CS,
    contextState.resourceStates, contextState.graphicsCommandListBarrierBatch, contextState.graphicsCommandListSplitBarrierTracker);
}

void DeviceContext::ExecutionContext::textureReadBack(Image *image, HostDeviceSharedMemoryRegion cpu_memory,
  BufferImageCopyListRef::RangeType regions, DeviceQueueType queue)
{
  D3D12_TEXTURE_COPY_LOCATION src;
  src.pResource = image->getHandle();
  src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

  if (DeviceQueueType::READ_BACK == queue)
  {
    // defers read back after the frame has ended onto the read back queue.
    if (!contextState.activeReadBackCommandList)
    {
      auto &frame = contextState.getFrameData();
      contextState.activeReadBackCommandList = frame.readBackCommands.allocateList(device.device.get());
    }

    if (!contextState.activeReadBackCommandList)
    {
      // on error
      return;
    }

    for (auto &&region : regions)
    {
      contextState.resourceStates.beginReadyTextureAsReadBack(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, image, SubresourceIndex::make(region.subresourceIndex));
      contextState.textureReadBackSplitBarrierEnds.emplace_back(image, SubresourceIndex::make(region.subresourceIndex));

      src.SubresourceIndex = region.subresourceIndex;

      D3D12_TEXTURE_COPY_LOCATION dst;
      dst.pResource = cpu_memory.buffer;
      dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      dst.PlacedFootprint = region.layout;
      dst.PlacedFootprint.Offset += cpu_memory.range.front();

      contextState.activeReadBackCommandList.copyTextureRegion(&dst, region.imageOffset.x, region.imageOffset.y, region.imageOffset.z,
        &src, nullptr);
    }
  }
  else if (DeviceQueueType::GRAPHICS == queue)
  {
    if (!readyCommandList())
    {
      return;
    }
    disablePredication();

    // executed the copy from texture to buffer directly on the 3d queue.
    // this is used for swapchain read back, otherwise we have to copy the
    // swapchain into a temp texture, which may blow resource limits.
    for (auto &&region : regions)
    {
      contextState.resourceStates.useTextureAsReadBack(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, image, SubresourceIndex::make(region.subresourceIndex));
    }

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    for (auto &&region : regions)
    {
      src.SubresourceIndex = region.subresourceIndex;

      D3D12_TEXTURE_COPY_LOCATION dst;
      dst.pResource = cpu_memory.buffer;
      dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      dst.PlacedFootprint = region.layout;
      dst.PlacedFootprint.Offset += cpu_memory.range.front();

      contextState.cmdBuffer.copyTexture(&dst, region.imageOffset.x, region.imageOffset.y, region.imageOffset.z, &src, nullptr);
    }

    dirtyTextureState(image);
  }
  else
  {
    fatal("DX12: Texture read back requested on invalid queue %u", static_cast<uint32_t>(queue));
  }
}

void DeviceContext::ExecutionContext::bufferReadBack(BufferResourceReferenceAndRange buffer, HostDeviceSharedMemoryRegion cpu_memory,
  size_t offset)
{
  auto dstOffset = cpu_memory.range.front() + offset;
  auto srcOffset = buffer.offset;
#if !_TARGET_XBOXONE
  auto infoD = cpu_memory.buffer->GetDesc();
  auto infoS = buffer.buffer->GetDesc();
  if ((infoD.Width < (dstOffset + buffer.size)) || (infoS.Width < (srcOffset + buffer.size)))
  {
    wchar_t wcbuf[MAX_OBJECT_NAME_LENGTH];
    char cbuf[MAX_OBJECT_NAME_LENGTH];
    UINT cnt = sizeof(wcbuf);
    logerr("DX12: Read Back Copy out of range (skipping):");
    cpu_memory.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
    eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
    logerr("DX12: Dest '%s' Size %u, copy range %u - %u", cbuf, infoD.Width, dstOffset, dstOffset + buffer.size);

    buffer.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
    eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
    logerr("DX12: Source '%s' Size %u, copy range %u - %u", cbuf, infoS.Width, srcOffset, srcOffset + buffer.size);
    return;
  }
#endif
  if (readyReadBackCommandList())
  {
    contextState.activeReadBackCommandList.copyBufferRegion(cpu_memory.buffer, dstOffset, buffer.buffer, srcOffset, buffer.size);
  }
}

#if !_TARGET_XBOXONE
void DeviceContext::ExecutionContext::setVariableRateShading(D3D12_SHADING_RATE rate, D3D12_SHADING_RATE_COMBINER vs_combiner,
  D3D12_SHADING_RATE_COMBINER ps_combiner)
{
  contextState.cmdBuffer.setShadingRate(rate, vs_combiner, ps_combiner);
}

void DeviceContext::ExecutionContext::setVariableRateShadingTexture(Image *texture)
{
  if (texture)
  {
    contextState.resourceStates.useTextureAsVRSSource(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, texture);
    contextState.cmdBuffer.setShadingRateTexture(texture->getHandle());
    dirtyTextureState(texture);
  }
  else
  {
    contextState.cmdBuffer.setShadingRateTexture(nullptr);
  }
}
#endif

void DeviceContext::ExecutionContext::registerInputLayout(InputLayoutID ident, const InputLayout &layout)
{
  device.pipeMan.registerInputLayout(ident, layout);
}

void DeviceContext::ExecutionContext::createDlssFeature(int dlss_quality, Extent2D output_resolution, bool stereo_render)
{
  if (!readyCommandList())
  {
    return;
  }

  // note sure if we have to, but better do it and don't worry
  disablePredication();

  self.ngxWrapper.createOptimalDlssFeature(static_cast<void *>(contextState.cmdBuffer.getHandle()), output_resolution.width,
    output_resolution.height, dlss_quality, stereo_render);
}

void DeviceContext::ExecutionContext::releaseDlssFeature() { self.ngxWrapper.releaseDlssFeature(); }

void DeviceContext::ExecutionContext::prepareExecuteAA(Image *inColor, Image *inDepth, Image *inMotionVectors, Image *outColor)
{
  // not sure if we have to, but better do it and don't worry
  disablePredication();

  for (Image *image : {inColor, inDepth, inMotionVectors})
  {
    contextState.resourceStates.useTextureAsDLSSInput(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, image);
    dirtyTextureState(image);
  }

  contextState.resourceStates.useTextureAsDLSSOutput(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, outColor);
  dirtyTextureState(outColor);

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);
}

void DeviceContext::ExecutionContext::executeDlss(const DlssParamsDx12 &dlss_params, int view_index)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA(dlss_params.inColor, dlss_params.inDepth, dlss_params.inMotionVectors, dlss_params.outColor);

  // DLSS programming guide section 3.4:
  // "After the evaluate call, DLSS accesses and processes the buffers and may change their state
  // but always transitions buffers back to these known states".
  self.ngxWrapper.evaluateDlss(static_cast<void *>(contextState.cmdBuffer.getHandle()), static_cast<const void *>(&dlss_params),
    view_index);

  // DLSS alters command list state so we need to reset everything afterwards to keep consistency
  contextState.cmdBuffer.dirtyAll();
}


void DeviceContext::ExecutionContext::executeXess(const XessParamsDx12 &params)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA(params.inColor, params.inDepth, params.inMotionVectors, params.outColor);

  self.xessWrapper.evaluateXess(static_cast<void *>(contextState.cmdBuffer.getHandle()), static_cast<const void *>(&params));

  contextState.cmdBuffer.dirtyAll();
}

void DeviceContext::ExecutionContext::executeFSR2(const Fsr2ParamsDx12 &params)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA(params.inColor, params.inDepth, params.inMotionVectors, params.outColor);
  self.fsr2Wrapper.evaluateFsr2(static_cast<void *>(contextState.cmdBuffer.getHandle()), static_cast<const void *>(&params));
  contextState.cmdBuffer.dirtyAll();
}

void DeviceContext::ExecutionContext::beginCapture(UINT flags, WStringIndexRef::RangeType name)
{
  contextState.debugFrameCaptureBegin(device, device.queues[DeviceQueueType::GRAPHICS].getHandle(), flags, name);
}

void DeviceContext::ExecutionContext::endCapture()
{
  contextState.debugFrameCaptureEnd(device, device.queues[DeviceQueueType::GRAPHICS].getHandle());
}

void DeviceContext::ExecutionContext::captureNextFrames(UINT flags, WStringIndexRef::RangeType name, int frame_count)
{
  contextState.debugFrameCaptureQueueNextFrames(device, device.queues[DeviceQueueType::GRAPHICS].getHandle(), flags, name,
    frame_count);
}

void DeviceContext::ExecutionContext::removeVertexShader(ShaderID shader)
{
  auto &frame = contextState.getFrameData();
  auto module = device.pipeMan.releaseVertexShader(shader);
  frame.deletedShaderModules.push_back(eastl::move(module));
}

void DeviceContext::ExecutionContext::removePixelShader(ShaderID shader)
{
  auto &frame = contextState.getFrameData();
  auto module = device.pipeMan.releasePixelShader(shader);
  frame.deletedShaderModules.push_back(eastl::move(module));
}

void DeviceContext::ExecutionContext::deleteProgram(ProgramID program)
{
  auto &frame = contextState.getFrameData();

  frame.deletedPrograms.push_back(program);
  device.pipeMan.prepareForRemove(program);
}

void DeviceContext::ExecutionContext::deleteGraphicsProgram(GraphicsProgramID program)
{
  auto &frame = contextState.getFrameData();

  frame.deletedGraphicPrograms.push_back(program);
  device.pipeMan.prepareForRemove(program);
}

void DeviceContext::ExecutionContext::deleteQueries(QueryPointerListRef::RangeType queries)
{
  auto &frame = contextState.getFrameData();
  frame.deletedQueries.insert(end(frame.deletedQueries), begin(queries), end(queries));
}

void DeviceContext::ExecutionContext::hostToDeviceMemoryCopy(BufferResourceReferenceAndRange target,
  HostDeviceSharedMemoryRegion source, size_t source_offset)
{
  auto dstOffset = target.offset;
  auto srcOffset = source.range.front() + source_offset;
  auto copyBufferRegion = [&](CopyCommandList<VersionedPtr<D3DCopyCommandList>> &activeUploadCommandList) {
#if !_TARGET_XBOXONE
    auto infoD = target.buffer->GetDesc();
    auto infoS = source.buffer->GetDesc();
    if ((infoD.Width < (dstOffset + target.size)) || (infoS.Width < (srcOffset + target.size)))
    {
      wchar_t wcbuf[MAX_OBJECT_NAME_LENGTH];
      char cbuf[MAX_OBJECT_NAME_LENGTH];
      UINT cnt = sizeof(wcbuf);
      logerr("DX12: Upload Copy out of range (skipping):");
      target.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
      eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
      logerr("DX12: Dest '%s' Size %u, copy range %u - %u", cbuf, infoD.Width, dstOffset, dstOffset + target.size);

      source.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
      eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
      logerr("DX12: Source '%s' Size %u, copy range %u - %u", cbuf, infoS.Width, srcOffset, srcOffset + target.size);
      return;
    }
#endif
    activeUploadCommandList.copyBufferRegion(target.buffer, dstOffset, source.buffer, srcOffset, target.size);
  };

  if (contextState.bufferAccessTracker.wasAccessedPreviousFrames(target.resourceId))
  {
    if (!readyLateUploadCommandList())
    {
      return;
    }

    copyBufferRegion(contextState.activeLateUploadCommandList);
  }
  else
  {
    if (!readyEarlyUploadCommandList())
    {
      return;
    }

    copyBufferRegion(contextState.activeEarlyUploadCommandList);
  }
}

void DeviceContext::ExecutionContext::initializeTextureState(D3D12_RESOURCE_STATES state,
  ValueRange<ExtendedImageGlobalSubresouceId> id_range)
{
  contextState.resourceStates.setTextureState(contextState.initialResourceStateSet, id_range, state);
}

void DeviceContext::ExecutionContext::uploadTexture(Image *target, BufferImageCopyListRef::RangeType regions,
  HostDeviceSharedMemoryRegion source, DeviceQueueType queue, bool is_discard)
{
  bool shouldUseGraphicsQueue = DeviceQueueType::GRAPHICS == queue;
  if (DeviceQueueType::GRAPHICS != queue)
  {
    // To avoid issues we update the swapchain only on the graphics queue.
    if (is_swapchain_color_image(target))
    {
      shouldUseGraphicsQueue = true;
    }
    // If its a discard, we can move it in front of the frame if no one was readying it until now.
    else if (is_discard && target->wasAccessedThisFrame(self.back.frameProgress))
    {
      shouldUseGraphicsQueue = true;
    }
  }
  if (shouldUseGraphicsQueue)
  {
    if (!readyCommandList())
    {
      return;
    }
    disablePredication();

    for (auto &&region : regions)
    {
      contextState.resourceStates.useTextureAsUploadDestination(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, target, SubresourceIndex::make(region.subresourceIndex));
    }

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    D3D12_TEXTURE_COPY_LOCATION dst;
    dst.pResource = target->getHandle();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    for (auto &&region : regions)
    {
      dst.SubresourceIndex = region.subresourceIndex;

      D3D12_TEXTURE_COPY_LOCATION src;
      src.pResource = source.buffer;
      src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      src.PlacedFootprint = region.layout;
      src.PlacedFootprint.Offset += source.range.front();

      contextState.cmdBuffer.copyTexture(&dst, region.imageOffset.x, region.imageOffset.y, region.imageOffset.z, &src, nullptr);
    }

    for (auto &&region : regions)
    {
      contextState.resourceStates.finishUseTextureAsUploadDestination(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, target, SubresourceIndex::make(region.subresourceIndex));
    }

    target->updateLastFrameAccess(self.back.frameProgress);

    dirtyTextureState(target);
  }
  else
  {
    D3D12_TEXTURE_COPY_LOCATION dst;
    dst.pResource = target->getHandle();
    dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

    for (auto &&region : regions)
    {
      dst.SubresourceIndex = region.subresourceIndex;

      D3D12_TEXTURE_COPY_LOCATION src;
      src.pResource = source.buffer;
      src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
      src.PlacedFootprint = region.layout;
      src.PlacedFootprint.Offset += source.range.front();

      bool barrierWasPlaced =
        contextState.resourceStates.beginUseTextureAsUploadDestinationOnCopyQueue(contextState.uploadBarrierBatch,
          contextState.initialResourceStateSet, target, SubresourceIndex::make(region.subresourceIndex));

      bool wasUsedInPreviousFrames = target->wasAccessedPreviousFrames(self.back.frameProgress);

      if (barrierWasPlaced || wasUsedInPreviousFrames)
      {
        if (!readyLateUploadCommandList())
        {
          break;
        }

        contextState.activeLateUploadCommandList.copyTextureRegion(&dst, region.imageOffset.x, region.imageOffset.y,
          region.imageOffset.z, &src, nullptr);
      }
      else
      {
        if (!readyEarlyUploadCommandList())
        {
          break;
        }

        contextState.activeEarlyUploadCommandList.copyTextureRegion(&dst, region.imageOffset.x, region.imageOffset.y,
          region.imageOffset.z, &src, nullptr);
      }

      contextState.resourceStates.finishUseTextureAsUploadDestinationOnCopyQueue(contextState.postUploadBarrierBatch, target,
        SubresourceIndex::make(region.subresourceIndex));
    }
  }
}

#if _TARGET_XBOX
void DeviceContext::ExecutionContext::updateFrameInterval(int32_t freq_level)
{
  device.context.back.swapchain.updateFrameInterval(device.device.get(), false, freq_level);
}
void DeviceContext::ExecutionContext::resummarizeHtile(ID3D12Resource *depth) { contextState.cmdBuffer.resummarizeHtile(depth); }
#endif

void DeviceContext::ExecutionContext::bufferBarrier(BufferResourceReference buffer, ResourceBarrier barrier, GpuPipeline)
{
  // aliasing barriers are not optional, the driver can not generate them, but state transition
  // barriers are on the other hand are optional.
  // Alias all can end up here, as the single param constructor of the barrier definition type
  // uses the buffer entry.
  if (RB_ALIAS_ALL == barrier)
  {
    contextState.graphicsCommandListBarrierBatch.flushAliasAll();
    return;
  }

  if (RB_NONE == barrier && buffer)
  {
    contextState.resourceStates.userBufferUAVAccessFlushSkip(buffer);
    return;
  }

  if (!device.processesUserBarriers())
    return;

  dirtyBufferState(buffer.resourceId);

#if DX12_PRINT_USER_BUFFER_BARRIERS
  if (buffer)
  {
    char cbuf[MAX_OBJECT_NAME_LENGTH];
    char maskNameBuffer[256];
    auto state = translate_buffer_barrier_to_state(barrier);
    make_resource_barrier_string_from_state(maskNameBuffer, array_size(maskNameBuffer), state, barrier);
    debug("DX12: Resource barrier for buffer %s - %p, with %s, during %s", get_resource_name(buffer.buffer, cbuf), buffer.buffer,
      maskNameBuffer, getEventPath());
  }
#endif

  if (RB_NONE != (RB_FLUSH_UAV & barrier))
  {
    if (!buffer.buffer)
    {
      contextState.resourceStates.userUAVAccessFlush();
    }
    else
    {
      contextState.resourceStates.userBufferUAVAccessFlush(buffer);
    }
  }
  else if (buffer)
  {
    contextState.resourceStates.userBufferTransitionBarrier(contextState.graphicsCommandListBarrierBatch, buffer, barrier);
  }
}

void DeviceContext::ExecutionContext::textureBarrier(Image *tex, SubresourceRange sub_res_range, uint32_t tex_flags,
  ResourceBarrier barrier, GpuPipeline, bool force_barrier)
{
  if (!readyCommandList())
  {
    return;
  }


  if (RB_ALIAS_ALL == barrier)
  {
    contextState.graphicsCommandListBarrierBatch.flushAliasAll();
    return;
  }


  if (RB_NONE != (RB_ALIAS_FROM & barrier))
  {
    // This just ensures that the resource is in a consistent state on repeated begin / end alias
    // barriers
    if (tex)
    {
      contextState.resourceStates.useTextureToAliasFrom(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, tex, tex_flags);

      G_ASSERTF(nullptr == contextState.lastAliasBegin, "DX12: Missing end aliasing barrier?");
      contextState.lastAliasBegin = tex->getHandle();
    }
    else
    {
      contextState.lastAliasBegin = nullptr;
    }
    return;
  }


  if (RB_NONE != (RB_ALIAS_TO & barrier))
  {
    ID3D12Resource *resouce = tex ? tex->getHandle() : nullptr;

    const bool validAliasingBarrier =
      ((nullptr == contextState.lastAliasBegin) && (nullptr == resouce)) ||
      ((nullptr != resouce) && (nullptr != contextState.lastAliasBegin) && (resouce != contextState.lastAliasBegin));

    G_ASSERTF(validAliasingBarrier, "DX12: Missing begin aliasing barrier?");
    if (validAliasingBarrier)
    {
      contextState.graphicsCommandListBarrierBatch.flushAlias(contextState.lastAliasBegin, resouce);
    }

    contextState.lastAliasBegin = nullptr;

    if (RB_NONE != (RB_FLAG_DONT_PRESERVE_CONTENT & barrier))
    {
      if (!tex)
      {
        logerr("DX12: RB_ALIAS_TO_AND_DISCARD used with null resource!");
        return;
      }

      // discard requested, need to be in the right state, RT, DS or UAV
      contextState.resourceStates.useTextureToAliasToAndDiscard(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, tex, tex_flags);

      contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch,
        device.currentEventPath().data(), device.validatesUserBarriers());
      contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

      contextState.cmdBuffer.discardResource(tex->getHandle(), nullptr);
    }
    return;
  }

  if (RB_NONE == barrier && tex)
  {
    contextState.resourceStates.userTextureUAVAccessFlushSkip(tex);
    return;
  }

  // aliasing barriers are not optional, the driver can not generate them, but state transition
  // barriers are on the other hand are optional
  if (!device.processesUserBarriers() && !force_barrier)
    return;

  dirtyTextureState(tex);

#if DX12_PRINT_USER_TEXTURE_BARRIERS
  if (tex)
  {
    char cbuf[MAX_OBJECT_NAME_LENGTH];
    char maskNameBuffer[256];
    auto state = translate_texture_barrier_to_state(barrier, !tex->getFormat().isColor());
    make_resource_barrier_string_from_state(maskNameBuffer, array_size(maskNameBuffer), state, barrier);
    debug("DX12: Resource barrier for texture %s - %p [%u - %u], with %s, during %s", static_cast<uint32_t>(barrier),
      get_resource_name(tex->getHandle(), cbuf), tex->getHandle(), sub_res_range.start, sub_res_range.stop, maskNameBuffer,
      this->getEventPath());
  }
#endif

  if (RB_NONE != (RB_FLUSH_UAV & barrier))
  {
    if (tex && tex->getHandle())
    {
      contextState.resourceStates.userTextureUAVAccessFlush(tex);
    }
    else
    {
      contextState.resourceStates.userUAVAccessFlush();
    }
  }
  else if (tex)
  {
    if (RB_NONE != (RB_FLAG_SPLIT_BARRIER_BEGIN & barrier))
    {
      contextState.resourceStates.userTextureTransitionSplitBarrierBegin(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, tex, sub_res_range, barrier);
    }
    else if (RB_NONE != (RB_FLAG_SPLIT_BARRIER_END & barrier))
    {
      contextState.resourceStates.userTextureTransitionSplitBarrierEnd(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, tex, sub_res_range, barrier);
    }
    else
    {
      contextState.resourceStates.userTextureTransitionBarrier(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, tex, sub_res_range, barrier);
    }
  }
}

#if _TARGET_XBOX
void DeviceContext::ExecutionContext::enterSuspendState()
{
  debug("DX12: Engine requested to suspend...");
  debug("DX12: Entered suspended state, notifying engine and waiting for resume "
        "request...");
  // Acknowledge that we are now in suspended state and wait for resume signal
  SignalObjectAndWait(self.enteredSuspendedStateEvent.get(), self.resumeExecutionEvent.get(), INFINITE, FALSE);
  debug("DX12: Engine requested to resume...");
}
#endif

void DeviceContext::ExecutionContext::writeDebugMessage(StringIndexRef::RangeType message, int severity)
{
  if (severity < 1)
  {
    debug("%s", message.data());
  }
  else if (severity < 2)
  {
    logwarn("%s", message.data());
  }
  else
  {
    logerr("%s", message.data());
  }
}

void DeviceContext::ExecutionContext::bindlessSetResourceDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  contextState.bindlessSetManager.setResourceDescriptor(device.device.get(), slot, descriptor);
}

void DeviceContext::ExecutionContext::bindlessSetSamplerDescriptor(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  contextState.bindlessSetManager.setSamplerDescriptor(device.device.get(), slot, descriptor);
}

void DeviceContext::ExecutionContext::bindlessCopyResourceDescriptors(uint32_t src, uint32_t dst, uint32_t count)
{
  contextState.bindlessSetManager.copyResourceDescriptors(device.device.get(), src, dst, count);
}

void DeviceContext::ExecutionContext::registerFrameCompleteEvent(os_event_t event) { self.back.frameCompleteEvents.push_back(event); }

void DeviceContext::ExecutionContext::registerFrameEventsCallback(FrameEvents *callback)
{
  callback->beginFrameEvent();
  self.back.frameEventCallbacks.push_back(callback);
}

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
void DeviceContext::ExecutionContext::resetBufferState(BufferResourceReference buffer)
{
  contextState.resourceStates.resetBufferState(contextState.graphicsCommandListBarrierBatch, buffer.buffer, buffer.resourceId);
}
#endif

void DeviceContext::ExecutionContext::onBeginCPUTextureAccess(Image *image)
{
  contextState.resourceStates.beginTextureCPUAccess(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image);
}

void DeviceContext::ExecutionContext::onEndCPUTextureAccess(Image *image)
{
  contextState.resourceStates.endTextureCPUAccess(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image);
}

void DeviceContext::ExecutionContext::addSwapchainView(Image *image, ImageViewInfo view)
{
  self.back.swapchain.registerSwapchainView(device.device.get(), image, view);
}

void DeviceContext::ExecutionContext::mipMapGenSource(Image *image, MipMapIndex mip, ArrayLayerIndex ary)
{
  contextState.resourceStates.useTextureAsMipGenSource(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image, mip, ary);
}

void DeviceContext::ExecutionContext::disablePredication()
{
  if (contextState.graphicsState.activePredicationBuffer.buffer && readyCommandList())
  {
    contextState.graphicsState.activePredicationBuffer = {};
    contextState.cmdBuffer.setPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
  }
}

void DeviceContext::ExecutionContext::tranistionPredicationBuffer()
{
  if (contextState.graphicsState.predicationBuffer.buffer)
  {
    contextState.resourceStates.useBufferAsPredication(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsState.predicationBuffer);
  }
}

void DeviceContext::ExecutionContext::applyPredicationBuffer()
{
  if (contextState.graphicsState.predicationBuffer == contextState.graphicsState.activePredicationBuffer || !readyCommandList())
  {
    return;
  }

  if (contextState.graphicsState.predicationBuffer.buffer)
  {
    contextState.cmdBuffer.setPredication(contextState.graphicsState.predicationBuffer.buffer,
      contextState.graphicsState.predicationBuffer.offset, D3D12_PREDICATION_OP_EQUAL_ZERO);
  }
  else
  {
    contextState.cmdBuffer.setPredication(nullptr, 0, D3D12_PREDICATION_OP_EQUAL_ZERO);
  }
  contextState.graphicsState.activePredicationBuffer = contextState.graphicsState.predicationBuffer;
}

#if _TARGET_PC_WIN
void DeviceContext::ExecutionContext::onDeviceError(HRESULT remove_reason)
{
#if DAGOR_DBGLEVEL > 0
  self.dumpCommandLog();
#endif
  contextState.debugOnDeviceRemoved(device, device.device.get(), remove_reason);

  contextState.activeReadBackCommandList.reset();
  contextState.activeEarlyUploadCommandList.reset();
  contextState.activeLateUploadCommandList.reset();
  contextState.cmdBuffer.releaseBufferForSubmit();

  // increment progress to break up link to previous frames
  ++self.back.frameProgress;
}

void DeviceContext::ExecutionContext::onSwapchainSwapCompleted() { self.back.swapchain.onFrameBegin(device.device.get()); }
#endif

#if _TARGET_PC_WIN
void DeviceContext::ExecutionContext::beginTileMapping(Image *image, ID3D12Heap *heap, size_t heap_base, size_t mapping_count)
{
  device.queues[DeviceQueueType::GRAPHICS].beginTileMapping(image->getHandle(), heap, heap_base, mapping_count);
}

#else

void DeviceContext::ExecutionContext::beginTileMapping(Image *image, uintptr_t address, uint32_t size, size_t mapping_count)
{
  device.queues[DeviceQueueType::GRAPHICS].beginTileMapping(image->getHandle(), image->getAccessComputer(), address, size,
    mapping_count);
}

#endif

void DeviceContext::ExecutionContext::addTileMappings(const TileMapping *mapping, size_t mapping_count)
{
  device.queues[DeviceQueueType::GRAPHICS].addTileMappings(mapping, mapping_count);
}

void DeviceContext::ExecutionContext::endTileMapping() { device.queues[DeviceQueueType::GRAPHICS].endTileMapping(); }

void DeviceContext::ExecutionContext::activateBuffer(BufferResourceReferenceAndAddressRangeWithClearView buffer,
  const ResourceMemory &memory, ResourceActivationAction action, const ResourceClearValue &value, GpuPipeline gpu_pipeline)
{
  if (!readyCommandList())
  {
    return;
  }
  auto &frame = contextState.getFrameData();
  contextState.resourceActivationTracker.activateBuffer(buffer, memory, action, value, contextState.resourceStates,
    contextState.graphicsCommandListBarrierBatch, device.device.get(), frame.resourceViewHeaps, contextState.cmdBuffer);
  G_UNUSED(gpu_pipeline);
  dirtyBufferState(buffer.resourceId);
}

void DeviceContext::ExecutionContext::activateTexture(Image *tex, ResourceActivationAction action, const ResourceClearValue &value,
  ImageViewState view_state, D3D12_CPU_DESCRIPTOR_HANDLE view, GpuPipeline gpu_pipeline)
{
  if (!readyCommandList())
  {
    return;
  }
  auto &frame = contextState.getFrameData();
  contextState.resourceActivationTracker.activateTexture(tex, action, value, view_state, view, contextState.resourceStates,
    contextState.graphicsCommandListBarrierBatch, contextState.graphicsCommandListSplitBarrierTracker, device.device.get(),
    frame.resourceViewHeaps, contextState.cmdBuffer);
  G_UNUSED(gpu_pipeline);
  dirtyTextureState(tex);
}

void DeviceContext::ExecutionContext::deactivateBuffer(BufferResourceReferenceAndAddressRange buffer, const ResourceMemory &memory,
  GpuPipeline gpu_pipeline)
{
  if (!readyCommandList())
  {
    return;
  }
  contextState.resourceActivationTracker.deactivateBuffer(buffer, memory);
  G_UNUSED(gpu_pipeline);
}

void DeviceContext::ExecutionContext::deactivateTexture(Image *tex, GpuPipeline gpu_pipeline)
{
  if (!readyCommandList())
  {
    return;
  }
  contextState.resourceActivationTracker.deactivateTexture(tex);
  G_UNUSED(gpu_pipeline);
}

void DeviceContext::ExecutionContext::aliasFlush(GpuPipeline gpu_pipeline)
{
  contextState.resourceActivationTracker.flushAll(contextState.graphicsCommandListBarrierBatch);
  G_UNUSED(gpu_pipeline);
}

void DeviceContext::ExecutionContext::twoPhaseCopyBuffer(BufferResourceReferenceAndOffset source, uint32_t destination_offset,
  ScratchBuffer scratch_memory, uint32_t data_size)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  // phase one, copy source region to scratch
  contextState.resourceStates.useBufferAsCopySource(contextState.graphicsCommandListBarrierBatch, source);
  contextState.resourceStates.useScratchAsCopyDestination(contextState.graphicsCommandListBarrierBatch, scratch_memory.buffer);
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);
  contextState.cmdBuffer.copyBuffer(scratch_memory.buffer, scratch_memory.offset, source.buffer, source.offset, data_size);

  // phase two, copy scratch to destination region
  contextState.resourceStates.useBufferAsCopyDestination(contextState.graphicsCommandListBarrierBatch, source);
  contextState.resourceStates.useScratchAsCopySource(contextState.graphicsCommandListBarrierBatch, scratch_memory.buffer);
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);
  contextState.cmdBuffer.copyBuffer(source.buffer, destination_offset, scratch_memory.buffer, scratch_memory.offset, data_size);

  contextState.bufferAccessTracker.updateLastFrameAccess(source.resourceId);

  dirtyBufferState(source.resourceId);
}

void DeviceContext::ExecutionContext::transitionBuffer(BufferResourceReference buffer, D3D12_RESOURCE_STATES state)
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.tranitionBufferExplicit(contextState.graphicsCommandListBarrierBatch, buffer, state);
}

void DeviceContext::ExecutionContext::resizeImageMipMapTransfer(Image *src, Image *dst, MipMapRange mip_map_range,
  uint32_t src_mip_map_offset, uint32_t dst_mip_map_offset)
{
  const auto arrayLayers = src->getArrayLayers();
  const auto formatPlanes = src->getPlaneCount();
  const auto format = src->getFormat();
  Extent3D blockExtent{1, 1, 1};
  format.getBytesPerPixelBlock(&blockExtent.width, &blockExtent.height);

  D3D12_TEXTURE_COPY_LOCATION dstLocation;
  dstLocation.pResource = dst->getHandle();
  dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

  D3D12_TEXTURE_COPY_LOCATION srcLocation;
  srcLocation.pResource = src->getHandle();
  srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

  D3D12_BOX copyBox;
  copyBox.left = 0;
  copyBox.top = 0;
  copyBox.front = 0;

  bool doBeforeFrame = true;
  // If it has a id and it was accessed during this frame already, we assume it was changed in this
  // frame, so we do the replacement on the graphics queue to ensure nothing may be lost.
  // This can be optimized if we track if modifications where actually made or not.
  // TODO: wasAccessedThisFrame should be replaced with something like wasAlteredThisFrame(...)
  if (src->getGlobalSubResourceIdBase().isValid() && src->wasAccessedThisFrame(self.back.frameProgress))
  {
    doBeforeFrame = false;
  }

  for (auto level : mip_map_range)
  {
    const auto dstLevel = level - dst_mip_map_offset;
    const auto srcLevel = level - src_mip_map_offset;

    const auto dstExtent = align_value(dst->getMipExtents(dstLevel), blockExtent);
    const auto srcExtent = align_value(src->getMipExtents(srcLevel), blockExtent);

    // If sizes do not match, we try to safe what we can and only copy a region that does not cause
    // a out of bounds read error and device reset.
    const auto copyExtent = min(dstExtent, srcExtent);
    // Report an error, but keep going.
    if (dstExtent != srcExtent)
    {
      src->getDebugName([src, srcLevel, dst, dstLevel, srcExtent, dstExtent](auto &name) {
        logerr("DX12: resizeImageMipMapTransfer of <%s> from %p[%u] to %p[%u] mip-map level sizes "
               "do not match %ux%ux%u vs %ux%ux%u, using min",
          name, src, srcLevel.index(), dst, dstLevel.index(), srcExtent.width, srcExtent.height, srcExtent.depth, dstExtent.width,
          dstExtent.height, dstExtent.depth);
      });
    }

    copyBox.right = copyExtent.width;
    copyBox.bottom = copyExtent.height;
    copyBox.back = copyExtent.depth;

    for (auto layer : arrayLayers)
    {
      for (auto plane : formatPlanes)
      {
        const auto dstSubRes = dst->mipAndLayerResourceIndex(dstLevel, layer, plane);
        const auto srcSubRes = src->mipAndLayerResourceIndex(srcLevel, layer, plane);

        dstLocation.SubresourceIndex = dstSubRes.index();
        srcLocation.SubresourceIndex = srcSubRes.index();

        // Should be the most common path, resizing "static" textures, this can be safely done on
        // the copy queue before the frame starts.
        // With different rules when to switch over to the resized texture, we could move this
        // over to a command list that runs on the copy queue in parallel to the frame.
        if (doBeforeFrame)
        {
          bool sourceBarrierWasPlaced = contextState.resourceStates.beginUseTextureAsMipTransferSourceOnCopyQueue(
            contextState.uploadBarrierBatch, contextState.initialResourceStateSet, src, srcSubRes);
          bool destinationBarrierWasPlaced = contextState.resourceStates.beginUseTextureAsMipTransferDestinationOnCopyQueue(
            contextState.uploadBarrierBatch, contextState.initialResourceStateSet, dst, dstSubRes);

          bool sourceWasUsedInPreviousFrames = src->wasAccessedPreviousFrames(self.back.frameProgress);

          CopyCommandList<VersionedPtr<D3DCopyCommandList>> cmdList;
          if (sourceBarrierWasPlaced || destinationBarrierWasPlaced || sourceWasUsedInPreviousFrames)
          {
            readyLateUploadCommandList();
            cmdList = contextState.activeLateUploadCommandList;
          }
          else
          {
            readyEarlyUploadCommandList();
            cmdList = contextState.activeEarlyUploadCommandList;
          }

          if (!cmdList)
          {
            break;
          }

          cmdList.copyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, &copyBox);

          contextState.resourceStates.finishUseTextureAsMipTransferSourceOnCopyQueue(contextState.postUploadBarrierBatch, src,
            srcSubRes);

          contextState.resourceStates.finishUseTextureAsMipTransferDestinationOnCopyQueue(contextState.postUploadBarrierBatch, dst,
            dstSubRes);
        }
        else
        {
          if (!readyCommandList())
          {
            break;
          }

          contextState.resourceStates.useTextureAsCopySource(contextState.graphicsCommandListBarrierBatch,
            contextState.graphicsCommandListSplitBarrierTracker, src, dstSubRes);

          contextState.resourceStates.useTextureAsCopyDestination(contextState.graphicsCommandListBarrierBatch,
            contextState.graphicsCommandListSplitBarrierTracker, dst, dstSubRes);

          contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

          contextState.cmdBuffer.copyTexture(&dstLocation, 0, 0, 0, &srcLocation, &copyBox);

          contextState.resourceStates.finishUseTextureAsCopyDestination(contextState.graphicsCommandListBarrierBatch,
            contextState.graphicsCommandListSplitBarrierTracker, dst, dstSubRes);
        }
      }
    }
  }
}

void DeviceContext::ExecutionContext::debugBreak() { contextState.breakNow(); }

void DeviceContext::ExecutionContext::addDebugBreakString(StringIndexRef::RangeType str)
{
  contextState.addBreakPointString({str.data(), str.size()});
}

void DeviceContext::ExecutionContext::removeDebugBreakString(StringIndexRef::RangeType str)
{
  contextState.removeBreakPointString({str.data(), str.size()});
}

#if !_TARGET_XBOXONE
void DeviceContext::ExecutionContext::dispatchMesh(uint32_t x, uint32_t y, uint32_t z)
{
  if (!contextState.graphicsState.basePipeline || !readyCommandList())
  {
    return;
  }

  if (checkDrawCallHasOutput("Skipped: dispatchMesh"))
  {
    tranistionPredicationBuffer();

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();
    contextState.cmdBuffer.dispatchMesh(x, y, z);

    contextState.debugDispatchMesh(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_VS],
      contextState.stageState[STAGE_PS], *contextState.graphicsState.basePipeline, *contextState.graphicsState.pipeline, x, y, z);
  }
}

void DeviceContext::ExecutionContext::dispatchMeshIndirect(BufferResourceReferenceAndOffset args, uint32_t stride,
  BufferResourceReferenceAndOffset count, uint32_t max_count)
{
  if (!contextState.graphicsState.basePipeline || !readyCommandList())
  {
    return;
  }

  if (checkDrawCallHasOutput("Skipped: dispatchMeshIndirect"))
  {
    auto &signature = contextState.graphicsState.basePipeline->getSignature();
    auto indirectSignature = contextState.drawIndirectSignatures.getSignatureForStride(device.device.get(), stride,
      D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, signature);
    if (!indirectSignature)
    {
      // error while trying to create a signature
      return;
    }

    contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, args);
    contextState.bufferAccessTracker.updateLastFrameAccess(args.resourceId);

    if (count)
    {
      contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, count);
      contextState.bufferAccessTracker.updateLastFrameAccess(count.resourceId);
    }

    tranistionPredicationBuffer();

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    applyPredicationBuffer();
    contextState.cmdBuffer.dispatchMeshIndirect(indirectSignature, args.buffer, args.offset, count.buffer, count.offset, max_count);

    contextState.debugDispatchMeshIndirect(device, contextState.cmdBuffer.getHandle(), contextState.stageState[STAGE_VS],
      contextState.stageState[STAGE_PS], *contextState.graphicsState.basePipeline, *contextState.graphicsState.pipeline, args, count,
      max_count);
  }
}
#endif

void DeviceContext::ExecutionContext::addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader)
{
  device.pipeMan.addShaderGroup(device.getDevice(), &device.pipelineCache, &contextState.framebufferLayouts, group, dump,
    null_pixel_shader);
}

void DeviceContext::ExecutionContext::removeShaderGroup(uint32_t group)
{
  device.pipeMan.removeShaderGroup(
    group, [this](ProgramID program) { contextState.getFrameData().deletedPrograms.push_back(program); },
    [this](GraphicsProgramID program) { contextState.getFrameData().deletedGraphicPrograms.push_back(program); });
}

void DeviceContext::ExecutionContext::loadComputeShaderFromDump(ProgramID program)
{
  device.pipeMan.loadComputeShaderFromDump(device.device.get(), device.pipelineCache, program,
    get_recover_behvior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
      device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
    device.shouldNameObjects());
}

void DeviceContext::ExecutionContext::compilePipelineSet(DynamicArray<InputLayoutID> &&input_layouts,
  DynamicArray<StaticRenderStateID> &&static_render_states, DynamicArray<FramebufferLayout> &&framebuffer_layouts,
  DynamicArray<GraphicsPipelinePreloadInfo> &&graphics_pipelines, DynamicArray<MeshPipelinePreloadInfo> &&mesh_pipelines,
  DynamicArray<ComputePipelinePreloadInfo> &&compute_pipelines)
{
  device.pipeMan.compilePipelineSet(device.getDevice(), device.pipelineCache, contextState.framebufferLayouts,
    eastl::forward<DynamicArray<InputLayoutID>>(input_layouts),
    eastl::forward<DynamicArray<StaticRenderStateID>>(static_render_states),
    eastl::forward<DynamicArray<FramebufferLayout>>(framebuffer_layouts),
    eastl::forward<DynamicArray<GraphicsPipelinePreloadInfo>>(graphics_pipelines),
    eastl::forward<DynamicArray<MeshPipelinePreloadInfo>>(mesh_pipelines),
    eastl::forward<DynamicArray<ComputePipelinePreloadInfo>>(compute_pipelines));
}
