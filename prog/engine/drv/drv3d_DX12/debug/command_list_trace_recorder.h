#pragma once

#include <supp/dag_comPtr.h>

#include "driver.h"
#include "winapi_helpers.h"


namespace drv3d_dx12::debug
{
class CommandListTraceRecorder
{
public:
  using TraceID = uint32_t;

private:
  static constexpr uint32_t TraceBufferSize = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  static constexpr uint32_t TracesPerBuffer = TraceBufferSize / sizeof(TraceID);

  TraceID firstID = 0;
  TraceID lastID = 0;
  VirtualAllocPtr<TraceID[]> traceMemory;
  ComPtr<ID3D12Heap> traceMemoryHeap;
  ComPtr<ID3D12Resource> traceBuffer;
  D3D12_GPU_VIRTUAL_ADDRESS traceBufferAddress = {};

  static uint32_t getTraceIndex(TraceID trace) { return trace % TracesPerBuffer; }
  static uint32_t getTraceOffset(TraceID trace) { return getTraceIndex(trace) * sizeof(TraceID); }
  bool wasTraceCompleted(TraceID trace) const { return traceMemory[getTraceIndex(trace)] == trace; }

public:
  CommandListTraceRecorder() = default;
  CommandListTraceRecorder(ID3D12Device3 *device)
  {
    // setup stuff for software mode
    // Memory has to be sourced from VirtualAlloc and has to be the complete memory
    // section, subranges are not possible as the heap gets its size from the allocation
    // metadata.
    // NOTE memory returned by VirtualAlloc is nulled out so no memset 0 needed to
    // tidy up that memory block
    traceMemory.reset(reinterpret_cast<uint32_t *>(
      VirtualAlloc(nullptr, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)));
    if (!traceMemory)
    {
      logdbg("DX12: VirtualAlloc of %u bytes failed, can not record GPU trace", D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
      return;
    }

    if (FAILED(device->OpenExistingHeapFromAddress(traceMemory.get(), COM_ARGS(&traceMemoryHeap))))
    {
      logdbg("DX12: OpenExistingHeapFromAddress failed, can not record GPU trace");
      traceMemory.reset();
      return;
    }

    // need the exact properties to setup the buffer correctly
    auto heapDesc = traceMemoryHeap->GetDesc();

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    bufferDesc.Width = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    // flags have to match or create call will fail
    if (heapDesc.Flags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER)
    {
      bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
    }

    if (FAILED(device->CreatePlacedResource(traceMemoryHeap.Get(), 0, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
          COM_ARGS(&traceBuffer))))
    {
      logdbg("DX12: CreatePlacedResource failed, can not record GPU trace");
      traceMemoryHeap.Reset();
      traceMemory.reset();
      return;
    }

    traceBufferAddress = traceBuffer->GetGPUVirtualAddress();
    logdbg("DX12: New GPU trace recording buffer, memory at %p, heap %p, buffer %p at GPU address %u", traceMemory.get(),
      traceMemoryHeap.Get(), traceBuffer.Get(), traceBufferAddress);
  }

  ~CommandListTraceRecorder() = default;
  CommandListTraceRecorder(CommandListTraceRecorder &&) = default;
  CommandListTraceRecorder &operator=(CommandListTraceRecorder &&) = default;

  TraceID record(ID3D12GraphicsCommandList2 *cmd)
  {
    if (!traceMemory)
    {
      return lastID;
    }
    auto id = ++lastID;

    D3D12_WRITEBUFFERIMMEDIATE_PARAMETER param;
    param.Value = id;
    param.Dest = traceBufferAddress + getTraceOffset(param.Value);
    // we want to know if previous commands where done or not, "marker in" ensures that
    D3D12_WRITEBUFFERIMMEDIATE_MODE mode = D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_IN;
    cmd->WriteBufferImmediate(1, &param, &mode);

    return id;
  }

  // do not call this repeatedly, as it may loop over the whole trace buffer each time
  uint32_t completedCount() const
  {
    if (!traceMemory)
    {
      return 0;
    }
    // This may look a bit awkward and inefficient but this has to handle wraparounds of the ids in the range of first to last.
    uint32_t count = 0;
    for (uint32_t count = 0; count < TracesPerBuffer; ++count)
    {
      auto trace = firstID + count;
      if (!wasTraceCompleted(trace))
      {
        break;
      }
      if (trace == lastID)
      {
        break;
      }
    }
    return count;
  }

  bool isCompleted() const
  {
    if (firstID == lastID)
    {
      return true;
    }

    return wasTraceCompleted(lastID);
  }

  TraceID indexToTraceID(uint32_t index) const { return firstID + index; }

  // Instead of nulling out a memory block, we just change the range of ids to start where we left off.
  void beginRecording() { firstID = lastID; }
};
} // namespace drv3d_dx12::debug