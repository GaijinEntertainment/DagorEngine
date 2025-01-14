// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_context.h"
#include "frontend_state.h"
#include "render_target_mask_util.h"

#include <3d/dag_gpuConfig.h>
#include <3d/dag_lowLatency.h>
#include <amdFsr.h>
#include <debug/dag_logSys.h>
#include <drv_returnAddrStore.h>
#include <EASTL/sort.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_files.h>
#include <perfMon/dag_statDrv.h>
#include <stereoHelper.h>
#include <util/dag_watchdog.h>

#if _TARGET_XBOX
#include <osApiWrappers/gdk/app.h> // make_thread_time_sensitive
#else
#include <d3d12video.h>
#endif


#define DX12_LOCK_FRONT() WinAutoLock lock(getFrontGuard())

using namespace drv3d_dx12;

#if _TARGET_64BIT
#define PTR_LIKE_HEX_FMT "%016X"
#else
#define PTR_LIKE_HEX_FMT "%08X"
#endif

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
#define DX12_BEGIN_CONTEXT_COMMAND(IsPrimary, Name)    \
  template <typename T, typename D>                    \
  void execute(const Cmd##Name &cmd, D &debug, T &ctx) \
  {                                                    \
    G_UNUSED(cmd);                                     \
    G_UNUSED(ctx);                                     \
    debug.setCommandData(cmd, "Cmd" #Name);

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(IsPrimary, Name, Param0Type, Param0Name)     \
  template <typename T, typename D>                                                   \
  void execute(const ExtendedVariant<Cmd##Name, Param0Type> &param, D &debug, T &ctx) \
  {                                                                                   \
    auto &cmd = param.cmd;                                                            \
    auto &Param0Name = param.p0;                                                      \
    G_UNUSED(Param0Name);                                                             \
    G_UNUSED(cmd);                                                                    \
    G_UNUSED(ctx);                                                                    \
    debug.setCommandData(cmd, "Cmd" #Name);

#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(IsPrimary, Name, Param0Type, Param0Name, Param1Type, Param1Name) \
  template <typename T, typename D>                                                                       \
  void execute(const ExtendedVariant2<Cmd##Name, Param0Type, Param1Type> &param, D &debug, T &ctx)        \
  {                                                                                                       \
    auto &cmd = param.cmd;                                                                                \
    auto &Param0Name = param.p0;                                                                          \
    auto &Param1Name = param.p1;                                                                          \
    G_UNUSED(Param0Name);                                                                                 \
    G_UNUSED(cmd);                                                                                        \
    G_UNUSED(ctx);                                                                                        \
    debug.setCommandData(cmd, "Cmd" #Name);

#define DX12_END_CONTEXT_COMMAND }
// make an alias so we do not need to write cmd.
#define DX12_CONTEXT_COMMAND_PARAM(type, name)  \
  decltype(auto) name = resolve(ctx, cmd.name); \
  G_UNUSED(name);
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size) \
  decltype(auto) name = resolve(ctx, cmd.name);            \
  G_UNUSED(name);
#include "device_context_cmd.inc.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
#undef DX12_CONTEXT_COMMAND_IMPLEMENTATION
#undef HANDLE_RETURN_ADDRESS
} // namespace

BufferImageCopy drv3d_dx12::calculate_texture_subresource_copy_info(const Image &texture, uint32_t subresource_index, uint64_t offset)
{
  auto subResInfo = calculate_texture_subresource_info(texture, SubresourceIndex::make(subresource_index));
  return {{offset, subResInfo.footprint}, subresource_index, {0, 0, 0}};
}

TextureMipsCopyInfo drv3d_dx12::calculate_texture_mips_copy_info(const Image &texture, uint32_t mip_levels, uint32_t array_slice,
  uint32_t array_size, uint64_t initial_offset)
{
  G_ASSERT(mip_levels <= MAX_MIPMAPS);
  TextureMipsCopyInfo copies{mip_levels};
  uint64_t offset = initial_offset;
  for (uint32_t j = 0; j < mip_levels; ++j)
  {
    BufferImageCopy &copy = copies[j];
    auto subResInfo = calculate_texture_mip_info(texture, MipMapIndex::make(j));
    copy = {{offset, subResInfo.footprint}, calculate_subresource_index(j, array_slice, 0, mip_levels, array_size), {0, 0, 0}};
    G_ASSERT(copy.layout.Offset % D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT == 0);
    offset += subResInfo.rowCount * subResInfo.footprint.RowPitch * subResInfo.footprint.Depth;
    offset = (offset + D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1);
  }
  return copies;
}

void FrameInfo::init(ID3D12Device *device)
{
  genericCommands.init(device);
  computeCommands.init(device);
  earlyUploadCommands.init(device);
  lateUploadCommands.init(device);
  readBackCommands.init(device);

  resourceViewHeaps = ShaderResourceViewDescriptorHeapManager //
    {device, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)};
  samplerHeaps = SamplerDescriptorHeapManager //
    {device, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)};

  progressEvent.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
}
void FrameInfo::shutdown(DeviceQueueGroup &queue_group, PipelineManager &pipe_man)
{
  // have to check if the queue group was initialized properly to wait on frame progress
  // also if progress is 0, this frame info was never used
  if (queue_group.canWaitOnFrameProgress() && progress > 0)
  {
    wait_for_frame_progress_with_event(queue_group, progress, progressEvent.get(), "FrameInfo::shutdown");
  }

  genericCommands.shutdown();
  computeCommands.shutdown();
  earlyUploadCommands.shutdown();
  lateUploadCommands.shutdown();
  readBackCommands.shutdown();

  for (auto &&prog : deletedPrograms)
    pipe_man.removeProgram(prog);
  deletedPrograms.clear();

  for (auto &&prog : deletedGraphicPrograms)
    pipe_man.removeProgram(prog);
  deletedGraphicPrograms.clear();

  deletedShaderModules.clear();

  resourceViewHeaps = ShaderResourceViewDescriptorHeapManager{};
  samplerHeaps = SamplerDescriptorHeapManager{};

  backendQueryManager.shutdown();
}

int64_t FrameInfo::beginFrame(DeviceQueueGroup &queue_group, PipelineManager &pipe_man, uint32_t frame_idx)
{
  auto waitTicks = ref_time_ticks();
  wait_for_frame_progress_with_event(queue_group, progress, progressEvent.get(), "FrameInfo::beginFrame");
  waitTicks = ref_time_ticks() - waitTicks;

  frameIndex = frame_idx;

  backendQueryManager.flush();

  genericCommands.frameReset();
  computeCommands.frameReset();
  earlyUploadCommands.frameReset();
  lateUploadCommands.frameReset();
  readBackCommands.frameReset();

  for (auto &&prog : deletedPrograms)
    pipe_man.removeProgram(prog);
  deletedPrograms.clear();

  for (auto &&prog : deletedGraphicPrograms)
    pipe_man.removeProgram(prog);
  deletedGraphicPrograms.clear();

  deletedShaderModules.clear();

  // usual ranges are from sub 100 to about 2k on resources and sub 100 to about 300 for samplers
#if DX12_REPORT_DESCRIPTOR_USES
  logdbg("DX12: Frame %u used %u resource descriptors", progress, resourceViewHeaps.getTotalUsedDescriptors());
  logdbg("DX12: Frame %u used %u sampler descriptors", progress, samplerHeaps.getTotalUsedDescriptors());
#endif

  resourceViewHeaps.clearScratchSegments();
  samplerHeaps.clearScratchSegment();

  return waitTicks;
}

void FrameInfo::preRecovery(DeviceQueueGroup &queue_group, PipelineManager &pipe_man)
{
  wait_for_frame_progress_with_event(queue_group, progress, progressEvent.get(), "FrameInfo::preRecovery");

  genericCommands.shutdown();
  computeCommands.shutdown();
  earlyUploadCommands.shutdown();
  lateUploadCommands.shutdown();
  readBackCommands.shutdown();

  for (auto &&prog : deletedPrograms)
    pipe_man.removeProgram(prog);
  deletedPrograms.clear();

  for (auto &&prog : deletedGraphicPrograms)
    pipe_man.removeProgram(prog);
  deletedGraphicPrograms.clear();

  // handle?
  // dag::Vector<ProgramID> deletedPrograms;
  deletedShaderModules.clear();
  resourceViewHeaps = ShaderResourceViewDescriptorHeapManager{};
  samplerHeaps = SamplerDescriptorHeapManager{};
  backendQueryManager.shutdown();
  progress = 0;
}

void FrameInfo::recover(ID3D12Device *device)
{
#if _TARGET_PC_WIN
  genericCommands.init(device);
  computeCommands.init(device);
  earlyUploadCommands.init(device);
  lateUploadCommands.init(device);
  readBackCommands.init(device);

  resourceViewHeaps = ShaderResourceViewDescriptorHeapManager //
    {device, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)};
  samplerHeaps = SamplerDescriptorHeapManager //
    {device, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)};
#else
  G_UNUSED(device);
#endif
}

ID3D12CommandSignature *SignatureStore::getSignatureForStride(ID3D12Device *device, uint32_t stride, D3D12_INDIRECT_ARGUMENT_TYPE type)
{
  auto ref = eastl::find_if(begin(signatures), end(signatures),
    [=](const SignatureInfo &si) { return si.type == type && si.stride == stride; });
  if (end(signatures) == ref)
  {
    D3D12_INDIRECT_ARGUMENT_DESC arg;
    D3D12_COMMAND_SIGNATURE_DESC desc;
    desc.NumArgumentDescs = 1;
    desc.pArgumentDescs = &arg;
    desc.NodeMask = 0;

    arg.Type = type;
    desc.ByteStride = stride;

    SignatureInfo info;
    info.stride = stride;
    info.type = type;
    if (!DX12_CHECK_OK(device->CreateCommandSignature(&desc, nullptr, COM_ARGS(&info.signature))))
    {
      return nullptr;
    }
    ref = signatures.insert(ref, eastl::move(info));
  }
  return ref->signature.Get();
}

ID3D12CommandSignature *SignatureStore::getSignatureForStride(ID3D12Device *device, uint32_t stride, D3D12_INDIRECT_ARGUMENT_TYPE type,
  GraphicsPipelineSignature &signature)
{
  uint8_t mask = signature.def.vertexShaderRegisters.specialConstantsMask | signature.def.pixelShaderRegisters.specialConstantsMask |
                 signature.def.geometryShaderRegisters.specialConstantsMask | signature.def.hullShaderRegisters.specialConstantsMask |
                 signature.def.domainShaderRegisters.specialConstantsMask;
  bool has_draw_id = mask & dxil::SpecialConstantType::SC_DRAW_ID;
  if (!has_draw_id)
    return getSignatureForStride(device, stride, type);

  ID3D12RootSignature *root_signature = signature.signature.Get();
  auto ref = eastl::find_if(begin(signaturesEx), end(signaturesEx),
    [=](const SignatureInfoEx &si) { return si.type == type && si.stride == stride && si.rootSignature == root_signature; });
  if (end(signaturesEx) == ref)
  {
    D3D12_INDIRECT_ARGUMENT_DESC args[2] = {};
    D3D12_COMMAND_SIGNATURE_DESC desc;
    desc.NumArgumentDescs = 2;
    desc.pArgumentDescs = args;
    desc.NodeMask = 0;

    args[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
    args[0].Constant.Num32BitValuesToSet = 1;
    args[0].Constant.RootParameterIndex = 0;
    args[0].Constant.DestOffsetIn32BitValues = 0;
    args[1].Type = type;
    desc.ByteStride = stride;

    SignatureInfoEx info;
    info.stride = stride;
    info.type = type;
    info.rootSignature = root_signature;
    if (!DX12_CHECK_OK(device->CreateCommandSignature(&desc, root_signature, COM_ARGS(&info.signature))))
    {
      return nullptr;
    }
    ref = signaturesEx.insert(ref, eastl::move(info));
  }
  return ref->signature.Get();
}


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

void DeviceContext::frontFlush(TidyFrameMode tidy_mode)
{
  front.recordingLatchedFrame->progress = front.recordingWorkItemProgress;
  front.recordingWorkItemProgress = front.nextWorkItemProgress++;
  manageLatchedState(tidy_mode);
}

DAGOR_NOINLINE
void DeviceContext::waitForLatchedFrame()
{
  auto nextWait = eastl::min_element(front.latchedFrameSet.cbegin(), front.latchedFrameSet.cend(),
    [](auto &a, auto &b) { return a.progress < b.progress; });

  wait_for_frame_progress_with_event(device.queues, nextWait->progress, *eventsPool.acquireEvent(), "waitForLatchedFrame");
}

void DeviceContext::manageLatchedState(TidyFrameMode tidy_mode)
{
  for (FrontendFrameLatchedData *nextFrame = nullptr;;)
  {
    uint64_t gpuProgress = device.queues.checkFrameProgress() + 1;
    for (auto &&latchedFrame : front.latchedFrameSet)
    {
      if (latchedFrame.progress < gpuProgress)
      {
        if (latchedFrame.progress)
        {
          tidyFrame(latchedFrame, tidy_mode);
          gpuProgress = device.queues.checkFrameProgress() + 1;
        }
        nextFrame = &latchedFrame;
      }
    }

    if (DAGOR_UNLIKELY(gpuProgress == 0))
    {
      return;
    }

    if (nextFrame)
    {
      front.recordingLatchedFrame = nextFrame;
      break;
    }

    waitForLatchedFrame();
  }

  ResourceMemoryHeap::BeginFrameRecordingInfo frameRecodingInfo;
  frameRecodingInfo.historyIndex = eastl::distance(front.latchedFrameSet.begin(), front.recordingLatchedFrame);
  device.resources.beginFrameRecording(frameRecodingInfo);

#if DX12_RECORD_TIMING_DATA
  front.recordingLatchedFrame->frameIndex = front.frameIndex;

#if DX12_REPORT_LONG_FRAMES
#if _TARGET_XBOX
  // There is no need to report long frames when game is in constrained state
  if (::dgs_app_active)
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
      logdbg("DX12: Frame interval was %uus", frameTimeUsec);
      logdbg("DX12: Frame latency was %uus", frameLatency);
      logdbg("DX12: Frame front to back wait was %uus", frameFrontBackWait);
      logdbg("DX12: Frame back to front wait was %uus", frameBackFrontWait);
      logdbg("DX12: Frame GPU wait was %uus", frameGPUWait);
      logdbg("DX12: Frame Swapchain swap was %uus", swapSwapWait);
      logdbg("DX12: Frame Get Swapchain buffer was %uus", getSwapWait);
      logdbg("DX12: Frame Wait For Swapchain waitable object was %uus", frontSwapchainWait);
    }

#if _TARGET_XBOX
  }
#endif
#endif

#endif
}

void DeviceContext::makeReadyForFrame(uint32_t frame_index, [[maybe_unused]] bool update_swapchain)
{
#if DX12_RECORD_TIMING_DATA
  auto now = ref_time_ticks();
#endif
  back.sharedContextState.frames.advance();
#if _TARGET_PC_WIN
  // On PC, if we execute stuff on the worker thread, we need to wait for the main thread to send
  // the message to acquire the swap image, otherwise we probably get the same swap index as before
  // and use the swap image of the last frame
  const bool shouldAcquireSwapImageNow =
    device.getContext().isPresentAsync() || (!device.getContext().hasWorkerThread() && update_swapchain);
#else
  constexpr bool shouldAcquireSwapImageNow = true;
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
    frame.beginFrame(device.queues, device.pipeMan, frame_index);

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
  debug::call_stack::Generator::configure(dxCfg);

  commandStream.resize((1024 * 1024) / 4);

#if _TARGET_PC_WIN
  isPresentSynchronous = !dxCfg->getBool("asyncPresent", true);
  isWaitForASyncPresent = dxCfg->getBool("waitForAsyncPresent", false);
#endif

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
    D3D_ERROR("DX12: The immediate flush doesn't work in non immedite execution mode!");
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
  commandStream.pushBack(cmd, str.data(), str.size());
  immediateModeExecute();
}

void DeviceContext::removeDebugBreakString(eastl::string_view str)
{
  auto cmd = make_command<CmdRemoveDebugBreakString>();
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd, str.data(), str.size());
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

void DeviceContext::addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader,
  eastl::string_view name)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdAddShaderGroup, const char>(group, dump, null_pixel_shader), name.data(), name.size());
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

void DeviceContext::compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
  DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, const DataBlock *output_formats_set,
  const DataBlock *graphics_pipeline_set, const DataBlock *mesh_pipeline_set, const DataBlock *compute_pipeline_set,
  const char *default_format)
{
  DynamicArray<FramebufferLayoutWithHash> framebufferLayouts;
  if (output_formats_set && output_formats_set->blockCount() > 0)
  {
    auto blockCount = output_formats_set->blockCount();
    framebufferLayouts.resize(blockCount);
    pipeline::DataBlockDecodeEnumarator<pipeline::FramebufferLayoutDecoder> decoder{*output_formats_set, 0, default_format};
    for (; !decoder.completed(); decoder.next())
    {
      auto &target = framebufferLayouts[decoder.index()];
      target = {};
      decoder.decode(target.layout, target.hash);
    }
  }

  DynamicArray<GraphicsPipelinePreloadInfo> graphicsPiplines;
  if (graphics_pipeline_set && graphics_pipeline_set->blockCount() > 0)
  {
    auto blockCount = graphics_pipeline_set->blockCount();
    graphicsPiplines.resize(blockCount);
    uint32_t decodedCount = 0;
    pipeline::DataBlockDecodeEnumarator<pipeline::GraphicsPipelineDecoder> decoder{*graphics_pipeline_set, 0};
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
    pipeline::DataBlockDecodeEnumarator<pipeline::MeshPipelineDecoder> decoder{*mesh_pipeline_set, 0};
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
    pipeline::DataBlockDecodeEnumarator<pipeline::ComputePipelineDecoder> decoder{*compute_pipeline_set, 0};
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

namespace
{
struct GraphicsVariantDecoderContext
{
  const DynamicArray<InputLayoutIDWithHash> &inputLayouts;
  const DynamicArray<StaticRenderStateIDWithHash> &staticRenderStates;
  const DynamicArray<FramebufferLayoutWithHash> &framebufferLayouts;
  DynamicArray<cacheBlk::GraphicsVariantGroup> &output;
  uint32_t variantCount = 0;
  uint32_t classCount = 0;

  GraphicsVariantDecoderContext(DynamicArray<cacheBlk::GraphicsVariantGroup> &o, uint32_t max_variant_count,
    const DynamicArray<InputLayoutIDWithHash> &il, const DynamicArray<StaticRenderStateIDWithHash> &srs,
    const DynamicArray<FramebufferLayoutWithHash> &fbl) :
    output{o}, inputLayouts{il}, staticRenderStates{srs}, framebufferLayouts{fbl}
  {
    output.resize(max_variant_count);
  }

  ~GraphicsVariantDecoderContext() { output.resize(variantCount); }

  int translateInputLayoutHashString(const char *hash_string) const
  {
    auto hashValue = dxil::HashValue::fromString(hash_string);
    auto ref =
      eastl::find_if(eastl::begin(inputLayouts), eastl::end(inputLayouts), [hashValue](auto &e) { return hashValue == e.hash; });
    if (ref == eastl::end(inputLayouts))
    {
      logwarn("DX12: ...was looking for input layout with hash %s, but none was found, listing input layout hashes...", hash_string);
      for (auto &layout : inputLayouts)
      {
        char buf[sizeof(layout.hash) * 2 + 1];
        layout.hash.convertToString(buf, sizeof(buf));
        logwarn("DX12: ...%s...", buf);
      }
      return -1;
    }
    return eastl::distance(eastl::begin(inputLayouts), ref);
  }

  int translateStaticRenderStateHashString(const char *hash_string) const
  {
    auto hashValue = dxil::HashValue::fromString(hash_string);
    auto ref = eastl::find_if(eastl::begin(staticRenderStates), eastl::end(staticRenderStates),
      [hashValue](auto &e) { return hashValue == e.hash; });
    if (ref == eastl::end(staticRenderStates))
    {
      logwarn("DX12: ...was looking for render state with hash %s, but none was found, listing render state hashes...", hash_string);
      for (auto &state : staticRenderStates)
      {
        char buf[sizeof(state.hash) * 2 + 1];
        state.hash.convertToString(buf, sizeof(buf));
        logwarn("DX12: ...%s...", buf);
      }
      return -1;
    }
    return eastl::distance(eastl::begin(staticRenderStates), ref);
  }

  int translateFrambufferLayoutHashString(const char *hash_string) const
  {
    auto hashValue = dxil::HashValue::fromString(hash_string);
    auto ref = eastl::find_if(eastl::begin(framebufferLayouts), eastl::end(framebufferLayouts),
      [hashValue](auto &e) { return hashValue == e.hash; });
    if (ref == eastl::end(framebufferLayouts))
    {
      logwarn("DX12: ...was looking for frame buffer layout with hash %s, but none was found, listing frame buffer layout hashes...",
        hash_string);
      for (auto &layout : framebufferLayouts)
      {
        char buf[sizeof(layout.hash) * 2 + 1];
        layout.hash.convertToString(buf, sizeof(buf));
        logwarn("DX12: ...%s...", buf);
      }
      return -1;
    }
    return eastl::distance(eastl::begin(framebufferLayouts), ref);
  }

  void onVariantBegin(int input_layout_id, int static_render_state_id, int frame_buffer_layout_id, int topology, bool is_wire_frame,
    cacheBlk::SignatureMask signature_mask)
  {
    auto &target = output[variantCount];
    target.validSiagnatures = signature_mask;
    target.isWireFrame = is_wire_frame;
    target.topology = topology;
    target.inputLayout = input_layout_id;
    target.staticRenderState = static_render_state_id;
    target.frameBufferLayout = frame_buffer_layout_id;
    classCount = 0;
  }

  void onVariantEnd()
  {
    auto &target = output[variantCount];
    if (classCount)
    {
      target.classes.resize(classCount);
      ++variantCount;
    }
  }

  void onMaxClasses(uint32_t count)
  {
    auto &target = output[variantCount];
    target.classes.resize(count);
  }

  void onClass(eastl::string &&name, DynamicArray<cacheBlk::UseCodes> &&codes)
  {
    auto &target = output[variantCount].classes[classCount++];
    target.name = eastl::move(name);
    target.codes = eastl::move(codes);
  }
};
}

void DeviceContext::compilePipelineSet2(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
  DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, const DataBlock *output_formats_set,
  const DataBlock *compute_pipeline_set, const DataBlock *full_graphics_set, const DataBlock *null_override_graphics_set,
  const DataBlock *signature, const char *default_format, ShaderID null_pixel_shader)
{
  DynamicArray<FramebufferLayoutWithHash> framebufferLayouts;
  if (output_formats_set && output_formats_set->blockCount() > 0)
  {
    auto blockCount = output_formats_set->blockCount();
    framebufferLayouts.resize(blockCount);
    logdbg("DX12: ...loading frame buffer layout from BLK, has %u entries...", blockCount);
    pipeline::DataBlockDecodeEnumarator<pipeline::FramebufferLayoutDecoder> decoder{*output_formats_set, 0, default_format};
    for (; !decoder.completed(); decoder.next())
    {
      auto &target = framebufferLayouts[decoder.index()];
      target = {};
      if (decoder.decode(target.layout, target.hash))
      {
        char buf[sizeof(target.hash) * 2 + 1];
        target.hash.convertToString(buf, sizeof(buf));
        logdbg("DX12: ...loaded frame buffer layout %s...", buf);
      }
      else
      {
        logwarn("DX12: ...error during frame buffer layout load from BLK...");
      }
    }
  }

  DynamicArray<cacheBlk::SignatureEntry> signatureData;
  if (signature && signature->blockCount() > 0)
  {
    signatureData.resize(signature->blockCount());
    for (uint32_t si = 0; si < signature->blockCount(); ++si)
    {
      auto signatureBlock = signature->getBlock(si);
      auto &signatureTarget = signatureData[si];
      signatureTarget.name = signatureBlock->getStr("name", "");

      if (auto classBlock = signatureBlock->getBlockByName("classes"))
      {
        pipeline::DataBlockDecodeEnumarator<pipeline::SignatueHashDecoder> decoder(*classBlock, 0);
        signatureTarget.hashes.resize(classBlock->blockCount());
        for (; !decoder.completed(); decoder.next())
        {
          decoder.decode(signatureTarget.hashes[decoder.index()]);
        }
      }
      else if (auto hashBlock = signatureBlock->getBlockByName("hashes"))
      {
        pipeline::DataBlockParameterDecodeEnumarator<pipeline::SignatueHashDecoder> decoder{*hashBlock, 0};
        signatureTarget.hashes.resize(hashBlock->paramCount());
        for (; !decoder.completed(); decoder.next())
        {
          decoder.decode(signatureTarget.hashes[decoder.index()]);
        }
      }
    }
  }

  DynamicArray<cacheBlk::ComputeClassUse> computeClasses;
  if (compute_pipeline_set)
  {
    // figure out how much space we need
    size_t maxComputeClassCount = 0;
    for (uint32_t v = 0; v < compute_pipeline_set->blockCount(); ++v)
    {
      maxComputeClassCount += compute_pipeline_set->getBlock(v) ? compute_pipeline_set->getBlock(v)->blockCount() : 0;
    }
    computeClasses.resize(maxComputeClassCount);
    uint32_t computeClassCount = 0;
    pipeline::DataBlockDecodeEnumarator<pipeline::ComputeVariantDecoder> decoder{*compute_pipeline_set, 0};
    for (; !decoder.completed(); decoder.next())
    {
      decoder.invoke([&computeClasses, &computeClassCount](auto &&name, auto &&singature_mask, auto &&codes) {
        auto &target = computeClasses[computeClassCount++];
        target.name = eastl::move(name);
        target.validSiagnatures = singature_mask;
        target.codes = eastl::move(codes);
      });
    }
    computeClasses.resize(computeClassCount);
  }

  DynamicArray<cacheBlk::GraphicsVariantGroup> fullGraphicsClasses;
  if (full_graphics_set && full_graphics_set->blockCount() > 0)
  {
    pipeline::DataBlockDecodeEnumarator<pipeline::GraphicsVariantDecoder> decoder{*full_graphics_set, 0};

    GraphicsVariantDecoderContext ctx{
      fullGraphicsClasses, decoder.remaining(), input_layouts, static_render_states, framebufferLayouts};
    for (; !decoder.completed(); decoder.next())
    {
      decoder.invoke(ctx);
    }
  }

  DynamicArray<cacheBlk::GraphicsVariantGroup> nullOverrideGraphicsClasses;
  if (null_override_graphics_set && null_override_graphics_set->blockCount() > 0)
  {
    pipeline::DataBlockDecodeEnumarator<pipeline::GraphicsVariantDecoder> decoder{*null_override_graphics_set, 0};

    GraphicsVariantDecoderContext ctx{
      nullOverrideGraphicsClasses, decoder.remaining(), input_layouts, static_render_states, framebufferLayouts};
    for (; !decoder.completed(); decoder.next())
    {
      decoder.invoke(ctx);
    }
  }

  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdCompilePipelineSet2>(input_layouts.releaseAsSpan(), static_render_states.releaseAsSpan(),
    framebufferLayouts.releaseAsSpan(), signatureData.releaseAsSpan(), computeClasses.releaseAsSpan(),
    fullGraphicsClasses.releaseAsSpan(), nullOverrideGraphicsClasses.releaseAsSpan(), null_pixel_shader));
  immediateModeExecute();
}

void DeviceContext::shutdownWorkerThread()
{
#if !FIXED_EXECUTION_MODE
  if (ExecutionMode::CONCURRENT == executionMode)
#endif
  {
    DX12_LOCK_FRONT();
    if (worker)
    {
      commandStream.pushBack(make_command<CmdTerminate>());
      waitInternal();
      // tell the worker to terminate
      // it will execute all pending commands
      // and then clean up and shutdown
      worker->terminate(true);
      worker.reset();
    }
    // switch to immediate mode if something else should come up
#if !FIXED_EXECUTION_MODE
    executionMode = ExecutionMode::IMMEDIATE;
#endif
  }
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::dispatchRays(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchParameters &rdv)
{
  auto pipe = RayTracePipelineWrapper{pipeline}.get();

  RayDispatchBasicParameters basicParams;
  basicParams.rootSignature = &pipe->getSignature();
  basicParams.pipeline = pipe->get();

  drv3d_dx12::RayDispatchParameters dispatchParams;

  auto rayGenBuffer = (GenericBufferInterface *)rdv.shaderBindingTableSet.rayGenGroup.bindingTableBuffer;
  dispatchParams.rayGenTable = {get_any_buffer_ref(rayGenBuffer), rdv.shaderBindingTableSet.rayGenGroup.offsetInBytes,
    rdv.shaderBindingTableSet.rayGenGroup.sizeInBytes};

  auto missBuffer = (GenericBufferInterface *)rdv.shaderBindingTableSet.missGroup.bindingTableBuffer;
  dispatchParams.missTable = {get_any_buffer_ref(missBuffer), rdv.shaderBindingTableSet.missGroup.offsetInBytes,
    rdv.shaderBindingTableSet.missGroup.sizeInBytes};
  dispatchParams.missStride = rdv.shaderBindingTableSet.missGroup.strideInBytes;

  auto hitBuffer = (GenericBufferInterface *)rdv.shaderBindingTableSet.hitGroup.bindingTableBuffer;
  dispatchParams.hitTable = {
    get_any_buffer_ref(hitBuffer), rdv.shaderBindingTableSet.hitGroup.offsetInBytes, rdv.shaderBindingTableSet.hitGroup.sizeInBytes};
  dispatchParams.hitStride = rdv.shaderBindingTableSet.hitGroup.strideInBytes;

  if (rdv.shaderBindingTableSet.callableGroup.bindingTableBuffer)
  {
    auto callableBuffer = (GenericBufferInterface *)rdv.shaderBindingTableSet.callableGroup.bindingTableBuffer;
    dispatchParams.callableTable = {get_any_buffer_ref(callableBuffer), rdv.shaderBindingTableSet.callableGroup.offsetInBytes,
      rdv.shaderBindingTableSet.callableGroup.sizeInBytes};
    dispatchParams.callableStride = rdv.shaderBindingTableSet.callableGroup.strideInBytes;
  }

  dispatchParams.width = rdv.width;
  dispatchParams.height = rdv.height;
  dispatchParams.depth = rdv.depth;

  DX12_LOCK_FRONT();
  // resolve_resource_binding_table has to be locked by DX12_LOCK_FRONT
  commandStream.pushBack(make_command<CmdDispatchRays>(basicParams, dispatchParams, resolveResourceBindingTable(rbt, pipe)),
    rbt.immediateConstants.data(), rbt.immediateConstants.size());
  immediateModeExecute();
}

void DeviceContext::dispatchRaysIndirect(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchIndirectParameters &rdip)
{
  auto pipe = RayTracePipelineWrapper{pipeline}.get();

  RayDispatchBasicParameters basicParams;
  basicParams.rootSignature = &pipe->getSignature();
  basicParams.pipeline = pipe->get();

  GenericBufferInterface *argsBuffer = (GenericBufferInterface *)rdip.indirectBuffer;
  argsBuffer->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsIndirectBuffer(); });

  RayDispatchIndirectParameters indirectParams;
  indirectParams.argumentBuffer = {get_any_buffer_ref(argsBuffer), rdip.indirectByteOffset};
  indirectParams.argumentStrideInBytes = rdip.indirectByteStride;
  indirectParams.maxCount = rdip.count;

  DX12_LOCK_FRONT();
  // resolve_resource_binding_table has to be locked by DX12_LOCK_FRONT
  commandStream.pushBack(make_command<CmdDispatchRaysIndirect>(basicParams, indirectParams, resolveResourceBindingTable(rbt, pipe)),
    rbt.immediateConstants.data(), rbt.immediateConstants.size());
  immediateModeExecute();
}

void DeviceContext::dispatchRaysIndirectCount(const ::raytrace::ResourceBindingTable &rbt, const ::raytrace::Pipeline &pipeline,
  const ::raytrace::RayDispatchIndirectCountParameters &rdicp)
{
  auto pipe = RayTracePipelineWrapper{pipeline}.get();

  RayDispatchBasicParameters basicParams;
  basicParams.rootSignature = &pipe->getSignature();
  basicParams.pipeline = pipe->get();

  GenericBufferInterface *argsBuffer = (GenericBufferInterface *)rdicp.indirectBuffer;
  GenericBufferInterface *countBuffer = (GenericBufferInterface *)rdicp.countBuffer;

  argsBuffer->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsIndirectBuffer(); });
  countBuffer->updateDeviceBuffer([](auto &buf) { buf.resourceId.markUsedAsIndirectBuffer(); });

  RayDispatchIndirectParameters indirectParams;
  indirectParams.argumentBuffer = {get_any_buffer_ref(argsBuffer), rdicp.indirectByteOffset};
  indirectParams.countBuffer = {get_any_buffer_ref(countBuffer), rdicp.countByteOffset};
  indirectParams.argumentStrideInBytes = rdicp.indirectByteStride;
  indirectParams.maxCount = rdicp.maxCount;

  DX12_LOCK_FRONT();
  // resolve_resource_binding_table has to be locked by DX12_LOCK_FRONT
  commandStream.pushBack(make_command<CmdDispatchRaysIndirect>(basicParams, indirectParams, resolveResourceBindingTable(rbt, pipe)),
    rbt.immediateConstants.data(), rbt.immediateConstants.size());
  immediateModeExecute();
}

namespace
{
template <typename T>
const T *find_slot(uint32_t slot, dag::ConstSpan<const T> values)
{
  for (auto &value : values)
  {
    if (value.slot == slot)
    {
      return &value;
    }
  }
  return nullptr;
}
template <typename T, typename U>
void in_slot_order_visit(uint32_t visitation_mask, dag::ConstSpan<const T> values, U clb)
{
  for (auto visitSlot : LsbVisitor{visitation_mask})
  {
    auto e = find_slot(visitSlot, values);
    if (!e)
    {
      continue;
    }
    clb(visitSlot, *e);
  }
}
template <typename T>
void visit_const_buffer_reads(const ::raytrace::ResourceBindingTable &rbt, uint16_t visitation_mask, T clb)
{
  in_slot_order_visit(visitation_mask, rbt.constantBufferReads,
    [&clb](uint32_t slot, auto &e) { clb(slot, e.buffer, e.offsetInBytes, e.sizeInBytes); });
}
template <typename T>
void visit_reads(const ::raytrace::ResourceBindingTable &rbt, uint32_t visitation_mask, T clb)
{
  for (auto visitSlot : LsbVisitor{visitation_mask})
  {
    auto bSlot = find_slot(visitSlot, rbt.bufferReads);
    Sbuffer *b = bSlot ? bSlot->buffer : nullptr;
    BaseTexture *t = nullptr;
    if (!b)
    {
      auto tSlot = find_slot(visitSlot, rbt.textureReads);
      t = tSlot ? tSlot->texture : nullptr;
    }
    RaytraceTopAccelerationStructure *s = nullptr;
    if (!b && !t)
    {
      auto sSlot = find_slot(visitSlot, rbt.accelerationStructureReads);
      s = sSlot ? sSlot->structure : nullptr;
    }
    clb(visitSlot, b, t, s);
  }
}
template <typename T>
void visit_writes(const ::raytrace::ResourceBindingTable &rbt, uint16_t visitation_mask, T clb)
{
  for (auto visitSlot : LsbVisitor{visitation_mask})
  {
    auto bSlot = find_slot(visitSlot, rbt.bufferWrites);
    Sbuffer *b = bSlot ? bSlot->buffer : nullptr;
    BaseTexture *t = nullptr;
    uint8_t textureMipIndex = 0;
    uint16_t textureArrayIndex = 0;
    bool textureViewAsUI32 = false;
    if (!b)
    {
      auto tSlot = find_slot(visitSlot, rbt.textureWrites);
      if (tSlot)
      {
        t = tSlot->texture;
        textureMipIndex = tSlot->mipIndex;
        textureArrayIndex = tSlot->arrayIndex;
        textureViewAsUI32 = tSlot->viewAsUI32;
      }
    }
    clb(visitSlot, b, t, textureMipIndex, textureArrayIndex, textureViewAsUI32);
  }
}
template <typename T>
void visit_samplers(const ::raytrace::ResourceBindingTable &rbt, uint32_t visitation_mask, T clb)
{
  in_slot_order_visit(visitation_mask, rbt.samples, [&clb](uint32_t slot, auto &e) { clb(slot, e.sampler); });
}
} // namespace

drv3d_dx12::ResourceBindingTable DeviceContext::resolveResourceBindingTable(const ::raytrace::ResourceBindingTable &rbt,
  RayTracePipeline *pipeline)
{

  auto &signture = pipeline->getSignature();
  auto &registers = signture.def.registers;
  drv3d_dx12::ResourceBindingTable result{};
  visit_const_buffer_reads(rbt, registers.bRegisterUseMask, [&](uint32_t, Sbuffer *buffer, uint32_t offset, uint32_t size) {
    result.constBuffers[result.contBufferCount++] = {get_any_buffer_ref((GenericBufferInterface *)buffer), offset, size};
#if 0
// do validation if frame mem buffers
#if DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION
    target.lastDiscardFrameIdx = gbuf->getDiscardFrame();
    target.isStreamBuffer = gbuf->isStreamBuffer();
#endif
    target.constBufferName[index] = gbuf->getResName();
#endif
  });
  visit_reads(rbt, registers.tRegisterUseMask,
    [&](uint32_t, Sbuffer *buffer, BaseTexture *texture, RaytraceTopAccelerationStructure *tlas) {
      if (buffer)
      {
        BufferResourceReferenceAndShaderResourceView refAndView = ((GenericBufferInterface *)buffer)->getDeviceBuffer();
        result.readResources[result.readCount].bufferRef = refAndView;
        result.readResourceViews[result.readCount++] = refAndView.srv;
      }
      else if (texture)
      {
        auto tex = cast_to_texture_base(texture);
        if (tex->isStub())
        {
          tex = tex->getStubTex();
        }

        auto &target = result.readResources[result.readCount];
        target.image = tex->getDeviceImage();
        target.imageView = tex->getViewInfo();
        target.isConstDepthRead = false; // does this even make sense?
        result.readResourceViews[result.readCount++] = device.getImageView(target.image, target.imageView);
      }
      else if (tlas)
      {
        result.readResources[result.readCount].tlas = (RaytraceAccelerationStructure *)tlas;
        result.readResourceViews[result.readCount++] = ((RaytraceAccelerationStructure *)tlas)->descriptor;
      }
    });
  visit_writes(rbt, registers.uRegisterUseMask,
    [&](uint32_t, Sbuffer *buffer, BaseTexture *texture, uint32_t mip, uint32_t ary, bool as_ui32) {
      if (buffer)
      {
        BufferResourceReferenceAndUnorderedResourceView refAndView = ((GenericBufferInterface *)buffer)->getDeviceBuffer();
        result.writeResources[result.writeCount].bufferRef = refAndView;
        result.writeResourceViews[result.writeCount++] = refAndView.uav;
      }
      else if (texture)
      {
        auto tex = cast_to_texture_base(texture);
        auto &target = result.writeResources[result.writeCount];
        target.image = tex->getDeviceImage();
        target.imageView = tex->getViewInfoUav(MipMapIndex::make(mip), ArrayLayerIndex::make(ary), as_ui32);
        result.writeResourceViews[result.writeCount++] = device.getImageView(target.image, target.imageView);
      }
    });
  visit_samplers(rbt, registers.sRegisterUseMask, [&](uint32_t, d3d::SamplerHandle sampler) {
    auto &target = result.samplers[result.samplerCount++];
    target = device.getSampler(sampler);
  });
  return result;
}
#endif

void DeviceContext::resizeSwapchain(Extent2D size)
{
  front.swapchain.prepareForShutdown(device);
  back.swapchain.prepareForShutdown(device);
  // finish all outstanding frames
  back.sharedContextState.frames.walkAll([=](auto &frame) //
    { frame.beginFrame(device.queues, device.pipeMan, frame.frameIndex); });
  SwapchainProperties props;
  props.resolution = size;
  set_hdr_config(props);
  set_hfr_config(props);
  back.swapchain.bufferResize(device, props);
  front.swapchain.bufferResize(size);
  makeReadyForFrame(front.frameIndex);
}

void DeviceContext::waitInternal()
{
  uint64_t progressToWaitFor = front.recordingWorkItemProgress;
  auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush(TidyFrameMode::SyncPoint);

  wait_for_frame_progress_with_event(device.queues, progressToWaitFor, *eventsPool.acquireEvent(), "wait");
}

void DeviceContext::finishInternal()
{
  waitInternal();

  manageLatchedState(TidyFrameMode::SyncPoint);

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

  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdBlitImage used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::tidyFrame(FrontendFrameLatchedData &frame, TidyFrameMode mode)
{
  if (!frame.deletedQueries.empty() && TidyFrameMode::FrameCompleted == mode)
  {
    device.frontendQueryManager.removeDeletedQueries(frame.deletedQueries);
    frame.deletedQueries.clear();
  }

  ResourceMemoryHeap::CompletedFrameExecutionInfo frameCompleteInfo;
  frameCompleteInfo.historyIndex = eastl::distance(front.latchedFrameSet.begin(), &frame);
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

void DeviceContext::waitForCommandFence()
{
  std::atomic_bool signal = false;
  auto cmd = make_command<CmdCommandFence>(signal);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  ::wait(signal, false);
}
#endif

void DeviceContext::initFrameStates()
{
  back.sharedContextState.frames.iterate([=](auto &frame) //
    { frame.init(device.device.get()); });

  back.sharedContextState.bindlessSetManager.init(device.device.get());

  front.latchedFrameSet.fill({});
  front.recordingLatchedFrame = front.latchedFrameSet.begin();

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
#define DX12_BEGIN_CONTEXT_COMMAND(isPrimary, name)                                                       "Cmd" #name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_1(isPrimary, name, param0Type, param0Name)                         "Cmd" #name,
#define DX12_BEGIN_CONTEXT_COMMAND_EXT_2(isPrimary, name, param0Type, param0Name, param1Type, param1Name) "Cmd" #name,
#define DX12_END_CONTEXT_COMMAND
#define DX12_CONTEXT_COMMAND_PARAM(type, name)
#define DX12_CONTEXT_COMMAND_PARAM_ARRAY(type, name, size)
#include "device_context_cmd.inc.h"
#undef DX12_BEGIN_CONTEXT_COMMAND
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_1
#undef DX12_BEGIN_CONTEXT_COMMAND_EXT_2
#undef DX12_END_CONTEXT_COMMAND
#undef DX12_CONTEXT_COMMAND_PARAM
#undef DX12_CONTEXT_COMMAND_PARAM_ARRAY
      "void"};

    auto frameCount = metrics.template getVisitCountFor<CmdFinishFrame>();

    // -1 because last one is void as terminating entry
    eastl::sort(eastl::begin(commandArray), eastl::end(commandArray) - 1, [](auto &l, auto &r) {
      return r.visitCount.value() < l.visitCount.value() ||
             ((r.visitCount.value() == l.visitCount.value()) && (l.cmdIndex < r.cmdIndex));
    });

    logdbg("DX12: Command Stream visited commands statistics:");
    logdbg("%3s, [%3s] %-45s, %10s, %6s, %9s", "#", "ID", "name", "count", "%", "frame avg");
    for (auto at = eastl::begin(commandArray), ed = eastl::end(commandArray) - 1; at != ed; ++at)
    {
      auto perc = at->visitCount.value() * 100.0 / totalCount.value();
      auto perFrame = double(at->visitCount.value()) / frameCount;
      logdbg("%3u, [%3u] %-45s, %8.2f %1s, %5.2f%%, %9.2f", 1 + (at - eastl::begin(commandArray)), at->cmdIndex,
        cmdNames[at->cmdIndex], at->visitCount.units(), at->visitCount.name(), perc, perFrame);
    }
    logdbg("%.2f %s total visits", totalCount.units(), totalCount.name());
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
  auto update = device.resources.allocatePushMemory(device.getDXGIAdapter(), device, sizeof(ConstRegisterType) * data.size(),
    D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
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

  image->dbgValidateImageViewStateCompatibility(view);
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

void DeviceContext::setConstBuffer(uint32_t stage, size_t unit, const ConstBufferSetupInformationStream &info, const char *name)
{
  auto cmd = make_command<CmdSetUniformBuffer>(stage, static_cast<uint32_t>(unit), info);
  uint32_t nameLength = 0;
#if DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION
  nameLength = strlen(name);
#endif
  commandStream.pushBack(cmd, name, nameLength);
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
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdDispatch used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute(true);
}

void DeviceContext::dispatchIndirect(BufferResourceReferenceAndOffset buffer)
{
  auto cmd = make_command<CmdDispatchIndirect>(buffer);
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdDispatchIndirect used during a generic render pass");
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
  auto vertexData = device.resources.allocatePushMemory(device.getDXGIAdapter(), device, vertexDataBytes, sizeof(uint32_t));
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
  auto vertexData = device.resources.allocatePushMemory(device.getDXGIAdapter(), device, vertexDataBytes, sizeof(uint32_t));
  if (!vertexData)
  {
    return;
  }
  auto indexDataBytes = count * sizeof(uint16_t);
  auto indexData = device.resources.allocatePushMemory(device.getDXGIAdapter(), device, indexDataBytes, sizeof(uint16_t));
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
    auto scratchBuffer = device.resources.getTempScratchBufferSpace(device.getDXGIAdapter(), device, data_size, 1);
    auto cmd = make_command<CmdTwoPhaseCopyBuffer>(source, dest.offset, scratchBuffer, data_size);
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdTwoPhaseCopyBuffer used during a generic render pass");
    commandStream.pushBack(cmd);
    immediateModeExecute();
  }
  else
  {
    auto cmd = make_command<CmdCopyBuffer>(source, dest, data_size);
    DX12_LOCK_FRONT();
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdCopyBuffer used during a generic render pass");
    commandStream.pushBack(cmd);
    immediateModeExecute();
  }
}

void DeviceContext::updateBuffer(HostDeviceSharedMemoryRegion update, BufferResourceReferenceAndOffset dest)
{
  if (!update.buffer || !dest.buffer)
  {
    logwarn("DX12: updateBuffer: update.buffer (%p) or dest.buffer (%p) was null, skipping", update.buffer, update.buffer);
    return;
  }
  G_ASSERTF(HostDeviceSharedMemoryRegion::Source::TEMPORARY == update.source,
    "DX12: DeviceContext::updateBuffer called with wrong cpu paged gpu memory type %u", static_cast<uint32_t>(update.source));
  // flush writes
  update.flush();

  auto cmd = make_command<CmdUpdateBuffer>(update, dest);
  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea,
    "DX12: CmdUpdateBuffer (updateBuffer) used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearBufferFloat(BufferResourceReferenceAndClearView buffer, const float values[4])
{
  auto cmd = make_command<CmdClearBufferFloat>(buffer);
  memcpy(cmd.values, values, sizeof(cmd.values));

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdClearBufferFloat used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearBufferInt(BufferResourceReferenceAndClearView buffer, const unsigned values[4])
{
  auto cmd = make_command<CmdClearBufferInt>(buffer);
  memcpy(cmd.values, values, sizeof(cmd.values));

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdClearBufferInt used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::pushEvent(const char *name)
{
  auto length = strlen(name) + 1;
  device.resources.pushEvent(name, name + length - 1);

  auto cmd = make_command<CmdPushEvent>();
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd, name, length);
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
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
  if (activeRenderPassArea)
  {
    for (const auto viewport : viewports)
    {
      G_ASSERT(viewport.x >= activeRenderPassArea->left);
      G_ASSERT(viewport.y >= activeRenderPassArea->top);
      G_ASSERT(viewport.width + viewport.x <= activeRenderPassArea->left + activeRenderPassArea->width);
      G_ASSERT(viewport.height + viewport.y <= activeRenderPassArea->top + activeRenderPassArea->height);
    }
  }
#endif
  commandStream.pushBack(make_command<CmdSetViewports>(), viewports.data(), viewports.size());
  immediateModeExecute();
}

void DeviceContext::clearDepthStencilImage(Image *image, const ImageSubresourceRange &area, const ClearDepthStencilValue &value,
  const eastl::optional<D3D12_RECT> &rect)
{
  auto cmd = make_command<CmdClearDepthStencilTexture>(image, ImageViewState{}, D3D12_CPU_DESCRIPTOR_HANDLE{}, value, rect);

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

void DeviceContext::clearColorImage(Image *image, const ImageSubresourceRange &area, const ClearColorValue &value,
  const eastl::optional<D3D12_RECT> &rect)
{
  auto cmd = make_command<CmdClearColorTexture>(image, ImageViewState{}, D3D12_CPU_DESCRIPTOR_HANDLE{}, value, rect);

  const auto format = image->getFormat();
  cmd.viewState.setRTV();
  cmd.viewState.setFormat(format);
  cmd.viewState.setMipCount(1);
  cmd.viewState.setArrayCount(1);
  if (image->getArrayLayers().count() > 1)
  {
    cmd.viewState.isArray = 1;
  }
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
  if (!src || !dst)
    return;
  auto cmd = make_command<CmdCopyImage>(src, dst, copy);
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdCopyImage used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::blitImage(Image *src, Image *dst, const ImageBlit &region)
{
  DX12_LOCK_FRONT();
  blitImageInternal(src, dst, region, false);
}

void DeviceContext::resolveMultiSampleImage(Image *src, Image *dst)
{
  auto cmd = make_command<CmdResolveMultiSampleImage>(src, dst);

  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::flushDrawsNoLock()
{
  auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush(TidyFrameMode::SyncPoint);
}

void DeviceContext::flushDraws()
{
  DX12_LOCK_FRONT();

  flushDrawsNoLock();
}

bool DeviceContext::noActiveQueriesNoLock() { return front.activeRangedQueries == 0; }

void DeviceContext::wait()
{
  uint64_t progressToWaitFor;
  {
    DX12_LOCK_FRONT();

    progressToWaitFor = front.recordingWorkItemProgress;
    auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
    commandStream.pushBack(cmd);
    immediateModeExecute();
    frontFlush(TidyFrameMode::SyncPoint);
  }
  wait_for_frame_progress_with_event(device.queues, progressToWaitFor, *eventsPool.acquireEvent(), "wait");
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

void DeviceContext::destroyBuffer(BufferState buffer)
{
  if (buffer)
  {
    DX12_LOCK_FRONT();
#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
    auto cmd = make_command<CmdResetBufferState>(buffer);
#endif
    resetBindlessReferences(buffer);
    device.resources.freeBufferOnFrameCompletion(eastl::move(buffer), ResourceMemoryHeap::FreeReason::USER_DELETE);

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
    commandStream.pushBack(cmd);
    immediateModeExecute();
#endif
  }
}

BufferState DeviceContext::discardBuffer(BufferState to_discared, DeviceMemoryClass memory_class, FormatStore format,
  uint32_t struct_size, bool raw_view, bool struct_view, D3D12_RESOURCE_FLAGS flags, uint32_t cflags, const char *name)
{
  DX12_LOCK_FRONT();
  return device.resources.discardBuffer(device.getDXGIAdapter(), device, eastl::move(to_discared), memory_class, format, struct_size,
    raw_view, struct_view, flags, cflags, name, front.frameIndex,
    device.config.features.test(DeviceFeaturesConfig::DISABLE_BUFFER_SUBALLOCATION), device.shouldNameObjects());
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

void DeviceContext::waitForAsyncPresent()
{
  if (!isWaitForASyncPresent)
    return;

  DX12_LOCK_FRONT();

  for (auto finishedFrameId = finishedFrameIndex.load(); finishedFrameId + 1 < front.frameIndex;
       finishedFrameId = finishedFrameIndex.load())
  {
    ::wait(finishedFrameIndex, finishedFrameId);
  }
}

void DeviceContext::gpuLatencyWait()
{
  if (auto waitForSwapchainCount = front.waitForSwapchainCount.load();
      back.swapchain.getPresentedFrameCount() >= waitForSwapchainCount)
  {
    if (front.waitForSwapchainCount.compare_exchange_strong(waitForSwapchainCount, waitForSwapchainCount + 1))
    {
      // block until the swapchain gives its okay for the next frame
      front.swapchain.waitForFrameStart();
    }
  }
}

void DeviceContext::finishFrame(bool present_on_swapchain)
{
#if DX12_RECORD_TIMING_DATA
  auto now = ref_time_ticks();
#endif

  callFrameEndCallbacks();

  // if there was input sampling and the engine missed calling this, made up for it to avoid get out of sync
  gpuLatencyWait();

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

    logdbg("DX12: Frame %u: interval was %ld us, threshold is %ld us; capturing next frame", front.frameIndex, frameInterval,
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

  device.resources.completeFrameRecording(device, device.getDXGIAdapter(), device.bindlessManager,
    eastl::distance(front.latchedFrameSet.begin(), front.recordingLatchedFrame));

  auto cmd = make_command<CmdFinishFrame>(front.recordingWorkItemProgress, lowlatency::get_current_frame(), front.frameIndex,
    present_on_swapchain
#if DX12_RECORD_TIMING_DATA
    ,
    &frameTiming, now
#endif
  );
  commandStream.pushBack(cmd);
  immediateModeExecute();
  frontFlush(TidyFrameMode::FrameCompleted);
  front.frameIndex++;

#if _TARGET_PC_WIN
  if (DAGOR_UNLIKELY(!isPresentAsync()) && hasWorkerThread() && present_on_swapchain)
  {
    back.swapchain.signalPresentSentToBackend();
  }
#endif
}

void DeviceContext::changePresentMode(PresentationMode mode)
{
  if (mode == front.swapchain.getPresentationMode())
  {
    return;
  }

  logdbg("DX12: Swapchain present mode changed to %u, sending mode change to backend...", (uint32_t)mode);
  auto cmd = make_command<CmdChangePresentMode>(mode);
  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdChangePresentMode used during a generic render pass");
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

  logdbg("DX12: Swapchain extent changed to %u / %u, sending mode change to backend...", size.width, size.height);

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

HRESULT DeviceContext::getSwapchainDesc(DXGI_SWAP_CHAIN_DESC *out_desc) const { return back.swapchain.getDesc(out_desc); }

IDXGIOutput *DeviceContext::getSwapchainOutput() const { return back.swapchain.getOutput(); }
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
  if (!img)
  {
    G_ASSERT_LOG(!device.isHealthy(), "DX12: Image is null but device is not lost");
    return;
  }

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
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdMipMapGenSource used during a generic render pass");
      commandStream.pushBack(make_command<CmdMipMapGenSource>(img, blit.srcSubresource.mipLevel, a));
      immediateModeExecute();

      blitImageInternal(img, img, blit, true);

      blit.srcOffsets[1].x = blit.dstOffsets[1].x;
      blit.srcOffsets[1].y = blit.dstOffsets[1].y;
    }

    // to have a uniform state after mip gen, also transition the last mip level to pixel sampling state
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdMipMapGenSource used during a generic render pass");
    commandStream.pushBack(make_command<CmdMipMapGenSource>(img, blit.srcSubresource.mipLevel, a));
    immediateModeExecute();
  }
}

void DeviceContext::setFramebuffer(Image **image_list, ImageViewState *view_list, bool read_only_depth)
{
  static_assert(eastl::is_aggregate_v<CmdSetFramebuffer>);
  auto cmd = make_command<CmdSetFramebuffer>();
  memcpy(cmd.imageList, image_list, sizeof(cmd.imageList));
  memcpy(cmd.viewList, view_list, sizeof(cmd.viewList));
  for (uint32_t i = 0; i < (Driver3dRenderTarget::MAX_SIMRT + 1); ++i)
  {
    if (image_list[i])
    {
      cmd.viewDescriptors[i] = device.getImageView(image_list[i], view_list[i]);
    }
  }
  cmd.readOnlyDepth = read_only_depth;
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::raytraceBuildBottomAccelerationStructure(uint32_t batch_size, uint32_t batch_index,
  RaytraceBottomAccelerationStructure *as, const RaytraceGeometryDescription *desc, uint32_t count, RaytraceBuildFlags flags,
  bool update, BufferResourceReferenceAndAddress scratch_buf, BufferResourceReferenceAndAddress compacted_size)
{
  if (isImmediateMode())
  {
    batch_size = 1;
    batch_index = 0;
  }

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea,
    "DX12: CmdRaytraceBuildBottomAccelerationStructure used during a generic render pass");
  commandStream.pushBack<CmdRaytraceBuildBottomAccelerationStructure, D3D12_RAYTRACING_GEOMETRY_DESC,
    RaytraceGeometryDescriptionBufferResourceReferenceSet>(
    make_command<CmdRaytraceBuildBottomAccelerationStructure>(scratch_buf, compacted_size,
      reinterpret_cast<RaytraceAccelerationStructure *>(as), flags, update, batch_size, batch_index), //
    count,
    [desc](auto index, auto first, auto second) //
    {
      auto pair = raytraceGeometryDescriptionToGeometryDesc(desc[index]);
      first(pair.first);
      second(pair.second);
    });
  immediateModeExecute();
}

void DeviceContext::raytraceBuildTopAccelerationStructure(uint32_t batch_size, uint32_t batch_index,
  RaytraceTopAccelerationStructure *as, BufferReference instance_buffer, uint32_t instance_count, RaytraceBuildFlags flags,
  bool update, BufferResourceReferenceAndAddress scratch_buf)
{
  if (isImmediateMode())
  {
    batch_size = 1;
    batch_index = 0;
  }

  auto cmd = make_command<CmdRaytraceBuildTopAccelerationStructure>();
  cmd.scratchBuffer = scratch_buf;
  cmd.as = reinterpret_cast<RaytraceAccelerationStructure *>(as);
  cmd.instanceBuffer = instance_buffer;
  cmd.instanceCount = instance_count;
  cmd.flags = flags;
  cmd.update = update;
  cmd.batchSize = batch_size;
  cmd.batchIndex = batch_index;

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea,
    "DX12: CmdRaytraceBuildTopAccelerationStructure used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::raytraceCopyAccelerationStructure(RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src,
  bool compact)
{
  auto cmd = make_command<CmdRaytraceCopyAccelerationStructure>();
  cmd.src = src;
  cmd.dst = dst;
  cmd.compact = compact;

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea,
    "DX12: CmdRaytraceCopyAccelerationStructure used during a generic render pass");
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

void DeviceContext::addComputeProgram(ProgramID id, eastl::unique_ptr<ComputeShaderModule> csm, CSPreloaded preloaded)
{
  auto cmd = make_command<CmdAddComputeProgram>(id, csm.release(), preloaded);
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

void DeviceContext::setImageResourceState(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresourceId> range)
{
  DX12_LOCK_FRONT();
  setImageResourceStateNoLock(state, range);
}

void DeviceContext::setImageResourceStateNoLock(D3D12_RESOURCE_STATES state, ValueRange<ExtendedImageGlobalSubresourceId> range)
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
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdClearUAVTextureI used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::clearUAVTexture(Image *image, ImageViewState view, const float values[4])
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdClearUAVTextureF>(image, view, device.getImageView(image, view));
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdClearUAVTextureF used during a generic render pass");
  memcpy(cmd.values, values, sizeof(cmd.values));
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::setRootConstants(unsigned stage, eastl::span<const uint32_t> values)
{
  auto cmd = make_command<CmdSetRootConstants>(stage, RootConstatInfo{values.begin(), values.end()});
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd, false /*wake executor*/);
  immediateModeExecute();
}

void DeviceContext::beginGenericRenderPassChecks([[maybe_unused]] const RenderPassArea &renderPassArea)
{
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: Nested generic render pass detected");
  activeRenderPassArea = renderPassArea;
#endif
}

void DeviceContext::endGenericRenderPassChecks()
{
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(activeRenderPassArea, "DX12: End generic render pass called without a begin call");
  activeRenderPassArea.reset();
#endif
}

#if _TARGET_PC_WIN
void DeviceContext::preRecovery()
{
  dumpCommandLog(back.sharedContextState);

  mutex.lock();

  auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
  commandStream.pushBack(cmd);
  immediateModeExecute();
  waitForCommandFence();

  closeFrameEndCallbacks();

  back.sharedContextState.preRecovery(device.queues, device.pipeMan);
  back.constBufferStreamDescriptors.shutdown();

  // clear out all vectors
  front.latchedFrameSet.fill({});
  front.recordingLatchedFrame = nullptr;
  front.recordingWorkItemProgress = 1;
  front.nextWorkItemProgress = 2;
  front.completedFrameProgress = 0;

  front.swapchain.preRecovery();
  back.swapchain.preRecovery();
}

void DeviceContext::ContextState::preRecovery(DeviceQueueGroup &queue_group, PipelineManager &pipe_man)
{
  // simply set to zero descriptor for now, we don't have a valid one for that
  onFrameStateInvalidate({0});
  purgeAllBindings();

  frames.walkAll([&queue_group, &pipe_man](auto &frame) //
    { frame.preRecovery(queue_group, pipe_man); });

  bindlessSetManager.preRecovery();

  drawIndirectSignatures.reset();
  drawIndexedIndirectSignatures.reset();
  dispatchIndirectSignature.Reset();

  graphicsCommandListBarrierBatch.purgeAll();
  uploadBarrierBatch.purgeAll();
  postUploadBarrierBatch.purgeAll();
  readbackBarrierBatch.purgeAll();

  initialResourceStateSet = {};
  resourceStates = {};
  bufferAccessTracker = {};
  cmdBuffer = {};
}

void DeviceContext::recover(const dag::Vector<D3D12_CPU_DESCRIPTOR_HANDLE> &unbounded_samplers)
{
  back.sharedContextState.frames.walkAll([=](auto &frame) //
    { frame.recover(device.device.get()); });
  back.sharedContextState.bindlessSetManager.init(device.device.get());

  front.recordingLatchedFrame = front.latchedFrameSet.begin();
  back.sharedContextState.bindlessSetManager.recover(device.device.get(), unbounded_samplers);
  back.constBufferStreamDescriptors.init(device.device.get());

  ResourceMemoryHeap::BeginFrameRecordingInfo frameRecodingInfo;
  frameRecodingInfo.historyIndex = 0;
  device.resources.beginFrameRecording(frameRecodingInfo);

  makeReadyForFrame(front.frameIndex);

  mutex.unlock();
}
#endif

void DeviceContext::deleteTexture(BaseTex *tex)
{
  STORE_RETURN_ADDRESS();
  resetBindlessReferences(tex);
  DX12_LOCK_FRONT();
  device.resources.deleteTextureObjectOnFrameCompletion(tex);
}

void DeviceContext::resetBindlessReferences(BaseTex *tex) { device.bindlessManager.resetTextureReferences(*this, tex); }

void DeviceContext::resetBindlessReferences(BufferState &buffer)
{
  if (!buffer.srvs)
    return;
  device.bindlessManager.resetBufferReferences(*this, buffer.currentSRV());
}

void DeviceContext::updateBindlessReferences(D3D12_CPU_DESCRIPTOR_HANDLE old_descriptor, D3D12_CPU_DESCRIPTOR_HANDLE new_descriptor)
{
  // need to take the lock here to keep consistent ordering
  ScopedCommitLock ctxLock{*this};
  OSSpinlockScopedLock bindlessStateLock{get_resource_binding_guard()};
  device.bindlessManager.updateBufferReferencesNoLock(*this, old_descriptor, new_descriptor);
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

void DeviceContext::uploadToImage(const BaseTex &dst_tex, const BufferImageCopy *regions, uint32_t region_count,
  HostDeviceSharedMemoryRegion memory, DeviceQueueType queue, bool is_discard)
{
  DX12_LOCK_FRONT();
  Image *target = dst_tex.getDeviceImage();
  G_ASSERT(target);
  G_ASSERT(target->getHandle());
  G_ASSERT(memory.buffer);
  if (!target || !target->getHandle() || !memory.buffer)
  {
    D3D_ERROR("DX12: DeviceContext::uploadToImage: invalid argument (target: %p, target->getHandle(): %p, memory.buffer: %p)", target,
      target ? target->getHandle() : nullptr, memory.buffer);
    if (target && !target->getHandle())
    {
      target->getDebugName([=](auto &name) {
        D3D_ERROR("DX12: DeviceContext::uploadToImage: target->getHandle() is nullptr (target: %p, name: %s)", target, name.c_str());
      });
    }
    return;
  }
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
  suppressImmediateFlush();
  immediateModeExecute();
}

void DeviceContext::endVisibilityQuery(Query *q)
{
  auto cmd = make_command<CmdEndVisibilityQuery>(q);
  DX12_LOCK_FRONT();
  --front.activeRangedQueries;
  commandStream.pushBack(cmd);
  restoreImmediateFlush();
  immediateModeExecute();
}

#if _TARGET_XBOX
void DeviceContext::suspendExecution()
{
  // block creation of resources
  device.resources.enterSuspended();

  logdbg("DX12: DeviceContext::suspendExecution called...");
  // First lock the context, so no one else can issue commands
  getFrontGuard().lock();
#if !FIXED_EXECUTION_MODE
  if (ExecutionMode::CONCURRENT == executionMode)
#endif
  {
    logdbg("DX12: Sending suspend request to worker thread...");
    // Now tell the worker to enter suspended state, it will signal enteredSuspendedStateEvent and
    // wait for resumeExecutionEvent to be signaled
    commandStream.pushBack(make_command<CmdEnterSuspendState>());
    WaitForSingleObject(enteredSuspendedStateEvent.get(), INFINITE);
    logdbg("DX12: Worker is now in suspend...");
  }

  logdbg("DX12: Sending suspend command to API...");
  // Engine requests suspended state
  // Tell DX12 driver to safe the current GPU and driver state
  device.queues.enterSuspendedState();
  logdbg("DX12: API suspended...");
}
void DeviceContext::resumeExecution()
{
  logdbg("DX12: Sending resume command to API...");
  // If we end up here, we got the resume signal
  device.queues.leaveSuspendedState();
  logdbg("DX12: API resumed...");

#if _TARGET_XBOX
  logdbg("DX12: Restoring swapchain configuration...");
  // We have to restore swapchain state
  back.swapchain.restoreAfterSuspend(device.device.get());
#endif

#if !FIXED_EXECUTION_MODE
  if (ExecutionMode::CONCURRENT == executionMode)
#endif
  {
    logdbg("DX12: Sending worker resume request to worker thread...");
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
  {
    DX12_LOCK_FRONT();
    if (progress == front.recordingWorkItemProgress)
    {
      auto cmd = make_command<CmdFlushWithFence>(front.recordingWorkItemProgress);
      commandStream.pushBack(cmd);
      immediateModeExecute();
      frontFlush(TidyFrameMode::SyncPoint);
    }
  }
  wait_for_frame_progress_with_event(device.queues, progress, *eventsPool.acquireEvent(), "waitForProgress");
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
    const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");

    int xessQuality = -1;
    if (blk_video.getNameId("antialiasing_mode") > -1 && blk_video.getNameId("antialiasing_upscaling") > -1)
    {
      if (stricmp(blk_video.getStr("antialiasing_mode", "off"), "xess") == 0)
      {
        auto quality = blk_video.getStr("antialiasing_upscaling", "native");
        if (stricmp(quality, "native") == 0)
          xessQuality = 5;
        else if (stricmp(quality, "ultra_quality") == 0)
          xessQuality = 3;
        else if (stricmp(quality, "quality") == 0)
          xessQuality = 2;
        else if (stricmp(quality, "balanced") == 0)
          xessQuality = 1;
        else if (stricmp(quality, "performance") == 0)
          xessQuality = 0;
        else if (stricmp(quality, "ultra_performance") == 0)
          xessQuality = 6;
      }
    }
    else
      xessQuality = blk_video.getInt("xessQuality", -1);

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

void DeviceContext::initDLSS()
{
#if !_TARGET_XBOX
  if (streamlineAdapter)
  {
    auto toExtent2D = [](auto size) { return Extent2D({static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height)}); };
    auto targetResolution = stereo_config_callback && stereo_config_callback->desiredStereoRender()
                              ? toExtent2D(stereo_config_callback->desiredRendererSize())
                              : getSwapchainExtent();
    bool wantStereoRender = stereo_config_callback ? stereo_config_callback->desiredStereoRender() : false;
    createDlssFeature(wantStereoRender, targetResolution.width, targetResolution.height);
    wait();
  }
#endif
}

void DeviceContext::shutdownDLSS()
{
#if !_TARGET_XBOX
  if (streamlineAdapter)
  {
    bool wantStereoRender = stereo_config_callback ? stereo_config_callback->desiredStereoRender() : false;
    releaseDlssFeature(wantStereoRender);
    wait();
  }
#endif
}

void DeviceContext::initStreamline([[maybe_unused]] ComPtr<DXGIFactory> &factory, [[maybe_unused]] DXGIAdapter *adapter)
{
#if !_TARGET_XBOX
  D3D12_FEATURE_DATA_VIDEO_EXTENSION_COMMAND_COUNT extensionCommandCount{};
  ComPtr<ID3D12VideoDevice> videoDevice;
  bool hasVideoExtensionCommands = SUCCEEDED(device.device->QueryInterface(COM_ARGS(&videoDevice))) &&
                                   SUCCEEDED(videoDevice->CheckFeatureSupport(D3D12_FEATURE_VIDEO_EXTENSION_COMMAND_COUNT,
                                     &extensionCommandCount, sizeof(extensionCommandCount))) &&
                                   extensionCommandCount.CommandCount > 0;

  StreamlineAdapter::SupportOverrideMap supportOverride;
  constexpr uint32_t kFeatureDLSS_G = 1000;
  if (!hasVideoExtensionCommands)
    supportOverride[kFeatureDLSS_G] = nv::SupportState::NotSupported;
  if (StreamlineAdapter::init(streamlineAdapter, StreamlineAdapter::RenderAPI::DX12, supportOverride))
  {
    factory = StreamlineAdapter::hook(factory);
    device.device = StreamlineAdapter::hook(device.device.get());
    streamlineAdapter->setAdapterAndDevice(adapter, device.device.get());
    streamlineAdapter->initializeDlssState();
  }
#endif
}

void DeviceContext::preRecoverStreamline()
{
#if !_TARGET_XBOX
  if (streamlineAdapter)
  {
    streamlineAdapter->preRecover();
  }
#endif
}

void DeviceContext::recoverStreamline([[maybe_unused]] DXGIAdapter *adapter)
{
#if !_TARGET_XBOX
  if (streamlineAdapter)
  {
    streamlineAdapter->recover();
    device.device = StreamlineAdapter::hook(device.device.get());
    streamlineAdapter->setAdapterAndDevice(adapter, device.device.get());
    streamlineAdapter->initializeDlssState();
  }
#endif
}

void DeviceContext::shutdownStreamline() { streamlineAdapter.reset(); }

void DeviceContext::shutdownXess() { xessWrapper.xessShutdown(); }

void DeviceContext::shutdownFsr2()
{
#if !_TARGET_XBOX
  fsr2Wrapper.shutdown();
#endif
}

void DeviceContext::createDlssFeature(bool stereo_render, int output_width, int output_height)
{
  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdCreateDlssFeature>(stereo_render, output_width, output_height);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::releaseDlssFeature(bool stereo_render)
{
  auto cmd = make_command<CmdReleaseDlssFeature>(stereo_render);
  DX12_LOCK_FRONT();
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::executeDlss(const nv::DlssParams<BaseTexture> &dlss_params, int view_index)
{
  nv::DlssParams<Image> dlssParams =
    nv::convertDlssParams(dlss_params, [](BaseTexture *t) { return t ? cast_to_texture_base(t)->getDeviceImage() : nullptr; });

  auto cmd = make_command<CmdExecuteDlss>(dlssParams, view_index, streamlineAdapter->getFrameId());

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdExecuteDlss used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::executeDlssG(const nv::DlssGParams<BaseTexture> &dlss_g_params, int view_index)
{
  nv::DlssGParams<Image> dlssGParams =
    nv::convertDlssGParams(dlss_g_params, [](BaseTexture *t) { return t ? cast_to_texture_base(t)->getDeviceImage() : nullptr; });

  auto cmd = make_command<CmdExecuteDlssG>(dlssGParams, view_index);

  DX12_LOCK_FRONT();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdExecuteDlssG used during a generic render pass");
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
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdExecuteXess used during a generic render pass");
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::executeFSR(amd::FSR *fsr, const amd::FSR::UpscalingArgs &params)
{
  auto cast_to_image = [](BaseTexture *src) {
    if (src)
      return cast_to_texture_base(src)->getDeviceImage();
    return (Image *)nullptr;
  };

  FSRUpscalingArgs args = params;
  args.colorTexture = cast_to_image(params.colorTexture);
  args.depthTexture = cast_to_image(params.depthTexture);
  args.motionVectors = cast_to_image(params.motionVectors);
  args.exposureTexture = cast_to_image(params.exposureTexture);
  args.outputTexture = cast_to_image(params.outputTexture);
  args.reactiveTexture = cast_to_image(params.reactiveTexture);
  args.transparencyAndCompositionTexture = cast_to_image(params.transparencyAndCompositionTexture);

  DX12_LOCK_FRONT();
  auto cmd = make_command<CmdDispatchFSR>(fsr, args);
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdDispatchFSR used during a generic render pass");
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
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea, "DX12: CmdDispatchFSR2 used during a generic render pass");
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

#if D3D_HAS_RAY_TRACING
void DeviceContext::blasBarrier(RaytraceBottomAccelerationStructure *blas, GpuPipeline queue)
{
  auto cmd = make_command<CmdAsBarrier>(reinterpret_cast<RaytraceAccelerationStructure *>(blas), queue);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}
#endif

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

void DeviceContext::bindlessSetResourceDescriptorNoLock(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  G_ASSERT(descriptor.ptr != 0);
  auto cmd = make_command<CmdBindlessSetResourceDescriptor>(slot, descriptor);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bindlessSetResourceDescriptorNoLock(uint32_t slot, Image *texture, ImageViewState view)
{
  auto cmd = make_command<CmdBindlessSetTextureDescriptor>(slot, texture, device.getImageView(texture, view));
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bindlessSetSamplerDescriptorNoLock(uint32_t slot, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
  auto cmd = make_command<CmdBindlessSetSamplerDescriptor>(slot, descriptor);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::bindlessCopyResourceDescriptorsNoLock(uint32_t src, uint32_t dst, uint32_t count)
{
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
  if (!buffer.buffer)
  {
    logwarn("DX12: pushBufferUpdate: buffer.buffer was null, skipping");
    return;
  }
  DX12_LOCK_FRONT();
  auto update = device.resources.allocateUploadRingMemory(device.getDXGIAdapter(), device, data_size, 1);
  if (!update)
  {
    logwarn("DX12: pushBufferUpdate: update was null, skipping");
    return;
  }
  memcpy(update.pointer, data, data_size);
  update.flush();
  VALIDATE_GENERIC_RENDER_PASS_CONDITION(!activeRenderPassArea,
    "DX12: CmdUpdateBuffer (pushBufferUpdate) used during a generic render pass");
  auto cmd = make_command<CmdUpdateBuffer>(update, buffer);
  commandStream.pushBack(cmd);
  immediateModeExecute();
}

void DeviceContext::updateFenceProgress()
{
  DX12_LOCK_FRONT();
  for (auto &frame : front.latchedFrameSet)
  {
    if (frame.progress && frame.progress < device.queues.checkFrameProgress() + 1)
    {
      tidyFrame(frame, TidyFrameMode::SyncPoint);
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
#else
void DeviceContext::onSwapchainSwapCompleted() {}
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
    G_ASSERTF_RETURN(memory, , "DX12: getUserHeapMemory for user heap %p returned empty memory range for texture %p <%s>", heap, tex,
      tex->getResName());
  }

  DX12_LOCK_FRONT();
  auto image = tex->getDeviceImage();
  G_ASSERTF_RETURN(image, , "DX12: mapTileToResource: image is nullptr for texture %p <%s>", tex, tex->getResName());
#if _TARGET_PC_WIN
  commandStream.pushBack(
    make_command<CmdBeginTileMapping>(image, memory.getHeap(), from_byte_offset_to_page_offset(memory.getOffset()), mapping_count));
#else
  commandStream.pushBack(make_command<CmdBeginTileMapping>(image, memory.getAddress(), memory.size(), mapping_count));
#endif
  immediateModeExecute();

  constexpr size_t batchSize = 1000;
  auto cmd = make_command<CmdAddTileMappings>();
  auto fullBatches = mapping_count / batchSize;
  auto extraBatch = mapping_count % batchSize;
  for (size_t i = 0; i < fullBatches; ++i, mapping += batchSize)
  {
    commandStream.pushBack(cmd, mapping, batchSize);
    immediateModeExecute();
  }
  if (extraBatch)
  {
    commandStream.pushBack(cmd, mapping, extraBatch);
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
  return device.resources.allocatePushMemory(device.getDXGIAdapter(), device, size, alignment);
}


void DeviceContext::moveBuffer(BufferResourceReferenceAndOffset from, BufferResourceReferenceAndRange to)
{
  DX12_LOCK_FRONT();
  commandStream.pushBack(make_command<CmdMoveBuffer>(from, to));
  immediateModeExecute();
}

void DeviceContext::moveTextureNoLock(Image *from, Image *to)
{
  commandStream.pushBack(make_command<CmdMoveTexture>(from, to));
  immediateModeExecute();
}


void DeviceContext::WorkerThread::execute()
{
  TIME_PROFILE_THREAD(getCurrentThreadName());
#if _TARGET_XBOX
  gdk::make_thread_time_sensitive();
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

void DeviceContext::ExecutionContext::setUniformBuffer(uint32_t stage, uint32_t unit, const ConstBufferSetupInformationStream &info,
  StringIndexRef::RangeType name)
{
  G_UNUSED(name);
  ConstBufferSetupInformation infoSlot;
  infoSlot.buffer = info.buffer;
#if DX12_VALIDATE_STREAM_CB_USAGE_WITHOUT_INITIALIZATION
  infoSlot.isStreamBuffer = info.isStreamBuffer;
  infoSlot.lastDiscardFrameIdx = info.lastDiscardFrameIdx;
  infoSlot.name = {name.cbegin(), name.cend()};
#endif
  contextState.stageState[stage].setConstBuffer(unit, infoSlot);
  if (info.buffer.resourceId)
  {
    contextState.bufferAccessTracker.updateLastFrameAccess(info.buffer.resourceId);
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

  if (image)
    image->dbgValidateImageViewStateCompatibility(view_state);
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
  if (!contextState.activeReadBackCommandList && device.isHealthyOrRecovering())
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
  if (!contextState.activeEarlyUploadCommandList && device.isHealthyOrRecovering())
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
  if (!contextState.activeLateUploadCommandList && device.isHealthyOrRecovering())
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
  auto hasDepthStencil = 0 != contextState.graphicsState.framebufferState.framebufferLayout.hasDepth;
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

void DeviceContext::ExecutionContext::checkCloseCommandListResult(HRESULT result, [[maybe_unused]] eastl::string_view debug_name,
  [[maybe_unused]] const debug::CommandListLogger &logger) const
{
  DX12_CHECK_RESULT(result);
#if DAGOR_DBGLEVEL > 0
  if (FAILED(result))
  {
    D3D_ERROR("Command list '%s' close failed", debug_name);
    self.dumpActiveFrameCommandLog(self.back.sharedContextState);
    debug::CommandLogDecoder::dump_commands_to_log(logger.getCommands());
  }
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
  // On device error we reset the command buffer and do not want to create it again until the device is not recovered
  if (!contextState.cmdBuffer.isReadyForRecording() && device.isHealthyOrRecovering())
  {
    auto buffer = frame.genericCommands.allocateList(device.device.get());
    contextState.cmdBuffer.makeReadyForRecording(buffer, device.hasDepthBoundsTest(), device.getVariableShadingRateTier());

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

int64_t DeviceContext::ExecutionContext::flush(uint64_t progress, bool frame_end, bool present_on_swapchain)
{
  G_ASSERTF(frame_end || !present_on_swapchain, "Calling ctx.flush for presenting before the frame end is incorrect");

  TIME_PROFILE_DEV(DeviceContext::ExecutionContext::flush);

  int64_t swapDur = 0;
  FrameInfo &frame = contextState.getFrameData();

  // adds barriers for read back access
  readBackImagePrePhase();

  if (present_on_swapchain)
  {
    contextState.resourceStates.useTextureAsSwapImageOutput(contextState.graphicsCommandListBarrierBatch,
      contextState.graphicsCommandListSplitBarrierTracker, self.back.swapchain.getColorImage());
  }

  // have to reverse the order how stuff is construct the command buffers
  // a command pool can only supply one command buffer at a time and
  // we don't need to have an extra one if we can avoid it by building stuff in the correct order
  contextState.cmdBuffer.prepareBufferForSubmit();
  auto frameCore = contextState.cmdBuffer.getHandle();
  if (frameCore)
  {
    contextState.debugEndCommandBuffer(device, frameCore);

    contextState.resourceStates.implicitUAVFlushAll(device.currentEventPath().data(), device.validatesUserBarriers());

    contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

    contextState.getFrameData().backendQueryManager.resolve(frameCore);
    checkCloseCommandListResult(frameCore->Close(), "graphics", contextState.cmdBuffer.getBufferWrapper());
  }
  contextState.cmdBuffer.resetBuffer();


  // TODO with compute queue support, we need to submit preFrameSet.copySyncGeneric to the graphics queue
  //      then sync graphics progress with compute progress, so that we are sure that compute can only
  //      access stuff after its in the right state


  // Uploads of resources that are already in a state where the upload queue can write to them
  bool hasEarlyUpload = static_cast<bool>(contextState.activeEarlyUploadCommandList);
  if (hasEarlyUpload)
  {
    auto cmdList = contextState.activeEarlyUploadCommandList.get();
    checkCloseCommandListResult(cmdList->Close(), "early upload", contextState.activeEarlyUploadCommandList);
    contextState.activeEarlyUploadCommandList.reset();
    ID3D12CommandList *submit = cmdList;
    device.queues[DeviceQueueType::UPLOAD].enqueueCommands(1, &submit);
  }

  // Transition resources into a state where the upload queue can write to them
  if (contextState.uploadBarrierBatch.hasBarriers())
  {
    if (auto cmdList = frame.genericCommands.allocateList(device.device.get()))
    {
      GraphicsCommandList<AnyCommandListPtr> wrappedCmdList{cmdList};
      contextState.uploadBarrierBatch.execute(wrappedCmdList);

      checkCloseCommandListResult(cmdList->Close(), "upload barrier batch", wrappedCmdList);
      ID3D12CommandList *submit = cmdList.get();
      device.queues[DeviceQueueType::GRAPHICS].enqueueCommands(1, &submit);
    }
  }

  // Upload of resources that where just transitioned into the needed state to write with the upload queue
  bool hasLateUpload = static_cast<bool>(contextState.activeLateUploadCommandList);
  if (hasLateUpload)
  {
    device.queues.synchronizeUploadWithGraphics();

    auto cmdList = contextState.activeLateUploadCommandList.get();
    checkCloseCommandListResult(cmdList->Close(), "late upload", contextState.activeLateUploadCommandList);
    contextState.activeLateUploadCommandList.reset();
    ID3D12CommandList *submit = cmdList;
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

        checkCloseCommandListResult(cmdList->Close(), "post upload barrier batch", wrappedCmdList);
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
    submits[submitCount++] = frameCore;
  }

  // we have to wait for previous frame to avoid overlapping previous frame read back and current frame graphics
  device.queues.synchronizeWithPreviousFrame();

  if (submitCount)
  {
    // flush the frame and possible uploaded resource flush
    device.queues[DeviceQueueType::GRAPHICS].enqueueCommands(submitCount, submits);
  }

  if (frame_end)
  {
    for (FrameEvents *callback : self.back.frameEventCallbacks)
      callback->endFrameEvent();
    self.back.frameEventCallbacks.clear();

    auto startPresent = ref_time_ticks();
    if (present_on_swapchain)
    {
      self.back.swapchain.present(device);
    }
    swapDur = ref_time_ticks() - startPresent;
    frame.progress = progress;

    contextState.debugFramePresent(device);
  }

  if (contextState.readbackBarrierBatch.hasBarriers())
  {
    if (auto cmdList = frame.genericCommands.allocateList(device.device.get()))
    {
      GraphicsCommandList<AnyCommandListPtr> wrappedCmdList{cmdList};
      contextState.readbackBarrierBatch.execute(wrappedCmdList);

      checkCloseCommandListResult(cmdList->Close(), "readback barrier batch", wrappedCmdList);
      ID3D12CommandList *submit = cmdList.get();
      device.queues[DeviceQueueType::GRAPHICS].enqueueCommands(1, &submit);
    }
  }

  // handle read backs
  if (contextState.activeReadBackCommandList)
  {
    auto cmdList = contextState.activeReadBackCommandList.get();
    checkCloseCommandListResult(cmdList->Close(), "readback", contextState.activeReadBackCommandList);
    contextState.activeReadBackCommandList.reset();

    device.queues.synchronizeReadBackWithGraphics();
    ID3D12CommandList *submit = cmdList;
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
  if (frame_end)
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

void DeviceContext::ExecutionContext::addComputePipeline(ProgramID id, ComputeShaderModule *csm, CSPreloaded preloaded)
{
  // if optimization pass is used, then it has to handle this, as it might needs the data for later
  // commands
  eastl::unique_ptr<ComputeShaderModule> sm{csm};
  // module id is the compute program id, no need for extra param
  device.pipeMan.addCompute(device.device.get(), device.pipelineCache, id, eastl::move(*csm),
    get_recover_behavior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
      device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
    device.shouldNameObjects(), preloaded);
}

void DeviceContext::ExecutionContext::addGraphicsPipeline(GraphicsProgramID program, ShaderID vs, ShaderID ps)
{
  device.pipeMan.addGraphics(device.device.get(), device.pipelineCache, contextState.framebufferLayouts, program, vs, ps,
    get_recover_behavior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
      device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
    device.shouldNameObjects());
}

void DeviceContext::ExecutionContext::registerStaticRenderState(StaticRenderStateID ident, const RenderStateSystem::StaticState &state)
{
  device.pipeMan.registerStaticRenderState(ident, state);
}

#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::buildBottomAccelerationStructure(uint32_t batch_size, uint32_t batch_index,
  D3D12_RAYTRACING_GEOMETRY_DESC_ListRef::RangeType geometry_descriptions,
  RaytraceGeometryDescriptionBufferResourceReferenceSetListRef::RangeType resource_refs,
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update, RaytraceAccelerationStructure *dst,
  RaytraceAccelerationStructure *src, BufferResourceReferenceAndAddress scratch_buffer,
  BufferResourceReferenceAndAddress compacted_size)
{
  G_UNUSED(batch_size);
  G_UNUSED(batch_index);

  static_assert(RT_TRANSFORM_SIZE == sizeof(mat43f));

  if (!readyCommandList())
  {
    return;
  }

#if _TARGET_SCARLETT
  ctxScarlett.buildBottomAccelerationStructure(batch_size, batch_index, geometry_descriptions);
#endif

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
    contextState.resourceStates.useRTASAsUpdateSource(contextState.graphicsCommandListBarrierBatch, src->asHeapResource);
  }

  if (compacted_size)
  {
    contextState.resourceStates.useBufferAsUAV(contextState.graphicsCommandListBarrierBatch, STAGE_CS, compacted_size);
  }

  contextState.resourceStates.useRTASAsBuildTarget(contextState.graphicsCommandListBarrierBatch, dst->asHeapResource);

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelInputs = {};
  bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  bottomLevelInputs.Flags = flags;
  if (update)
    bottomLevelInputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
#if _TARGET_SCARLETT
  if (!ctxScarlett.updateBottomLevelInputs(batch_size, batch_index, bottomLevelInputs))
#endif
  {
    bottomLevelInputs.NumDescs = geometry_descriptions.size();
    bottomLevelInputs.pGeometryDescs = geometry_descriptions.data();
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc = {};
  bottomLevelBuildDesc.Inputs = bottomLevelInputs;
  bottomLevelBuildDesc.ScratchAccelerationStructureData = scratch_buffer.gpuPointer;
  bottomLevelBuildDesc.DestAccelerationStructureData = dst->gpuAddress;
  if (src)
  {
    bottomLevelBuildDesc.SourceAccelerationStructureData = src->gpuAddress;
  }
  else if (update)
  {
    bottomLevelBuildDesc.SourceAccelerationStructureData = bottomLevelBuildDesc.DestAccelerationStructureData;
  }

  // TODO: not 100% sure if needed or wanted, have to see
  disablePredication();

  int postbuildCount = 0;
  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_DESC postBuildInfo;

  if (flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION)
  {
    postBuildInfo.DestBuffer = compacted_size.gpuPointer;
    postBuildInfo.InfoType = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_POSTBUILD_INFO_COMPACTED_SIZE;
    postbuildCount = 1;
  }

#if _TARGET_SCARLETT
  if (!ctxScarlett.buildBottomRaytracingAccelerationStructure(batch_size, batch_index, bottomLevelBuildDesc, postbuildCount,
        postBuildInfo, contextState))
#endif
    contextState.cmdBuffer.buildRaytracingAccelerationStructure(&bottomLevelBuildDesc, postbuildCount, &postBuildInfo);
}

void DeviceContext::ExecutionContext::buildTopAccelerationStructure(uint32_t batch_size, uint32_t batch_index, uint32_t instance_count,
  BufferResourceReferenceAndAddress instance_buffer, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, bool update,
  RaytraceAccelerationStructure *dst, RaytraceAccelerationStructure *src, BufferResourceReferenceAndAddress scratch)
{
  G_UNUSED(batch_size);
  G_UNUSED(batch_index);

  if (!readyCommandList())
  {
    return;
  }

#if _TARGET_SCARLETT
  ctxScarlett.buildTopAccelerationStructure(batch_size, batch_index);
#endif

  contextState.resourceStates.useBufferAsRTASBuildSource(contextState.graphicsCommandListBarrierBatch, instance_buffer);
  contextState.bufferAccessTracker.updateLastFrameAccess(instance_buffer.resourceId);
  dirtyBufferState(instance_buffer.resourceId);

  if (src)
  {
    contextState.resourceStates.useRTASAsUpdateSource(contextState.graphicsCommandListBarrierBatch, src->asHeapResource);
  }

  contextState.resourceStates.useRTASAsBuildTarget(contextState.graphicsCommandListBarrierBatch, dst->asHeapResource);

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  if (batch_index == 0)
    contextState.resourceActivationTracker.flushAll(contextState.graphicsCommandListBarrierBatch);

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
  topLevelBuildDesc.DestAccelerationStructureData = dst->gpuAddress;
  topLevelBuildDesc.ScratchAccelerationStructureData = scratch.gpuPointer;
  if (src)
  {
    topLevelBuildDesc.SourceAccelerationStructureData = src->gpuAddress;
  }
  else if (update)
  {
    topLevelBuildDesc.SourceAccelerationStructureData = dst->gpuAddress;
  }

  // TODO: not 100% sure if needed or wanted, have to see
  disablePredication();

#if _TARGET_SCARLETT
  if (!ctxScarlett.buildTopRaytracingAccelerationStructure(batch_size, batch_index, topLevelBuildDesc, contextState,
        dst->asHeapResource, scratch.buffer))
#endif
  {
    contextState.cmdBuffer.buildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);

    // Always flush scratch buffer, probably okay without it, but not correct.
    contextState.graphicsCommandListBarrierBatch.flushUAV(scratch.buffer);
    // Ensure writes are completed before we can use it as AS in shaders
    contextState.graphicsCommandListBarrierBatch.flushUAV(dst->asHeapResource);
  }
}

void DeviceContext::ExecutionContext::copyRaytracingAccelerationStructure(RaytraceAccelerationStructure *dst,
  RaytraceAccelerationStructure *src, bool compact)
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.useRTASAsUpdateSource(contextState.graphicsCommandListBarrierBatch, src->asHeapResource);
  contextState.resourceStates.useRTASAsBuildTarget(contextState.graphicsCommandListBarrierBatch, dst->asHeapResource);

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  contextState.cmdBuffer.copyRaytracingAccelerationStructure(dst->gpuAddress, src->gpuAddress,
    compact ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_COMPACT : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_CLONE);

  contextState.graphicsCommandListBarrierBatch.flushUAV(dst->asHeapResource);
}
#endif

void DeviceContext::ExecutionContext::continuePipelineSetCompilation() const
{
  // We are using slightly lower value than the target
  static constexpr uint32_t MAX_FRAME_WITH_COMPILATION_TIME_USEC = 30 * 1000;
  static constexpr uint64_t MIN_PIPELINES_TO_COMPILE_PER_FRAME = 10;
  uint64_t called = 0;
  auto cancellationPredicate = [this, &called]() {
    if (called++ < MIN_PIPELINES_TO_COMPILE_PER_FRAME)
      return false;
    return get_time_usec(self.back.previousPresentEndTicks) >= MAX_FRAME_WITH_COMPILATION_TIME_USEC;
  };
  device.pipeMan.continuePipelineSetCompilation(device.getDevice(), device.pipelineCache, contextState.framebufferLayouts,
    cancellationPredicate);
}

void DeviceContext::ExecutionContext::finishFrame(uint64_t progress, Drv3dTimings *timing_data, int64_t kickoff_stamp,
  uint32_t latency_frame, uint32_t front_frame, bool present_on_swapchain)
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
    auto swapDur = flush(progress, true, present_on_swapchain);
#if DX12_RECORD_TIMING_DATA
    timing_data->presentDuration = swapDur;
#else
    G_UNUSED(swapDur);
#endif
  }

  self.makeReadyForFrame(front_frame + 1, present_on_swapchain);
  self.initNextFrameLog();
  device.pipeMan.evictDecompressionCache();
  continuePipelineSetCompilation();
  self.back.previousPresentEndTicks = ref_time_ticks();

  self.finishedFrameIndex.store(front_frame);
  notify_one(self.finishedFrameIndex);
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
  D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor, const ClearDepthStencilValue &value, const eastl::optional<D3D12_RECT> &rect)
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
    value.stencil, rect ? 1 : 0, rect ? &(*rect) : nullptr);

  dirtyTextureState(image);
}

void DeviceContext::ExecutionContext::clearColorImage(Image *image, ImageViewState view, D3D12_CPU_DESCRIPTOR_HANDLE view_descriptor,
  const ClearColorValue &value, const eastl::optional<D3D12_RECT> &rect)
{
  if (!readyCommandList())
  {
    return;
  }

  disablePredication();

  contextState.resourceStates.useTextureAsRTVForClear(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, image, view);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  // Need to use shader to save integer bit pattern
  if (image->getFormat().isSampledAsFloat())
  {
    contextState.cmdBuffer.clearRenderTargetView(view_descriptor, value.float32, rect ? 1 : 0, rect ? &(*rect) : nullptr);
  }
  else
  {
    auto &frame = contextState.getFrameData();

    contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());
    auto clearPipeline = device.pipeMan.getClearPipeline(device.getDevice(), image->getFormat().asDxGiFormat(), true);

    if (!clearPipeline)
      return;

    const Extent2D dstExtent = image->getMipExtents2D(view.getMipBase());
    D3D12_RECT dstRect =
      rect ? rect.value() : D3D12_RECT{0, 0, static_cast<LONG>(dstExtent.width), static_cast<LONG>(dstExtent.height)};

    DWORD dwordRegs[4]{value.uint32[0], value.uint32[1], value.uint32[2], value.uint32[3]};
    contextState.cmdBuffer.clearExecute(device.pipeMan.getClearSignature(), clearPipeline, dwordRegs, view_descriptor, dstRect);
  }
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

void DeviceContext::ExecutionContext::resolveMultiSampleImage(Image *src, Image *dst)
{
  contextState.resourceStates.useTextureAsResolveSource(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, src);

  contextState.resourceStates.useTextureAsResolveDestination(contextState.graphicsCommandListBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, dst);

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  G_ASSERT(src->getFormat().asDxGiFormat() == dst->getFormat().asDxGiFormat());
  G_ASSERT(!src->getFormat().isDepth() || !src->isMultisampled() || d3d::get_driver_desc().caps.hasRenderPassDepthResolve);

  contextState.cmdBuffer.resolveSubresource(dst->getHandle(), 0, src->getHandle(), 0, src->getFormat().asDxGiFormat());

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
    dst_rect);

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

    D3D12_RECT dx12Area = vp.asRect();
    for (auto i : LsbVisitor{ccMask})
    {
      float cc[4];
      cc[0] = clear_color[i].r / 255.f;
      cc[1] = clear_color[i].g / 255.f;
      cc[2] = clear_color[i].b / 255.f;
      cc[3] = clear_color[i].a / 255.f;
      contextState.cmdBuffer.clearRenderTargetView(contextState.graphicsState.framebufferState.frameBufferInfo.colorAttachments[i], cc,
        1, &dx12Area);
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
  const auto &framebufferState = contextState.graphicsState.framebufferState;

  uint32_t cnt = 32 - __clz(framebufferState.framebufferLayout.colorTargetMask);

  contextState.cmdBuffer.setRenderTargets(framebufferState.frameBufferInfo.colorAttachments, cnt,
    framebufferState.frameBufferInfo.depthStencilAttachment);

  contextState.graphicsState.framebufferLayoutID = contextState.framebufferLayouts.getLayoutID(framebufferState.framebufferLayout);
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
        get_recover_behavior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
          device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
        device.shouldNameObjects(), device.pipeMan));
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

  auto devicePtr = device.device.get();

  uint32_t bindlessRev = 0;
  uint32_t bindlessCount = 0;
  if (signature.allCombinedBindlessMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
  {
    bindlessRev = contextState.bindlessSetManager.getResourceDescriptorRevision();
    bindlessCount = contextState.bindlessSetManager.getResourceDescriptorCount();
  }

  auto reservationResult = frame.resourceViewHeaps.reserveSpace(devicePtr, totalBRegisterDescriptorCount,
    totalTRegisterDescriptorCount, totalURegisterDescriptorCount, bindlessCount, bindlessRev);

  if (DescriptorReservationResult::NewHeap == reservationResult)
  {
    vsStageState.onResourceDescriptorHeapChanged();
    psStageState.onResourceDescriptorHeapChanged();
  }

  if (signature.allCombinedBindlessMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
  {
    contextState.bindlessSetManager.reserveSamplerHeap(devicePtr, frame.samplerHeaps);
  }

  psStageState.pushSamplers(devicePtr, frame.samplerHeaps, device.nullResourceTable.table[NullResourceTable::SAMPLER],
    device.nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE], psHeader.resourceUsageTable.sRegisterUseMask,
    psHeader.sRegisterCompareUseMask);

  vsStageState.pushSamplers(devicePtr, frame.samplerHeaps, device.nullResourceTable.table[NullResourceTable::SAMPLER],
    device.nullResourceTable.table[NullResourceTable::SAMPLER_COMPARE], signature.vsCombinedSRegisterMask,
    contextState.graphicsState.basePipeline->getVertexShaderSamplerCompareMask());

  psStageState.pushShaderResourceViews(devicePtr, frame.resourceViewHeaps, device.nullResourceTable,
    psHeader.resourceUsageTable.tRegisterUseMask, psHeader.tRegisterTypes);

  psStageState.pushUnorderedViews(devicePtr, frame.resourceViewHeaps, device.nullResourceTable,
    psHeader.resourceUsageTable.uRegisterUseMask, psHeader.uRegisterTypes);

  vsStageState.pushShaderResourceViews(devicePtr, frame.resourceViewHeaps, device.nullResourceTable, signature.vsCombinedTRegisterMask,
    contextState.graphicsState.basePipeline->getVertexShaderCombinedTRegisterTypes());

  vsStageState.pushUnorderedViews(devicePtr, frame.resourceViewHeaps, device.nullResourceTable, signature.vsCombinedURegisterMask,
    contextState.graphicsState.basePipeline->getVertexShaderCombinedURegisterTypes());

  auto constBufferMode = signature.def.psLayout.usesConstBufferRootDescriptors(signature.def.vsLayout, signature.def.gsLayout,
                           signature.def.hsLayout, signature.def.dsLayout)
                           ? PipelineStageStateBase::ConstantBufferPushMode::ROOT_DESCRIPTOR
                           : PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP;

  auto nullConstBufferView = device.getNullConstBufferView();

  vsStageState.pushConstantBuffers(devicePtr, frame.resourceViewHeaps, self.back.constBufferStreamDescriptors, nullConstBufferView,
    signature.vsCombinedBRegisterMask, contextState.cmdBuffer, STAGE_VS, constBufferMode, frame.frameIndex);

  psStageState.pushConstantBuffers(devicePtr, frame.resourceViewHeaps, self.back.constBufferStreamDescriptors, nullConstBufferView,
    psHeader.resourceUsageTable.bRegisterUseMask, contextState.cmdBuffer, STAGE_PS, constBufferMode, frame.frameIndex);

  for (auto s : {STAGE_VS, STAGE_PS})
  {
    contextState.stageState[s].migrateAllSamplers(devicePtr, frame.samplerHeaps);
  }

  if (signature.allCombinedBindlessMask & dxil::BINDLESS_SAMPLERS_SPACE_BITS_MASK)
  {
    contextState.bindlessSetManager.pushToSamplerHeap(devicePtr, frame.samplerHeaps);
  }

  if (signature.allCombinedBindlessMask & dxil::BINDLESS_RESOURCES_SPACE_BITS_MASK)
  {
    contextState.bindlessSetManager.pushToResourceHeap(devicePtr, frame.resourceViewHeaps);
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

      if (device.pipeMan.hasFeatureSetInCache)
        device.pipeMan.needToUpdateCache = true;

      contextState.graphicsState.pipeline->loadMesh(device.device.get(), device.pipeMan, *contextState.graphicsState.basePipeline,
        device.pipelineCache, contextState.graphicsState.statusBits.test(GraphicsState::USE_WIREFRAME), staticRenderState,
        contextState.graphicsState.framebufferState.framebufferLayout,
        get_recover_behavior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
          device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
        device.shouldNameObjects(), device.pipeMan);
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
    top = primitive_type_to_primtive_topology(contextState.graphicsState.basePipeline->getHullInputPrimitiveType(), top);
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

      if (device.pipeMan.hasFeatureSetInCache)
        device.pipeMan.needToUpdateCache = true;

      contextState.graphicsState.pipeline->load(device.device.get(), device.pipeMan, *contextState.graphicsState.basePipeline,
        device.pipelineCache, inputLayout, contextState.graphicsState.statusBits.test(GraphicsState::USE_WIREFRAME), staticRenderState,
        contextState.graphicsState.framebufferState.framebufferLayout, topType,
        get_recover_behavior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
          device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
        device.shouldNameObjects(), device.pipeMan);
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
      G_ASSERT(buffer.size <= eastl::numeric_limits<uint32_t>::max());
      D3D12_INDEX_BUFFER_VIEW view{buffer.gpuPointer, static_cast<uint32_t>(buffer.size), type};
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
  constexpr uint32_t vertexBufferDirtyMask = 1u << GraphicsState::VERTEX_BUFFER_0_DIRTY | 1u << GraphicsState::VERTEX_BUFFER_1_DIRTY |
                                             1u << GraphicsState::VERTEX_BUFFER_2_DIRTY | 1u << GraphicsState::VERTEX_BUFFER_3_DIRTY;

  constexpr uint32_t vertexBufferStateDirtyMask =
    1u << GraphicsState::VERTEX_BUFFER_STATE_0_DIRTY | 1u << GraphicsState::VERTEX_BUFFER_STATE_1_DIRTY |
    1u << GraphicsState::VERTEX_BUFFER_STATE_2_DIRTY | 1u << GraphicsState::VERTEX_BUFFER_STATE_3_DIRTY;

  const uint32_t vertexBufferBits =
    (contextState.graphicsState.statusBits.to_uint32() & vertexBufferDirtyMask) >> GraphicsState::VERTEX_BUFFER_0_DIRTY;

  // find range of which buffers we need to update
  uint32_t first = 0, last = 0, count = 0;
  if (vertexBufferBits)
  {
    first = __ctz_unsafe(vertexBufferBits);
    last = 32 - __clz_unsafe(vertexBufferBits);
    count = last - first;
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
  if (count && readyCommandList())
  {
    contextState.cmdBuffer.iaSetVertexBuffers(first, count, &views[first]);
  }

  const uint32_t vertexBufferStateBits =
    (contextState.graphicsState.statusBits.to_uint32() & vertexBufferStateDirtyMask) >> GraphicsState::VERTEX_BUFFER_STATE_0_DIRTY;

  // check buffer state and reset status bits
  for (auto i : LsbVisitor{vertexBufferStateBits})
  {
    auto &vertexBuffer = contextState.graphicsState.vertexBuffers[i];
    if (!vertexBuffer.resourceId)
      continue;
    contextState.resourceStates.useBufferAsVB(contextState.graphicsCommandListBarrierBatch, vertexBuffer);
    contextState.bufferAccessTracker.updateLastFrameAccess(vertexBuffer.resourceId);
  }

  contextState.graphicsState.statusBits &= eastl::bitset<GraphicsState::COUNT>{~(vertexBufferDirtyMask | vertexBufferStateDirtyMask)};
}

void DeviceContext::ExecutionContext::flushGraphicsStateResourceBindings()
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

void DeviceContext::ExecutionContext::setRootConstants(const unsigned stage, const RootConstatInfo &values)
{
  uint32_t offset = 0;
  for (uint32_t value : values)
  {
    if (STAGE_CS == stage)
    {
      contextState.cmdBuffer.updateComputeRootConstant(offset, value);
    }
    else if (STAGE_PS == stage)
    {
      contextState.cmdBuffer.updatePixelRootConstant(offset, value);
    }
    else if (STAGE_VS == stage)
    {
      contextState.cmdBuffer.updateVertexRootConstant(offset, value);
    }
    offset++;
  }
}

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

  auto reservationResult = frame.resourceViewHeaps.reserveSpace(device.device.get(), bRegisterCount, tRegisterCount, uRegisterCount,
    bindlessCount, bindlessRev);

  if (DescriptorReservationResult::NewHeap == reservationResult)
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
    nullConstBufferView, pipelineHeader.resourceUsageTable.bRegisterUseMask, contextState.cmdBuffer, STAGE_CS, constBufferMode,
    frame.frameIndex);

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
    DAG_FATAL("DX12: Texture read back requested on invalid queue %u", static_cast<uint32_t>(queue));
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
    D3D_ERROR("DX12: Read Back Copy out of range (skipping):");
    cpu_memory.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
    eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
    D3D_ERROR("DX12: Dest '%s' Size %u, copy range %u - %u", cbuf, infoD.Width, dstOffset, dstOffset + buffer.size);

    buffer.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
    eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
    D3D_ERROR("DX12: Source '%s' Size %u, copy range %u - %u", cbuf, infoS.Width, srcOffset, srcOffset + buffer.size);
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

void DeviceContext::ExecutionContext::createDlssFeature(bool stereo_render, int output_width, int output_height)
{
  G_UNUSED(stereo_render);

  if (!readyCommandList())
  {
    return;
  }

  // note sure if we have to, but better do it and don't worry
  disablePredication();

  self.streamlineAdapter->createDlssFeature(0, {output_width, output_height}, static_cast<void *>(contextState.cmdBuffer.getHandle()));
  if (stereo_render)
    self.streamlineAdapter->createDlssFeature(1, {output_width, output_height},
      static_cast<void *>(contextState.cmdBuffer.getHandle()));

  if (self.streamlineAdapter->isDlssGSupported() == nv::SupportState::Supported)
    self.streamlineAdapter->createDlssGFeature(0, static_cast<void *>(contextState.cmdBuffer.getHandle()));
}

void DeviceContext::ExecutionContext::releaseDlssFeature(bool stereo_render)
{
  self.streamlineAdapter->releaseDlssFeature(0);
  if (stereo_render)
    self.streamlineAdapter->releaseDlssFeature(1);
  if (self.streamlineAdapter->isDlssGSupported() == nv::SupportState::Supported)
    self.streamlineAdapter->releaseDlssGFeature(0);
}

void DeviceContext::ExecutionContext::prepareExecuteAA(std::initializer_list<Image *> inputs, std::initializer_list<Image *> outputs)
{
  // not sure if we have to, but better do it and don't worry
  disablePredication();

  for (Image *image : inputs)
  {
    if (image)
    {
      contextState.resourceStates.useTextureAsDLSSInput(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, image);
      dirtyTextureState(image);
    }
  }

  for (Image *image : outputs)
  {
    if (image)
    {
      contextState.resourceStates.useTextureAsDLSSOutput(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, image);
      dirtyTextureState(image);
    }
  }

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());

  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);
}

void DeviceContext::ExecutionContext::executeDlss(const nv::DlssParams<Image> &dlss_params, int view_index, uint32_t frame_id)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA({dlss_params.inColor, dlss_params.inDepth, dlss_params.inMotionVectors, dlss_params.inExposure},
    {dlss_params.outColor});

  auto getState = [&](Image *i) { return i ? contextState.resourceStates.currentTextureState(i, SubresourceIndex::make(0)) : 0; };

  auto convertedParams =
    nv::convertDlssParams(dlss_params, [](Image *i) { return i ? static_cast<void *>(i->getHandle()) : nullptr; });

  convertedParams.inColorState = getState(dlss_params.inColor);
  convertedParams.inDepthState = getState(dlss_params.inDepth);
  convertedParams.inExposureState = getState(dlss_params.inExposure);
  convertedParams.inMotionVectorsState = getState(dlss_params.inMotionVectors);
  convertedParams.outColorState = getState(dlss_params.outColor);

  self.streamlineAdapter->evaluateDlss(frame_id, view_index, convertedParams, contextState.cmdBuffer.getHandle());

  // DLSS alters command list state so we need to reset everything afterwards to keep consistency
  contextState.cmdBuffer.dirtyAll();
}

void DeviceContext::ExecutionContext::executeDlssG(const nv::DlssGParams<Image> &dlss_g_params, int view_index)
{
  if (!readyCommandList())
  {
    return;
  }

  // note sure if we have to, but better do it and don't worry
  disablePredication();

  prepareExecuteAA({dlss_g_params.inHUDless, dlss_g_params.inUI, dlss_g_params.inDepth, dlss_g_params.inMotionVectors}, {});

  auto convertedParams =
    nv::convertDlssGParams(dlss_g_params, [](Image *i) { return i ? static_cast<void *>(i->getHandle()) : nullptr; });

  auto getState = [&](Image *i) { return i ? contextState.resourceStates.currentTextureState(i, SubresourceIndex::make(0)) : 0; };

  convertedParams.inHUDlessState = getState(dlss_g_params.inHUDless);
  convertedParams.inUIState = getState(dlss_g_params.inUI);
  convertedParams.inDepthState = getState(dlss_g_params.inDepth);
  convertedParams.inMotionVectorsState = getState(dlss_g_params.inMotionVectors);

  self.streamlineAdapter->evaluateDlssG(view_index, convertedParams, contextState.cmdBuffer.getHandle());

  // DLSS alters command list state so we need to reset everything afterwards to keep consistency
  contextState.cmdBuffer.dirtyAll();
}

void DeviceContext::ExecutionContext::executeXess(const XessParamsDx12 &params)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA({params.inColor, params.inDepth, params.inMotionVectors}, {params.outColor});

  self.xessWrapper.evaluateXess(static_cast<void *>(contextState.cmdBuffer.getHandle()), static_cast<const void *>(&params));

  contextState.cmdBuffer.dirtyAll();
}

void DeviceContext::ExecutionContext::executeFSR2(const Fsr2ParamsDx12 &params)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA({params.inColor, params.inDepth, params.inMotionVectors}, {params.outColor});
  self.fsr2Wrapper.evaluateFsr2(static_cast<void *>(contextState.cmdBuffer.getHandle()), static_cast<const void *>(&params));
  contextState.cmdBuffer.dirtyAll();
}

void DeviceContext::ExecutionContext::executeFSR(amd::FSR *fsr, const FSRUpscalingArgs &params)
{
  if (!readyCommandList())
  {
    return;
  }

  prepareExecuteAA(
    {
      params.colorTexture,
      params.depthTexture,
      params.motionVectors,
      params.exposureTexture,
      params.reactiveTexture,
      params.transparencyAndCompositionTexture,
    },
    {});

  auto cast_to_resource = [](Image *src) {
    if (src)
      return src->getHandle();
    return (ID3D12Resource *)nullptr;
  };

  amd::FSR::UpscalingPlatformArgs args = params;
  args.colorTexture = cast_to_resource(params.colorTexture);
  args.depthTexture = cast_to_resource(params.depthTexture);
  args.motionVectors = cast_to_resource(params.motionVectors);
  args.exposureTexture = cast_to_resource(params.exposureTexture);
  args.outputTexture = cast_to_resource(params.outputTexture);
  args.reactiveTexture = cast_to_resource(params.reactiveTexture);
  args.transparencyAndCompositionTexture = cast_to_resource(params.transparencyAndCompositionTexture);

  fsr->doApplyUpscaling(args, contextState.cmdBuffer.getHandle());
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
      D3D_ERROR("DX12: Upload Copy out of range (skipping):");
      target.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
      eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
      D3D_ERROR("DX12: Dest '%s' Size %u, copy range %u - %u", cbuf, infoD.Width, dstOffset, dstOffset + target.size);

      source.buffer->GetPrivateData(WKPDID_D3DDebugObjectNameW, &cnt, wcbuf);
      eastl::copy(wcbuf, wcbuf + cnt / sizeof(wchar_t), cbuf);
      D3D_ERROR("DX12: Source '%s' Size %u, copy range %u - %u", cbuf, infoS.Width, srcOffset, srcOffset + target.size);
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
  ValueRange<ExtendedImageGlobalSubresourceId> id_range)
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
    make_resource_barrier_string_from_state(maskNameBuffer, countof(maskNameBuffer), state, barrier);
    logdbg("DX12: Resource barrier for buffer %s - %p, with %s, during %s", get_resource_name(buffer.buffer, cbuf), buffer.buffer,
      maskNameBuffer, getEventPath());
  }
#endif

  if (RB_NONE != ((RB_FLUSH_UAV | RB_FLUSH_RAYTRACE_ACCELERATION_BUILD_SCRATCH_USE) & barrier))
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
        D3D_ERROR("DX12: RB_ALIAS_TO_AND_DISCARD used with null resource!");
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
    make_resource_barrier_string_from_state(maskNameBuffer, countof(maskNameBuffer), state, barrier);
    logdbg("DX12: Resource barrier for texture %s - %p [%u - %u], with %s, during %s", static_cast<uint32_t>(barrier),
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

#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::asBarrier(RaytraceAccelerationStructure *as, GpuPipeline)
{
  contextState.graphicsCommandListBarrierBatch.flushUAV(as->asHeapResource);
}
#endif

#if _TARGET_XBOX
void DeviceContext::ExecutionContext::enterSuspendState()
{
  logdbg("DX12: Engine requested to suspend...");
  logdbg("DX12: Entered suspended state, notifying engine and waiting for resume "
         "request...");
  // Acknowledge that we are now in suspended state and wait for resume signal
  SignalObjectAndWait(self.enteredSuspendedStateEvent.get(), self.resumeExecutionEvent.get(), INFINITE, FALSE);
  logdbg("DX12: Engine requested to resume...");
}
#endif

void DeviceContext::ExecutionContext::writeDebugMessage(StringIndexRef::RangeType message, int severity)
{
  if (severity < 1)
  {
    logdbg(message.data());
  }
  else if (severity < 2)
  {
    logwarn(message.data());
  }
  else
  {
    D3D_ERROR(message.data());
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
  logdbg("DX12: onDeviceError has been triggered");
  contextState.debugOnDeviceRemoved(device, device.device.get(), remove_reason);

  contextState.activeReadBackCommandList.reset();
  contextState.activeEarlyUploadCommandList.reset();
  contextState.activeLateUploadCommandList.reset();
  contextState.cmdBuffer.prepareBufferForSubmit();
  contextState.cmdBuffer.resetBuffer();

  // increment progress to break up link to previous frames
  ++self.back.frameProgress;
  logdbg("DX12: onDeviceError finished");
}

void DeviceContext::ExecutionContext::onSwapchainSwapCompleted() { self.back.swapchain.onFrameBegin(device.device.get()); }

void DeviceContext::ExecutionContext::commandFence(std::atomic_bool &signal)
{
  signal.store(true, std::memory_order_relaxed);
  ::notify_all(signal);
}

#endif

#if _TARGET_PC_WIN
void DeviceContext::ExecutionContext::beginTileMapping(Image *image, ID3D12Heap *heap, size_t heap_base, size_t mapping_count)
{
  device.queues[DeviceQueueType::GRAPHICS].beginTileMapping(image->getHandle(), heap, heap_base, mapping_count);
}

#else

void DeviceContext::ExecutionContext::beginTileMapping(Image *image, uintptr_t address, uint64_t size, size_t mapping_count)
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
    frame.resourceViewHeaps, device.pipeMan, contextState.cmdBuffer);
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

void DeviceContext::ExecutionContext::twoPhaseCopyBuffer(BufferResourceReferenceAndOffset source, uint64_t destination_offset,
  ScratchBuffer scratch_memory, uint64_t data_size)
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

void DeviceContext::ExecutionContext::moveBuffer(BufferResourceReferenceAndOffset from, BufferResourceReferenceAndRange to)
{
  if (!readyCommandList())
    return;

#if !DX12_USE_AUTO_PROMOTE_AND_DECAY
  contextState.resourceStates.resetBufferState(contextState.graphicsCommandListBarrierBatch, from.buffer, from.resourceId);
#endif

  contextState.resourceStates.useBufferAsCopySource(contextState.graphicsCommandListBarrierBatch, from);
  contextState.resourceStates.useBufferAsCopyDestination(contextState.graphicsCommandListBarrierBatch, to);
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  // moves are executed on the read back queue for now
  if (readyReadBackCommandList())
    contextState.activeReadBackCommandList.copyBufferRegion(to.buffer, to.offset, from.buffer, from.offset, to.size);
  else
    contextState.cmdBuffer.copyBuffer(to.buffer, to.offset, from.buffer, from.offset, to.size);
}

void DeviceContext::ExecutionContext::moveTexture(Image *from, Image *to)
{
  if (!readyReadBackCommandList())
    return;

  contextState.resourceStates.useTextureAsCopyDestinationForWholeCopyOnReadbackQueue(contextState.readbackBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, to);
  contextState.resourceStates.useTextureAsCopySourceForWholeCopyOnReadbackQueue(contextState.readbackBarrierBatch,
    contextState.graphicsCommandListSplitBarrierTracker, from);

  // Using multiple copyTextureRegion instead of copyResource is a workaround for an
  // AMD RX 5700 XT issue, where copyResource for a heavy multidimensional resource may cause device hang.
#if _TARGET_PC_WIN
  if (d3d_get_gpu_cfg().primaryVendor == D3D_VENDOR_AMD)
  {
    for (const auto subres : from->getSubresourceRange())
    {
      D3D12_TEXTURE_COPY_LOCATION copySrc{};
      copySrc.pResource = from->getHandle();
      copySrc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      copySrc.SubresourceIndex = subres.index();

      D3D12_TEXTURE_COPY_LOCATION copyDst{};
      copyDst.pResource = to->getHandle();
      copyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
      copyDst.SubresourceIndex = subres.index();

      contextState.activeReadBackCommandList.copyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);
    }
  }
  else
#endif
  {
    contextState.activeReadBackCommandList.copyResource(to->getHandle(), from->getHandle());
  }

  // `from` is going to be removed, so we can safely update the last frame access for `to` texture only.
  // This tracking is required to avoid reordering with the next frame early upload commands.
  to->updateLastFrameAccess(self.back.frameProgress);

  dirtyTextureState(to);
  dirtyTextureState(from);
}

void DeviceContext::ExecutionContext::transitionBuffer(BufferResourceReference buffer, D3D12_RESOURCE_STATES state)
{
  if (!readyCommandList())
  {
    return;
  }

  contextState.resourceStates.transitionBufferExplicit(contextState.graphicsCommandListBarrierBatch, buffer, state);
}

void DeviceContext::ExecutionContext::resizeImageMipMapTransfer(Image *src, Image *dst, MipMapRange mip_map_range,
  uint32_t src_mip_map_offset, uint32_t dst_mip_map_offset)
{
  if (!src || !dst)
  {
    G_ASSERT_LOG(!device.isHealthy(), "DX12: Image is null but device is not lost");
    return;
  }

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
        D3D_ERROR("DX12: resizeImageMipMapTransfer of <%s> from %p[%u] to %p[%u] mip-map level sizes "
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

  G_ASSERTF_RETURN(contextState.graphicsState.basePipeline->isMesh(), ,
    "DX12: skipped dispatchMesh: graphics pipeline has no mesh shader");

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

  G_ASSERTF_RETURN(contextState.graphicsState.basePipeline->isMesh(), ,
    "DX12: skipped dispatchMeshIndirect: graphics pipeline has no mesh shader");

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

void DeviceContext::ExecutionContext::addShaderGroup(uint32_t group, ScriptedShadersBinDumpOwner *dump, ShaderID null_pixel_shader,
  StringIndexRef::RangeType name)
{
  device.pipeMan.addShaderGroup(device.getDevice(), &device.pipelineCache, &contextState.framebufferLayouts, group, dump,
    null_pixel_shader, {name.data(), name.size()});
}

void DeviceContext::ExecutionContext::removeShaderGroup(uint32_t group)
{
  device.pipeMan.removeShaderGroup(
    contextState.framebufferLayouts, group,
    [this](ProgramID program) { contextState.getFrameData().deletedPrograms.push_back(program); },
    [this](GraphicsProgramID program) { contextState.getFrameData().deletedGraphicPrograms.push_back(program); });
}

void DeviceContext::ExecutionContext::loadComputeShaderFromDump(ProgramID program)
{
  device.pipeMan.loadComputeShaderFromDump(device.device.get(), device.pipelineCache, program,
    get_recover_behavior_from_cfg(device.config.features.test(DeviceFeaturesConfig::PIPELINE_COMPILATION_ERROR_IS_FATAL),
      device.config.features.test(DeviceFeaturesConfig::ASSERT_ON_PIPELINE_COMPILATION_ERROR)),
    device.shouldNameObjects(), CSPreloaded::Yes);
}

bool DeviceContext::ExecutionContext::should_pipeline_set_compilation_spread_over_frames()
{
  return dgs_get_settings()->getBlockByNameEx("dx12")->getBool("spreadCompilationOverFrames", true);
}

void DeviceContext::ExecutionContext::compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
  DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
  DynamicArray<GraphicsPipelinePreloadInfo> &&graphics_pipelines, DynamicArray<MeshPipelinePreloadInfo> &&mesh_pipelines,
  DynamicArray<ComputePipelinePreloadInfo> &&compute_pipelines)
{
  logdbg("DX12: compilePipelineSet started");
  device.pipeMan.storePipelineSetForCompilation(eastl::forward<DynamicArray<InputLayoutIDWithHash>>(input_layouts),
    eastl::forward<DynamicArray<StaticRenderStateIDWithHash>>(static_render_states),
    eastl::forward<DynamicArray<FramebufferLayoutWithHash>>(framebuffer_layouts),
    eastl::forward<DynamicArray<GraphicsPipelinePreloadInfo>>(graphics_pipelines),
    eastl::forward<DynamicArray<MeshPipelinePreloadInfo>>(mesh_pipelines),
    eastl::forward<DynamicArray<ComputePipelinePreloadInfo>>(compute_pipelines));
  if (!should_pipeline_set_compilation_spread_over_frames())
  {
    device.pipeMan.continuePipelineSetCompilation(device.getDevice(), device.pipelineCache, contextState.framebufferLayouts,
      []() { return false; });
  }
}

void DeviceContext::ExecutionContext::compilePipelineSet(DynamicArray<InputLayoutIDWithHash> &&input_layouts,
  DynamicArray<StaticRenderStateIDWithHash> &&static_render_states, DynamicArray<FramebufferLayoutWithHash> &&framebuffer_layouts,
  DynamicArray<cacheBlk::SignatureEntry> &&scripted_shader_dump_signature, DynamicArray<cacheBlk::ComputeClassUse> &&compute_pipelines,
  DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_pipelines,
  DynamicArray<cacheBlk::GraphicsVariantGroup> &&graphics_with_null_override_pipelines, ShaderID null_pixel_shader)
{
  logdbg("DX12: compilePipelineSet2 started");
  device.pipeMan.storePipelineSetForCompilation2(eastl::forward<DynamicArray<InputLayoutIDWithHash>>(input_layouts),
    eastl::forward<DynamicArray<StaticRenderStateIDWithHash>>(static_render_states),
    eastl::forward<DynamicArray<FramebufferLayoutWithHash>>(framebuffer_layouts),
    eastl::forward<DynamicArray<cacheBlk::SignatureEntry>>(scripted_shader_dump_signature),
    eastl::forward<DynamicArray<cacheBlk::ComputeClassUse>>(compute_pipelines),
    eastl::forward<DynamicArray<cacheBlk::GraphicsVariantGroup>>(graphics_pipelines),
    eastl::forward<DynamicArray<cacheBlk::GraphicsVariantGroup>>(graphics_with_null_override_pipelines), null_pixel_shader,
    device.shouldNameObjects());
  if (!should_pipeline_set_compilation_spread_over_frames())
  {
    device.pipeMan.continuePipelineSetCompilation(device.getDevice(), device.pipelineCache, contextState.framebufferLayouts,
      []() { return false; });
  }
}

void DeviceContext::ExecutionContext::switchActivePipeline(ActivePipeline pipeline) { contextState.switchActivePipeline(pipeline); }

#if D3D_HAS_RAY_TRACING
void DeviceContext::ExecutionContext::applyRaytraceState(const RayDispatchBasicParameters &dispatch_parameters,
  const ResourceBindingTable &rbt, UInt32ListRef::RangeType root_constants)
{
  if (!readyCommandList())
  {
    return;
  }

  auto &frame = contextState.getFrameData();

  auto constBufferMode = dispatch_parameters.rootSignature->def.csLayout.usesConstBufferRootDescriptors()
                           ? PipelineStageStateBase::ConstantBufferPushMode::ROOT_DESCRIPTOR
                           : PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP;

  // TODO needs only top be done if the shaders use bindless
  uint32_t bindlessCount = contextState.bindlessSetManager.getResourceDescriptorCount();
  uint32_t bindlessRev = contextState.bindlessSetManager.getResourceDescriptorRevision();

  frame.resourceViewHeaps.reserveSpace(device.device.get(), rbt.contBufferCount, rbt.readCount, rbt.writeCount, bindlessCount,
    bindlessRev);

  // TODO needs only top be done if the shaders use bindless
  contextState.bindlessSetManager.reserveSamplerHeap(device.device.get(), frame.samplerHeaps);

  auto sRegisterDescriptorIndex = frame.samplerHeaps.findInScratchSegment(rbt.samplers, rbt.samplerCount);
  if (!sRegisterDescriptorIndex)
  {
    frame.samplerHeaps.ensureScratchSegmentSpace(device.device.get(), rbt.samplerCount);
    sRegisterDescriptorIndex = frame.samplerHeaps.appendToScratchSegment(device.device.get(), rbt.samplers, rbt.samplerCount);
  }
  auto sRegisterDescriptorRange = DescriptorHeapRange::make(sRegisterDescriptorIndex, rbt.samplerCount);

  DescriptorHeapIndex tRegisterDescriptorIndex;
#if DX12_REUSE_SHADER_RESOURCE_VIEW_DESCRIPTOR_RANGES
  if (frame.resourceViewHeaps.highSRVScratchSegmentUsage())
  {
    tRegisterDescriptorIndex = frame.resourceViewHeaps.findInSRVScratchSegment(rbt.readResourceViews, rbt.readCount);
  }
  else
  {
    tRegisterDescriptorIndex = DescriptorHeapIndex::make_invalid();
  }
  if (!tRegisterDescriptorIndex)
#endif
  {
    tRegisterDescriptorIndex =
      frame.resourceViewHeaps.appendToSRVScratchSegment(device.device.get(), rbt.readResourceViews, rbt.readCount);
  }
  auto tRegisterDescriptorRange = DescriptorHeapRange::make(tRegisterDescriptorIndex, rbt.readCount);

  DescriptorHeapIndex uRegisterDescriptorIndex;
#if DX12_REUSE_UNORDERD_ACCESS_VIEW_DESCRIPTOR_RANGES
  uRegisterDescriptorIndex = frame.resourceViewHeaps.findInUAVScratchSegment(rbt.writeResourceViews, rbt.writeCount);
  if (!uRegisterDescriptorIndex)
#endif
  {
    uRegisterDescriptorIndex =
      frame.resourceViewHeaps.appendToUAVScratchSegment(device.device.get(), rbt.writeResourceViews, rbt.writeCount);
  }
  auto uRegisterDescriptorRange = DescriptorHeapRange::make(uRegisterDescriptorIndex, rbt.writeCount);

  // TODO needs only top be done if the shaders use bindless
  contextState.bindlessSetManager.pushToSamplerHeap(device.device.get(), frame.samplerHeaps);
  contextState.bindlessSetManager.pushToResourceHeap(device.device.get(), frame.resourceViewHeaps);

  contextState.cmdBuffer.bindRaytracePipeline(dispatch_parameters.rootSignature, dispatch_parameters.pipeline);

  DescriptorHeapRange bRegisterDescribtorRange;
  if (PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP == constBufferMode)
  {
    const auto base = self.back.constBufferStreamDescriptors.getDescriptors(device.device.get(), rbt.contBufferCount);
    const auto width = self.back.constBufferStreamDescriptors.getDescriptorSize();
    auto pos = base;

    for (uint32_t i = 0; i < rbt.contBufferCount; ++i)
    {
      D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc;
      viewDesc.BufferLocation = rbt.constBuffers[i].gpuPointer;
      viewDesc.SizeInBytes = rbt.constBuffers[i].size;
      device.device->CreateConstantBufferView(&viewDesc, pos);
      pos.ptr += width;
    }

    auto index = frame.resourceViewHeaps.appendToConstScratchSegment(device.device.get(), base, rbt.contBufferCount);
    bRegisterDescribtorRange = DescriptorHeapRange::make(index, rbt.contBufferCount);
  }
  else
  {
    for (uint32_t i = 0; i < rbt.contBufferCount; ++i)
    {
      contextState.cmdBuffer.setRaytraceConstantBuffer(i, rbt.constBuffers[i].gpuPointer);
    }
  }

  sRegisterDescriptorRange = frame.samplerHeaps.migrateToActiveScratchSegment(device.device.get(), sRegisterDescriptorRange);

  contextState.cmdBuffer.setSamplerHeap(frame.samplerHeaps.getActiveHandle(), frame.samplerHeaps.getBindlessGpuAddress());
  contextState.cmdBuffer.setResourceHeap(frame.resourceViewHeaps.getActiveHandle(), frame.resourceViewHeaps.getBindlessGpuAddress());

  contextState.cmdBuffer.setRaytraceSamplers(frame.samplerHeaps.getGpuAddress(sRegisterDescriptorRange));
  contextState.cmdBuffer.setRaytraceSRVs(frame.resourceViewHeaps.getGpuAddress(tRegisterDescriptorRange));
  contextState.cmdBuffer.setRaytraceUAVs(frame.resourceViewHeaps.getGpuAddress(uRegisterDescriptorRange));

#if DX12_ENABLE_CONST_BUFFER_DESCRIPTORS
  if (PipelineStageStateBase::ConstantBufferPushMode::DESCRIPTOR_HEAP == constBufferMode)
  {
    contextState.cmdBuffer.setRaytraceConstantBufferDescriptors(frame.resourceViewHeaps.getGpuAddress(bRegisterDescribtorRange));
  }
#endif

  // NOTE that for state, STAGE_CS is correct here as RT is executed on the CS unit on the queue.
  for (uint32_t i = 0; i < rbt.contBufferCount; ++i)
  {
    contextState.resourceStates.useBufferAsCBV(contextState.graphicsCommandListBarrierBatch, STAGE_CS, rbt.constBuffers[i]);
  }

  for (uint32_t i = 0; i < rbt.readCount; ++i)
  {
    auto &srv = rbt.readResources[i];
    if (srv.image)
    {
      contextState.resourceStates.useTextureAsSRV(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, STAGE_CS, srv.image, srv.imageView, srv.isConstDepthRead);
    }
    else if (srv.bufferRef)
    {
      contextState.resourceStates.useBufferAsSRV(contextState.graphicsCommandListBarrierBatch, STAGE_CS, srv.bufferRef);
    }
    // RT structures do not need any state tracking
  }

  for (uint32_t i = 0; i < rbt.writeCount; ++i)
  {
    auto &uav = rbt.writeResources[i];
    if (uav.image)
    {
      contextState.resourceStates.useTextureAsUAV(contextState.graphicsCommandListBarrierBatch,
        contextState.graphicsCommandListSplitBarrierTracker, STAGE_CS, uav.image, uav.imageView);
      contextState.resourceStates.beginImplicitUAVAccess(uav.image->getHandle());
    }
    else if (uav.bufferRef)
    {
      contextState.resourceStates.useBufferAsUAV(contextState.graphicsCommandListBarrierBatch, STAGE_CS, uav.bufferRef);
      contextState.resourceStates.beginImplicitUAVAccess(uav.bufferRef.buffer);
    }
  }

  for (uint32_t i = 0; i < root_constants.size(); ++i)
  {
    contextState.cmdBuffer.updateRaytraceRootConstant(i, root_constants[i]);
  }
}

void DeviceContext::ExecutionContext::dispatchRays(const RayDispatchBasicParameters &dispatch_parameters,
  const ResourceBindingTable &rbt, UInt32ListRef::RangeType root_constants, const RayDispatchParameters &rdp)
{
  if (!readyCommandList())
  {
    return;
  }

  switchActivePipeline(ActivePipeline::Raytrace);

  applyRaytraceState(dispatch_parameters, rbt, root_constants);

  tranistionPredicationBuffer();

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  applyPredicationBuffer();

  contextState.cmdBuffer.traceRays(rdp.rayGenTable.gpuPointer, rdp.rayGenTable.size, rdp.missTable.gpuPointer, rdp.missTable.size,
    rdp.missStride, rdp.hitTable.gpuPointer, rdp.hitTable.size, rdp.hitStride, rdp.callableTable.gpuPointer, rdp.callableTable.size,
    rdp.callableStride, rdp.width, rdp.height, rdp.depth);

  contextState.debugDispatchRays(device, contextState.cmdBuffer.getHandle(), dispatch_parameters, rbt, rdp);
}

void DeviceContext::ExecutionContext::dispatchRaysIndirect(const RayDispatchBasicParameters &dispatch_parameters,
  const ResourceBindingTable &rbt, UInt32ListRef::RangeType root_constants, const RayDispatchIndirectParameters &rdip)
{
  if (!readyCommandList())
  {
    return;
  }

  switchActivePipeline(ActivePipeline::Raytrace);

  applyRaytraceState(dispatch_parameters, rbt, root_constants);

  contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, rdip.argumentBuffer);
  contextState.bufferAccessTracker.updateLastFrameAccess(rdip.argumentBuffer.resourceId);

  if (rdip.countBuffer)
  {
    contextState.resourceStates.useBufferAsIA(contextState.graphicsCommandListBarrierBatch, rdip.countBuffer);
    contextState.bufferAccessTracker.updateLastFrameAccess(rdip.countBuffer.resourceId);
  }

  tranistionPredicationBuffer();

  contextState.resourceStates.flushPendingUAVActions(contextState.graphicsCommandListBarrierBatch, device.currentEventPath().data(),
    device.validatesUserBarriers());
  contextState.graphicsCommandListBarrierBatch.execute(contextState.cmdBuffer);

  applyPredicationBuffer();

  auto indirectSignature = contextState.dispatchRaySignatures.getSignatureForStride(device.device.get(), rdip.argumentStrideInBytes,
    D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS);
  if (!indirectSignature)
  {
    // error while trying to create a signature
    return;
  }

  contextState.cmdBuffer.dispatchaysIndirect(indirectSignature, rdip.argumentBuffer.buffer, rdip.argumentBuffer.offset,
    rdip.countBuffer.buffer, rdip.countBuffer.offset, rdip.maxCount);

  contextState.debugDispatchRaysIndirect(device, contextState.cmdBuffer.getHandle(), dispatch_parameters, rbt, rdip);
}
#endif
