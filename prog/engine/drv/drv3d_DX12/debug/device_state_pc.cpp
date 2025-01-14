// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_state_pc.h"
#include "global_state.h"

#include <validationLayer.h>
#if USE_PIX
#if _TARGET_64BIT
// PROFILE_BUILD will enable USE_PIX in pix3.h if architecture is supported
#define PROFILE_BUILD
#if !defined(__d3d12_h__)
#define __d3d12_h__
#include <WinPixEventRuntime/pix3.h>
#undef __d3d12_h__
#else
#include <WinPixEventRuntime/pix3.h>
#endif
#endif
#endif


#if COMMAND_BUFFER_DEBUG_INFO_DEFINED

namespace
{
void __stdcall process_debug_log(D3D12_MESSAGE_CATEGORY category, D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id,
  LPCSTR description, void *context)
{
  G_UNUSED(category);
  G_UNUSED(context);
  switch (severity)
  {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    case D3D12_MESSAGE_SEVERITY_ERROR: D3D_ERROR("DX12: [#%u] %s", id, description); break;
    case D3D12_MESSAGE_SEVERITY_WARNING: logwarn("DX12: [#%u] %s", id, description); break;
    case D3D12_MESSAGE_SEVERITY_INFO:
    case D3D12_MESSAGE_SEVERITY_MESSAGE: logdbg("DX12: [#%u] %s", id, description); break;
  }
}

void unregister_message_callback(ID3D12InfoQueue *debug_queue, DWORD &callback_cookie)
{
  ComPtr<ID3D12InfoQueue1> debugQueue1;
  if (callback_cookie && SUCCEEDED(debug_queue->QueryInterface(COM_ARGS(&debugQueue1))))
  {
    debugQueue1->UnregisterMessageCallback(callback_cookie);
  }
  callback_cookie = 0;
}
} // namespace


namespace drv3d_dx12::debug::pc
{
bool DeviceState::setup(GlobalState &global, ID3D12Device *device, const Direct3D12Enviroment &d3d_env)
{
  globalState = &global;
  globalState->postmortemTrace().setupDevice(device, global.configuration(), d3d_env);

  logdbg("DX12: Trying to acquire debug message log...");
  if (SUCCEEDED(device->QueryInterface(COM_ARGS(&debugQueue))))
  {
    // TODO: make this setup configurable
    logdbg("DX12: Success, reporting queued debug messages on frame end and reset errors");
    D3D12_INFO_QUEUE_FILTER defaultFilter{};
    // on default we don't care about messages and info
    D3D12_MESSAGE_SEVERITY denySeverity[] = //
      {D3D12_MESSAGE_SEVERITY_MESSAGE, D3D12_MESSAGE_SEVERITY_INFO};
    auto denyId =
      get_ignored_validation_messages<D3D12_MESSAGE_ID>(*::dgs_get_settings()->getBlockByNameEx("dx12")->getBlockByNameEx("debug"));
    defaultFilter.DenyList.pSeverityList = denySeverity;
    defaultFilter.DenyList.NumSeverities = static_cast<UINT>(countof(denySeverity));
    defaultFilter.DenyList.pIDList = denyId.data();
    defaultFilter.DenyList.NumIDs = static_cast<UINT>(denyId.size());
    debugQueue->AddRetrievalFilterEntries(&defaultFilter);
    debugQueue->AddStorageFilterEntries(&defaultFilter);
    ComPtr<ID3D12InfoQueue1> debugQueue1;
    if (SUCCEEDED(debugQueue->QueryInterface(COM_ARGS(&debugQueue1))))
    {
      debugQueue1->RegisterMessageCallback(&::process_debug_log, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &callbackCookie);
    }
  }
  else
  {
    if (global.configuration().enableCPUValidation)
    {
      G_ASSERTF(false, "DX12: User requested debug layer to be enabled, but it is not available.");
    }
    logdbg("DX12: Failed, no debug messages are reported to the log...");
  }

  return static_cast<bool>(debugQueue);
}

void DeviceState::teardown()
{
  unregister_message_callback(debugQueue.Get(), callbackCookie);
  debugQueue.Reset();

  globalState->postmortemTrace().onDeviceShutdown();
}

void DeviceState::beginCommandBuffer(D3DDevice *device, ID3D12GraphicsCommandList *cmd)
{
  globalState->postmortemTrace().beginCommandBuffer(device, cmd);
}

void DeviceState::endCommandBuffer(ID3D12GraphicsCommandList *cmd) { globalState->postmortemTrace().endCommandBuffer(cmd); }

void DeviceState::beginSection(ID3D12GraphicsCommandList *cmd, eastl::string_view text)
{
  auto eventText = event_marker::Tracker::beginEvent(text);
  globalState->captureTool().beginEvent(cmd, eventText);
  globalState->postmortemTrace().beginEvent(cmd, eventText, currentEventPath());
}

void DeviceState::endSection(ID3D12GraphicsCommandList *cmd)
{
  event_marker::Tracker::endEvent();
  globalState->captureTool().endEvent(cmd);
  globalState->postmortemTrace().endEvent(cmd, currentEventPath());
}

void DeviceState::marker(ID3D12GraphicsCommandList *cmd, eastl::string_view text)
{
  auto markerText = event_marker::Tracker::marker(text);
  globalState->captureTool().marker(cmd, markerText);
  globalState->postmortemTrace().marker(cmd, markerText);
}

void DeviceState::draw(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
  uint32_t start, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  globalState->postmortemTrace().draw(debug_info, cmd, vs, ps, pipeline_base, pipeline, count, instance_count, start, first_instance,
    topology);
}

void DeviceState::drawIndexed(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &vs,
  const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline, uint32_t count, uint32_t instance_count,
  uint32_t index_start, int32_t vertex_base, uint32_t first_instance, D3D12_PRIMITIVE_TOPOLOGY topology)
{
  globalState->postmortemTrace().drawIndexed(debug_info, cmd, vs, ps, pipeline_base, pipeline, count, instance_count, index_start,
    vertex_base, first_instance, topology);
}

void DeviceState::drawIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  globalState->postmortemTrace().drawIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, buffer);
}

void DeviceState::drawIndexedIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &buffer)
{
  globalState->postmortemTrace().drawIndexedIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, buffer);
}

void DeviceState::dispatchIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &state, ComputePipeline &pipeline, const BufferResourceReferenceAndOffset &buffer)
{
  globalState->postmortemTrace().dispatchIndirect(debug_info, cmd, state, pipeline, buffer);
}

void DeviceState::dispatch(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd, const PipelineStageStateBase &state,
  ComputePipeline &pipeline, uint32_t x, uint32_t y, uint32_t z)
{
  globalState->postmortemTrace().dispatch(debug_info, cmd, state, pipeline, x, y, z);
}

void DeviceState::dispatchMesh(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  uint32_t x, uint32_t y, uint32_t z)
{
  globalState->postmortemTrace().dispatchMesh(debug_info, cmd, vs, ps, pipeline_base, pipeline, x, y, z);
}

void DeviceState::dispatchMeshIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const PipelineStageStateBase &vs, const PipelineStageStateBase &ps, BasePipeline &pipeline_base, PipelineVariant &pipeline,
  const BufferResourceReferenceAndOffset &args, const BufferResourceReferenceAndOffset &count, uint32_t max_count)
{
  globalState->postmortemTrace().dispatchMeshIndirect(debug_info, cmd, vs, ps, pipeline_base, pipeline, args, count, max_count);
}

void DeviceState::blit(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd)
{
  globalState->postmortemTrace().blit(debug_info, cmd);
}

#if D3D_HAS_RAY_TRACING
void DeviceState::dispatchRays(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchParameters &rdp)
{
  globalState->postmortemTrace().dispatchRays(debug_info, cmd, dispatch_parameters, rbt, rdp);
}

void DeviceState::dispatchRaysIndirect(const call_stack::CommandData &debug_info, D3DGraphicsCommandList *cmd,
  const RayDispatchBasicParameters &dispatch_parameters, const ResourceBindingTable &rbt, const RayDispatchIndirectParameters &rdip)
{
  globalState->postmortemTrace().dispatchRaysIndirect(debug_info, cmd, dispatch_parameters, rbt, rdip);
}
#endif

void DeviceState::onDeviceRemoved(D3DDevice *device, HRESULT remove_reason)
{
  if (DXGI_ERROR_INVALID_CALL == remove_reason)
  {
    logdbg("DX12: Removed reason was DXGI_ERROR_INVALID_CALL, no postmortem data collected. "
           "Application should be run with debugLevel set to 2, for CPU validation.");
  }

  globalState->postmortemTrace().onDeviceRemoved(device, remove_reason, *this);
}

void DeviceState::preRecovery()
{
  globalState->postmortemTrace().teardown();
  unregister_message_callback(debugQueue.Get(), callbackCookie);
  debugQueue.Reset();
}

void DeviceState::recover(ID3D12Device *device, const Direct3D12Enviroment &d3d_env) { setup(*globalState, device, d3d_env); }

void DeviceState::beginCapture(const wchar_t *name) { globalState->captureTool().beginCapture(name); }

void DeviceState::endCapture() { globalState->captureTool().endCapture(); }

void DeviceState::captureNextFrames(const wchar_t *filename, int count) { globalState->captureTool().captureFrames(filename, count); }

void DeviceState::handlePresentToPresentCapture() { globalState->captureTool().onPresent(); }

void DeviceState::sendGPUCrashDump(const char *type, const void *data, uintptr_t size)
{
  if (!globalState->postmortemTrace().sendGPUCrashDump(type, data, size))
  {
    D3D_ERROR("DX12: Tried to send unrecognized GPU crash dump type <%s>", type);
  }
}

void DeviceState::processDebugLogImpl()
{
  if (callbackCookie)
    return;

  // ClearStoredMessages can corrupt the internal counter and GetNumStoredMessages can return 0xffffffffffffffff
  int64_t numStoredMessages = (int64_t)debugQueue->GetNumStoredMessages();

  for (int64_t i = 0; i < numStoredMessages; ++i)
  {
    SIZE_T sz = 0;
    debugQueue->GetMessage(i, nullptr, &sz);
    debugMessageBuffer.resize(static_cast<size_t>(sz));
    if (sz < sizeof(D3D12_MESSAGE))
      continue;
    auto msg = reinterpret_cast<D3D12_MESSAGE *>(debugMessageBuffer.data());
    debugQueue->GetMessage(i, msg, &sz);
    ::process_debug_log(D3D12_MESSAGE_CATEGORY_APPLICATION_DEFINED, msg->Severity, msg->ID, msg->pDescription, nullptr);
  }
  debugQueue->ClearStoredMessages();
}
} // namespace drv3d_dx12::debug::pc

#endif // COMMAND_BUFFER_DEBUG_INFO_DEFINED
