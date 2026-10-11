// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "gpu_postmortem_microsoft_dred.h"
#include "configuration.h"
#include "names.h"
#include <platform.h>

#include <osApiWrappers/dag_unicode.h>

#include <cinttypes>
#include <cwchar>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#if USE_PIX
#include <pix3.h> // not self-contained, driver.h has to come first
#endif


// UTF-16 to UTF-8: one wchar_t converts to at most 3 bytes
using DebugNameBuffer = char[drv3d_dx12::MAX_OBJECT_NAME_LENGTH * 3];

namespace
{
const char *debug_name(const char *a_str, const wchar_t *w_str, DebugNameBuffer &buf)
{
  if (a_str)
    return a_str;
  if (!w_str)
    return "<unnamed>";
  if (wcs_to_utf8(w_str, buf, sizeof(buf)))
    return buf;
  return "<name not convertible>";
}

constexpr const char *to_string(D3D12_DRED_ALLOCATION_TYPE type)
{
#define CASE(name) \
  case D3D12_DRED_ALLOCATION_TYPE_##name: return #name
  switch (type)
  {
    CASE(COMMAND_QUEUE);
    CASE(COMMAND_ALLOCATOR);
    CASE(PIPELINE_STATE);
    CASE(COMMAND_LIST);
    CASE(FENCE);
    CASE(DESCRIPTOR_HEAP);
    CASE(HEAP);
    CASE(QUERY_HEAP);
    CASE(COMMAND_SIGNATURE);
    CASE(PIPELINE_LIBRARY);
    CASE(VIDEO_DECODER);
    CASE(VIDEO_PROCESSOR);
    CASE(RESOURCE);
    CASE(PASS);
    CASE(CRYPTOSESSION);
    CASE(CRYPTOSESSIONPOLICY);
    CASE(PROTECTEDRESOURCESESSION);
    CASE(VIDEO_DECODER_HEAP);
    CASE(COMMAND_POOL);
    CASE(COMMAND_RECORDER);
    CASE(STATE_OBJECT);
    CASE(METACOMMAND);
    CASE(SCHEDULINGGROUP);
    CASE(VIDEO_MOTION_ESTIMATOR);
    CASE(VIDEO_MOTION_VECTOR_HEAP);
    CASE(VIDEO_EXTENSION_COMMAND);
    default: return "<invalid>";
  }
#undef CASE
}

constexpr const char *to_string(D3D12_DRED_DEVICE_STATE state)
{
#define CASE(name) \
  case D3D12_DRED_DEVICE_STATE_##name: return #name
  switch (state)
  {
    CASE(UNKNOWN);
    CASE(HUNG);
    CASE(FAULT);
    CASE(PAGEFAULT);
    default: return "<invalid>";
  }
#undef CASE
}

constexpr const char *to_string(D3D12_AUTO_BREADCRUMB_OP op)
{
#define CASE(name) \
  case D3D12_AUTO_BREADCRUMB_OP_##name: return #name
  switch (op)
  {
    CASE(SETMARKER);
    CASE(BEGINEVENT);
    CASE(ENDEVENT);
    CASE(DRAWINSTANCED);
    CASE(DRAWINDEXEDINSTANCED);
    CASE(EXECUTEINDIRECT);
    CASE(DISPATCH);
    CASE(COPYBUFFERREGION);
    CASE(COPYTEXTUREREGION);
    CASE(COPYRESOURCE);
    CASE(COPYTILES);
    CASE(RESOLVESUBRESOURCE);
    CASE(CLEARRENDERTARGETVIEW);
    CASE(CLEARUNORDEREDACCESSVIEW);
    CASE(CLEARDEPTHSTENCILVIEW);
    CASE(RESOURCEBARRIER);
    CASE(EXECUTEBUNDLE);
    CASE(PRESENT);
    CASE(RESOLVEQUERYDATA);
    CASE(BEGINSUBMISSION);
    CASE(ENDSUBMISSION);
    CASE(DECODEFRAME);
    CASE(PROCESSFRAMES);
    CASE(ATOMICCOPYBUFFERUINT);
    CASE(ATOMICCOPYBUFFERUINT64);
    CASE(RESOLVESUBRESOURCEREGION);
    CASE(WRITEBUFFERIMMEDIATE);
    CASE(DECODEFRAME1);
    CASE(SETPROTECTEDRESOURCESESSION);
    CASE(DECODEFRAME2);
    CASE(PROCESSFRAMES1);
    CASE(BUILDRAYTRACINGACCELERATIONSTRUCTURE);
    CASE(EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO);
    CASE(COPYRAYTRACINGACCELERATIONSTRUCTURE);
    CASE(DISPATCHRAYS);
    CASE(INITIALIZEMETACOMMAND);
    CASE(EXECUTEMETACOMMAND);
    CASE(ESTIMATEMOTION);
    CASE(RESOLVEMOTIONVECTORHEAP);
    CASE(SETPIPELINESTATE1);
    CASE(INITIALIZEEXTENSIONCOMMAND);
    CASE(EXECUTEEXTENSIONCOMMAND);
    CASE(DISPATCHMESH);
    CASE(ENCODEFRAME);
    CASE(RESOLVEENCODEROUTPUTMETADATA);
    CASE(BARRIER);
    CASE(BEGIN_COMMAND_LIST);
    CASE(DISPATCHGRAPH);
    CASE(SETPROGRAM);
    CASE(SET_WORK_GRAPH_MAXIMUM_GPU_INPUT_RECORDS);
    CASE(ENCODEFRAME1);
    CASE(RESOLVEENCODEROUTPUTMETADATA1);
    CASE(PROCESSFRAMES2);
    CASE(RESOLVEINPUTPARAMLAYOUT);
    default: return "<invalid>";
  }
#undef CASE
}

bool is_dred_avilable()
{
  auto enablement = drv3d_dx12::get_application_DRED_enablement_from_registry();
  return D3D12_DRED_ENABLEMENT_FORCED_ON == enablement;
}

bool load_pix_event_runtime()
{
#if USE_PIX
  return LoadLibraryW(L"WinPixEventRuntime.dll") != nullptr;
#endif
  return false;
}

bool use_markers_only(const drv3d_dx12::debug::Configuration &config)
{
  return config.dredMarkersOnly && (config.usePIXCapturer || load_pix_event_runtime());
}

// DRED keeps the last 64Ki ops of a command list, older ops are overwritten
constexpr uint32_t BREADCRUMB_HISTORY_SIZE = 64 * 1024;
// the window counts the ops that are not events, the events in it are shown as the tree around them
constexpr uint32_t WORK_OPS_BEFORE_IN_FLIGHT = 16;
constexpr uint32_t WORK_OPS_AFTER_IN_FLIGHT = 8;
constexpr uint32_t MAX_WINDOW_OPS = 128; // per side, bounds the report where events are dense

constexpr wchar_t COMMAND_LIST_MARKER_PREFIX[] = L"Command list ";
constexpr size_t COMMAND_LIST_MARKER_PREFIX_LENGTH = eastl::size(COMMAND_LIST_MARKER_PREFIX) - 1;
using CommandListMarkerText = char[64];

[[maybe_unused]] void encode_command_list_marker(drv3d_dx12::debug::CommandListIdentifier id, CommandListMarkerText &text)
{
  _snprintf_s(text, _TRUNCATE, "%ls%" PRIXPTR, COMMAND_LIST_MARKER_PREFIX, eastl::bit_cast<uintptr_t>(id));
}

bool is_command_list_marker(const wchar_t *context)
{
  return context && wcsncmp(context, COMMAND_LIST_MARKER_PREFIX, COMMAND_LIST_MARKER_PREFIX_LENGTH) == 0;
}

drv3d_dx12::debug::CommandListIdentifier decode_command_list_marker(const wchar_t *context)
{
  const auto bits = static_cast<uintptr_t>(wcstoull(context + COMMAND_LIST_MARKER_PREFIX_LENGTH, nullptr, 16));
  return eastl::bit_cast<drv3d_dx12::debug::CommandListIdentifier>(bits);
}

D3D12_AUTO_BREADCRUMB_OP get_breadcrumb_op(const D3D12_AUTO_BREADCRUMB_NODE1 *node, uint32_t index)
{
  return node->pCommandHistory[index % BREADCRUMB_HISTORY_SIZE];
}

bool is_event(D3D12_AUTO_BREADCRUMB_OP op)
{
  return D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT == op || D3D12_AUTO_BREADCRUMB_OP_ENDEVENT == op;
}

const char *breadcrumb_context(const D3D12_AUTO_BREADCRUMB_NODE1 *node, uint32_t index, DebugNameBuffer &buf)
{
  for (const auto &context : eastl::span(node->pBreadcrumbContexts, node->BreadcrumbContextsCount))
  {
    if (context.BreadcrumbIndex == index)
      return context.pContextString && wcs_to_utf8(context.pContextString, buf, sizeof(buf)) ? buf : nullptr;
  }
  return nullptr;
}

eastl::vector<uint32_t> open_events(const D3D12_AUTO_BREADCRUMB_NODE1 *node, uint32_t oldest, uint32_t index)
{
  eastl::vector<uint32_t> open;
  for (uint32_t i = oldest; i < index; ++i)
  {
    const D3D12_AUTO_BREADCRUMB_OP op = get_breadcrumb_op(node, i);
    if (D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT == op)
      open.push_back(i);
    else if (D3D12_AUTO_BREADCRUMB_OP_ENDEVENT == op && !open.empty()) // the begin can be older than the ring
      open.pop_back();
  }
  return open;
}

void report_event_path(const D3D12_AUTO_BREADCRUMB_NODE1 *node, const eastl::vector<uint32_t> &open, eastl::string &out)
{
  if (open.empty())
    return;

  out += "  in '";
  for (uint32_t index : open)
  {
    DebugNameBuffer buf;
    const char *name = breadcrumb_context(node, index, buf);
    out += name ? name : "?";
    if (out.back() != '/') // some event names end with '/' already
      out += '/';
  }
  out.back() = '\'';
  out += '\n';
}

uint32_t report_command_list_marker(const D3D12_AUTO_BREADCRUMB_NODE1 *node, eastl::string &out)
{
  for (const auto &context : eastl::span(node->pBreadcrumbContexts, node->BreadcrumbContextsCount))
  {
    if (!is_command_list_marker(context.pContextString))
      continue;

    const auto fields = drv3d_dx12::debug::decode_command_list_identifier(decode_command_list_marker(context.pContextString));
    out.append_sprintf("(frame slot %u, history set %u, recorded list %u of the frame)\n", //
      fields.frameSlot, fields.historySet, fields.commandListIndex);
    return context.BreadcrumbIndex;
  }
  return ~0u;
}

// an ENDEVENT at the same level closes the parent, so no sibling follows
bool has_next_sibling(const D3D12_AUTO_BREADCRUMB_NODE1 *node, uint32_t index)
{
  uint32_t nested = D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT == get_breadcrumb_op(node, index) ? 1 : 0;
  for (uint32_t i = index + 1; i < node->BreadcrumbCount; ++i)
  {
    const D3D12_AUTO_BREADCRUMB_OP op = get_breadcrumb_op(node, i);
    if (nested == 0)
      return D3D12_AUTO_BREADCRUMB_OP_ENDEVENT != op;
    if (D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT == op)
      ++nested;
    else if (D3D12_AUTO_BREADCRUMB_OP_ENDEVENT == op)
      --nested;
  }
  return false;
}

void report_breadcrumb(const D3D12_AUTO_BREADCRUMB_NODE1 *node, uint32_t index, uint32_t completed,
  const eastl::vector<bool> &open_last, bool last, eastl::string &out)
{
  const char *status = index < completed ? "[COMPLETED]" : index == completed ? "[IN FLIGHT]" : "[RECORDED ]";
  out.append_sprintf("  %s %6u | ", status, index);
  for (bool parentLast : open_last) // a last child has no sibling below, so no rail
    out += parentLast ? " " : "| ";
  out += last ? "\\" : "+";

  const D3D12_AUTO_BREADCRUMB_OP op = get_breadcrumb_op(node, index);
  DebugNameBuffer buf;
  const char *context = breadcrumb_context(node, index, buf);
  if (D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT != op || !context)
    out.append_sprintf(" %s", to_string(op));
  if (context)
    out.append_sprintf(" '%s'", context);
  out += '\n';
}

void report_breadcrumb_node(const D3D12_AUTO_BREADCRUMB_NODE1 *node, eastl::string &out)
{
  DebugNameBuffer listName;
  DebugNameBuffer queueName;
  const char *list = debug_name(node->pCommandListDebugNameA, node->pCommandListDebugNameW, listName);
  const char *queue = debug_name(node->pCommandQueueDebugNameA, node->pCommandQueueDebugNameW, queueName);
  const uint32_t count = node->BreadcrumbCount;
  const uint32_t completed = node->pLastBreadcrumbValue ? *node->pLastBreadcrumbValue : 0;
  out.append_sprintf("Queue '%s' %p list '%s' %p: ", queue, node->pCommandQueue, list, node->pCommandList);
  if (completed >= count)
  {
    out.append_sprintf("all %u ops completed\n", count);
    return;
  }

  out.append_sprintf("%u of %u ops completed\n", completed, count);
  const uint32_t marker = report_command_list_marker(node, out);
  if (!node->pCommandHistory)
  {
    out += "  no op history\n\n";
    return;
  }

  const uint32_t oldest = count > BREADCRUMB_HISTORY_SIZE ? count - BREADCRUMB_HISTORY_SIZE : 0;
  uint32_t windowBegin = eastl::max(oldest, completed);
  for (uint32_t work = 0; work < WORK_OPS_BEFORE_IN_FLIGHT && windowBegin > oldest && completed - windowBegin < MAX_WINDOW_OPS;)
    work += !is_event(get_breadcrumb_op(node, --windowBegin));
  uint32_t windowEnd = completed + 1;
  for (uint32_t work = 0; work < WORK_OPS_AFTER_IN_FLIGHT && windowEnd < count && windowEnd - completed <= MAX_WINDOW_OPS; ++windowEnd)
    work += !is_event(get_breadcrumb_op(node, windowEnd));
  report_event_path(node, open_events(node, oldest, completed), out);
  out += "  status        op # | op 'event'\n";
  // the events still open at the window start, then the window
  eastl::vector<uint32_t> shown = open_events(node, oldest, windowBegin);
  for (uint32_t i = windowBegin; i < windowEnd; ++i)
    shown.push_back(i);

  eastl::vector<bool> openLast;
  for (uint32_t i : shown)
  {
    const D3D12_AUTO_BREADCRUMB_OP op = get_breadcrumb_op(node, i);
    const bool isEnd = D3D12_AUTO_BREADCRUMB_OP_ENDEVENT == op;
    const bool last = isEnd || !has_next_sibling(node, i);
    // an end event only closes the block, unless it is the op in flight
    if (i != marker && (!isEnd || i == completed))
      report_breadcrumb(node, i, completed, openLast, last, out);

    if (D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT == op)
      openLast.push_back(last);
    else if (isEnd && !openLast.empty())
      openLast.pop_back();
  }
  out += "\n";
}

void report_breadcrumbs(ID3D12DeviceRemovedExtendedData2 *dred, eastl::string &out)
{
  D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT1 output;
  if (FAILED(dred->GetAutoBreadcrumbsOutput1(&output)))
  {
    out += "No DRED auto breadcrumb data available\n";
    return;
  }
  if (!output.pHeadAutoBreadcrumbNode)
  {
    out += "No DRED auto breadcrumb nodes found\n";
    return;
  }
  for (auto node = output.pHeadAutoBreadcrumbNode; node; node = node->pNext)
    report_breadcrumb_node(node, out);
}

void report_allocations(const char *title, const D3D12_DRED_ALLOCATION_NODE1 *node, eastl::string &out)
{
  out.append_sprintf("%s:\n", title);
  if (!node)
  {
    out += "none\n";
    return;
  }
  for (; node; node = node->pNext)
  {
    DebugNameBuffer name;
    out.append_sprintf("%s %p '%s'\n", to_string(node->AllocationType), node->pObject,
      debug_name(node->ObjectNameA, node->ObjectNameW, name));
  }
}

void report_page_fault(ID3D12DeviceRemovedExtendedData2 *dred, eastl::string &out)
{
  D3D12_DRED_PAGE_FAULT_OUTPUT2 output = {};
  if (FAILED(dred->GetPageFaultAllocationOutput2(&output)))
  {
    out += "No DRED page fault data available\n";
    return;
  }

  out.append_sprintf("GPU page fault address 0x%016" PRIX64 ", flags 0x%X\n", output.PageFaultVA,
    static_cast<uint32_t>(output.PageFaultFlags));
  report_allocations("Existing allocations near the fault address", output.pHeadExistingAllocationNode, out);
  report_allocations("Recently freed allocations near the fault address", output.pHeadRecentFreedAllocationNode, out);
}

void report_dred(ID3D12DeviceRemovedExtendedData2 *dred)
{
  eastl::string report;
  report.append_sprintf("DX12: DRED device state %s\n", to_string(dred->GetDeviceState()));
  report_breadcrumbs(dred, report);
  report_page_fault(dred, report);
  report.pop_back(); // logdbg adds its own '\n'
  logdbg("%s", report.c_str());
}
} // namespace

namespace drv3d_dx12::debug::gpu_postmortem::microsoft
{

bool DeviceRemovedExtendedData::try_load(const Configuration &config, const Direct3D12Enviroment &d3d_env)
{
  if (!config.enableDRED)
  {
    logdbg("DX12: ...DRED is not allowed per configuration...");
    return false;
  }

  if (!config.ignoreDREDAvailability)
  {
    if (!is_dred_avilable())
    {
      logdbg("DX12: ...after registry inspection, it seems DRED is not available, DRED is ignored...");
      return false;
    }
  }
  else
  {
    logdbg("DX12: ...skipped DRED availability inspection of the registry, assuming it is available...");
  }

  logdbg("DX12: ...loading debug interface query...");
  PFN_D3D12_GET_DEBUG_INTERFACE D3D12GetDebugInterface = nullptr;
  reinterpret_cast<FARPROC &>(D3D12GetDebugInterface) = d3d_env.getD3DProcAddress("D3D12GetDebugInterface");
  if (!D3D12GetDebugInterface)
  {
    logdbg("DX12: ...D3D12GetDebugInterface not found in direct dx runtime library...");
    return false;
  }

  ComPtr<ID3D12DeviceRemovedExtendedDataSettings2> dredConfig2;
  if (FAILED(D3D12GetDebugInterface(COM_ARGS(&dredConfig2))))
  {
    logdbg("DX12: ...unable to query DRED settings interface, assuming it is unavailable...");
    return false;
  }

  dredConfig2->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  dredConfig2->SetBreadcrumbContextEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);

  if (config.enableDREDDumps)
  {
    dredConfig2->SetWatsonDumpEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  }

  if (use_markers_only(config))
  {
    dredConfig2->UseMarkersOnlyAutoBreadcrumbs(TRUE);
  }

  if (config.trackPageFaults)
  {
    dredConfig2->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
  }

  return true;
}

DeviceRemovedExtendedData::DeviceRemovedExtendedData(const Configuration &config) :
  hasPixRuntime{load_pix_event_runtime()}, writeEvents{hasPixRuntime && !config.usePIXCapturer}
{
#if USE_PIX
  const char *noEventSource = "none, WinPixEventRuntime.dll not found";
#else
  const char *noEventSource = "none, built without PIX";
#endif
  const char *eventSource = writeEvents ? "DRED module" : hasPixRuntime ? "PIX capturer" : noEventSource;
  logdbg("DX12: DRED enabled, breadcrumbs %s, event context from %s, page faults %s, Watson dump %s",
    use_markers_only(config) ? "markers only" : "all ops", eventSource, config.trackPageFaults ? "on" : "off",
    config.enableDREDDumps ? "on" : "off");
}

void DeviceRemovedExtendedData::beginCommandBuffer(D3DDevice *, [[maybe_unused]] CommandListIdentifier cmd_id,
  [[maybe_unused]] D3DGraphicsCommandList *cmd)
{
#if USE_PIX
  if (!hasPixRuntime)
    return;
  CommandListMarkerText text;
  encode_command_list_marker(cmd_id, text);
  PIXSetMarker(cmd, 0, text);
#endif
}

void DeviceRemovedExtendedData::endCommandBuffer(D3DGraphicsCommandList *) {}

void DeviceRemovedExtendedData::beginEvent([[maybe_unused]] D3DGraphicsCommandList *cmd, [[maybe_unused]] eastl::span<const char> text,
  const eastl::string &)
{
#if USE_PIX
  if (writeEvents)
    PIXBeginEvent(cmd, 0, text.data());
#endif
}

void DeviceRemovedExtendedData::endEvent([[maybe_unused]] D3DGraphicsCommandList *cmd, const eastl::string &)
{
#if USE_PIX
  if (writeEvents)
    PIXEndEvent(cmd);
#endif
}

void DeviceRemovedExtendedData::marker([[maybe_unused]] D3DGraphicsCommandList *cmd, [[maybe_unused]] eastl::span<const char> text)
{
#if USE_PIX
  if (writeEvents)
    PIXSetMarker(cmd, 0, text.data());
#endif
}

void DeviceRemovedExtendedData::onDeviceRemoved(D3DDevice *device, HRESULT reason, call_stack::Reporter &)
{
  if (DXGI_ERROR_INVALID_CALL == reason)
  {
    // Data from DRED is not useful when the runtime detected a invalid call.
    return;
  }
  logdbg("DX12: Acquiring DRED interface...");
  ComPtr<ID3D12DeviceRemovedExtendedData2> dred;
  if (FAILED(device->QueryInterface(COM_ARGS(&dred))))
  {
    logdbg("DX12: ...failed, no DRED information available");
    return;
  }

  report_dred(dred.Get());
}

void DeviceRemovedExtendedData::onDeviceShutdown() {}

void DeviceRemovedExtendedData::nameResource(ID3D12Resource *resource, eastl::string_view name) { set_object_name(resource, name); }

void DeviceRemovedExtendedData::nameResource(ID3D12Resource *resource, eastl::wstring_view name) { set_object_name(resource, name); }

void DeviceRemovedExtendedData::nameObject(ID3D12Object *object, eastl::string_view name) { set_object_name(object, name); }

void DeviceRemovedExtendedData::nameObject(ID3D12Object *object, eastl::wstring_view name) { set_object_name(object, name); }

} // namespace drv3d_dx12::debug::gpu_postmortem::microsoft
