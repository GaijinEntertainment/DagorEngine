// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <driver.h>
#include <supp/dag_comPtr.h>
#include <winapi_helpers.h>
#include "trace_status.h"
#include "trace_id.h"
#include "trace_run_status.h"


namespace drv3d_dx12::debug
{
/// A relative simple pool of trace recording buffers.
/// Each pool is backed by a 65KiByte buffer. Each pool can supply up to 64 command lists with trace memory.
/// Each trace is represented by 2 values written during GPU execution. The first value is written after the
/// GPU has launched all preceding commands. The second value is written after the GPU has finished all
/// preceding commands. Both values are written to the same memory location. Both values have the same index
/// value, but the first has not the isCompleted bit set, where the second one has it set.
/// When the end of the memory space for the trace recordings is reached, we wrap around and overwrite existing
/// entries. This will not break information about past commands, except in the case when the GPU can launch
/// more than 256 commands at a time, without needing to complete them, which is unlikely. We can deduce from
/// each memory location how many times it was overwritten. We assume overwritten values where completed.
/// To be able to distinguish between subsequent uses of the trace buffer, we add a run offset to the memory
/// index where the values are written. With this each time we start a new use (called run) we increment the
/// counter and so values of past runs will yield incorrect values to the expected values. This allows us to
/// distinguish up to 256 different runs before we overflow, this should be sufficient to avoid clashes with
/// false information.
class CommandListTraceRecorderPool
{
  /// Number of trace sets that can be supplied by one buffer, this value is more or less arbitrary.
  /// Number was chosen so that we have 256 number of entries per trace set so that we can use a byte
  /// as run counter.
  static constexpr uint32_t recorders_per_pool = 64;
  /// 64KiBytes is the min for a buffer, we don't need more than that, so go with it.
  static constexpr uint32_t trace_buffer_size = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  /// Total number of trace values we can store in one buffer.
  static constexpr uint32_t trace_entires_per_buffer = trace_buffer_size / sizeof(uint32_t);
  /// Number of entries per recorders, should be less or equal to 256 so we can use one byte for run counting.
  static constexpr uint32_t traces_per_recorder = trace_entires_per_buffer / recorders_per_pool;

public:
  enum class RecorderID : uint32_t
  {
  };

private:
  struct Pool
  {
    uint8_t traceRuns[recorders_per_pool]{};
    uint32_t runLengths[recorders_per_pool]{};
    VirtualAllocPtr<uint32_t[]> memory;
    ComPtr<ID3D12Heap> memoryHeap;
    ComPtr<ID3D12Resource> buffer;
    D3D12_GPU_VIRTUAL_ADDRESS bufferMemory = {};

    G_STATIC_ASSERT(traces_per_recorder <= 256);

    static uint32_t to_sub_index(RecorderID recorder) { return static_cast<uint32_t>(recorder) % recorders_per_pool; }

    // Always safe to invoke, will check if memory is valid or not.
    // Will return 0 when no memory is available, which corresponds to an invalid trace value, which will always
    // yield NotLaunched.
    uint32_t readMemoryLocation(RecorderID recorder, TraceID trace) const
    {
      if (!memory)
      {
        return 0;
      }
      const auto subIndex = to_sub_index(recorder);
      const auto traceIndex = static_cast<uint32_t>(trace);
      const auto baseOffset = subIndex * traces_per_recorder;
      const auto readLocation = (traceIndex + traceRuns[subIndex]) % traces_per_recorder;
      return memory[baseOffset + readLocation];
    }
    // Reads the memory location where trace is stored and computes the status from the read value.
    TraceStatus readTraceStatus(RecorderID recorder, TraceID trace) const
    {
      const auto traceIndex = static_cast<uint32_t>(trace);
      const auto value = readMemoryLocation(recorder, trace);
      const auto isCompleted = value & 1;
      const auto traceValue = value >> 1;
      if (traceIndex == traceValue)
      {
        return isCompleted ? TraceStatus::Completed : TraceStatus::Launched;
      }
      if ((traceValue % traces_per_recorder) != (traceIndex % traces_per_recorder))
      {
        // was from a different trace run, so this memory location was never written by this trace run
        return TraceStatus::NotLaunched;
      }
      return traceValue > traceIndex ? TraceStatus::Completed : TraceStatus::NotLaunched;
    }
    // Always safe to invoke, will check if buffer is valid or not.
    // Will return invalid_trace_value when no buffer is available, which will always yield NotLaunched.
    // Records a new trace with cmd and returns the trace value to check the status with
    TraceID recordTrace(RecorderID recorder, D3DGraphicsCommandList *cmd)
    {
      if (!buffer)
      {
        return invalid_trace_value;
      }
      const auto subIndex = to_sub_index(recorder);
      const auto traceIndex = ++runLengths[subIndex];

      const auto baseOffset = subIndex * traces_per_recorder;
      const auto writeLocation = (traceIndex + traceRuns[subIndex]) % traces_per_recorder;
      const auto memoryOffset = (baseOffset + writeLocation) * sizeof(uint32_t);

      // We write 2 values to the same memory location, first value is the "in" value, the value that indicates
      // all commands until this write command are launched. The second value is the "out" value, the value that
      // indicates that all commands until this write command are completed. With this we can very easily
      // determine if all commands where launched and completed for this check point.
      const D3D12_WRITEBUFFERIMMEDIATE_PARAMETER params[2] = {
        {
          .Dest = bufferMemory + memoryOffset,
          .Value = 0u | (traceIndex << 1u),
        },
        {
          .Dest = bufferMemory + memoryOffset,
          .Value = 1u | (traceIndex << 1u),
        },
      };
      const D3D12_WRITEBUFFERIMMEDIATE_MODE modes[2] = {
        D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_IN, D3D12_WRITEBUFFERIMMEDIATE_MODE_MARKER_OUT};
      cmd->WriteBufferImmediate(2, params, modes);
      return static_cast<TraceID>(traceIndex);
    }
    // Cheap check to see if all traces where recorded as completed
    bool isCompleted(RecorderID recorder) const
    {
      const auto runLength = runLengths[to_sub_index(recorder)];
      return (0 == runLength) || (TraceStatus::Completed == readTraceStatus(recorder, static_cast<TraceID>(runLength)));
    }
    // Resets trace set to start a new run
    void beginRecording(RecorderID recorder)
    {
      const auto subIndex = to_sub_index(recorder);
      ++traceRuns[subIndex];
      runLengths[subIndex] = 0;
    }
    // Cheap check if the first trace entry was written as at least launched
    bool hasAnyTracesRecorded(RecorderID recorder) const
    {
      if (runLengths[to_sub_index(recorder)] < 1)
      {
        return false;
      }
      // simply check if the first trace has any data for this run
      auto status = readTraceStatus(recorder, static_cast<TraceID>(1));
      return TraceStatus::NotLaunched != status;
    }
    // Returns the number of recorded traces for the trace set of the current run
    uint32_t getTraceRecordIssued(RecorderID recorder) const { return runLengths[to_sub_index(recorder)]; }
  };

  eastl::vector<Pool> pools;
  uint32_t allocatedRecorders = 0;

  void allocateTracePool(D3DDevice *device)
  {
    auto &newPool = pools.emplace_back();
    // setup stuff for software mode
    // Memory has to be sourced from VirtualAlloc and has to be the complete memory
    // section, subranges are not possible as the heap gets its size from the allocation
    // metadata.
    // NOTE: memory returned by VirtualAlloc is nulled out so no memset 0 needed to
    // tidy up that memory block
    newPool.memory.reset(
      reinterpret_cast<uint32_t *>(VirtualAlloc(nullptr, trace_buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)));
    if (!newPool.memory)
    {
      logdbg("DX12: VirtualAlloc of %u bytes failed, can not record GPU trace", trace_buffer_size);
      return;
    }

    if (FAILED(device->OpenExistingHeapFromAddress(newPool.memory.get(), COM_ARGS(&newPool.memoryHeap))))
    {
      logdbg("DX12: OpenExistingHeapFromAddress failed, can not record GPU trace");
      newPool.memory.reset();
      return;
    }

    // need the exact properties to setup the buffer correctly
    const auto heapDesc = newPool.memoryHeap->GetDesc();

    const D3D12_RESOURCE_DESC bufferDesc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
      .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
      .Width = trace_buffer_size,
      .Height = 1,
      .DepthOrArraySize = 1,
      .MipLevels = 1,
      .Format = DXGI_FORMAT_UNKNOWN,
      .SampleDesc =
        {
          .Count = 1,
          .Quality = 0,
        },
      .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
      // flags have to match or create call will fail
      .Flags =
        (heapDesc.Flags & D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER) ? D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER : D3D12_RESOURCE_FLAG_NONE,
    };

    if (FAILED(device->CreatePlacedResource(newPool.memoryHeap.Get(), 0, &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
          COM_ARGS(&newPool.buffer))))
    {
      logdbg("DX12: CreatePlacedResource failed, can not record GPU trace");
      newPool.memory.reset();
      newPool.memoryHeap.Reset();
      return;
    }

    newPool.bufferMemory = newPool.buffer->GetGPUVirtualAddress();
    logdbg("DX12: New GPU trace recording pool buffer, memory at %p, heap %p, buffer %p at GPU address %016X", newPool.memory.get(),
      newPool.memoryHeap.Get(), newPool.buffer.Get(), newPool.bufferMemory);
    return;
  }

  Pool &getPool(RecorderID recorder) { return pools[static_cast<uint32_t>(recorder) / recorders_per_pool]; }
  const Pool &getPool(RecorderID recorder) const { return pools[static_cast<uint32_t>(recorder) / recorders_per_pool]; }

public:
  // Will de-allocate all pools
  void reset()
  {
    allocatedRecorders = 0;
    pools.clear();
  }
  RecorderID allocateTraceRecorder(D3DDevice *device)
  {
    if (0 == (allocatedRecorders % recorders_per_pool))
    {
      allocateTracePool(device);
    }
    return static_cast<RecorderID>(allocatedRecorders++);
  }
  TraceID recordTrace(RecorderID recorder, D3DGraphicsCommandList *cmd) { return getPool(recorder).recordTrace(recorder, cmd); }
  bool isCompleted(RecorderID recorder) const { return getPool(recorder).isCompleted(recorder); }
  void beginRecording(RecorderID recorder) { return getPool(recorder).beginRecording(recorder); }
  TraceStatus readTraceStatus(RecorderID recorder, TraceID trace) const { return getPool(recorder).readTraceStatus(recorder, trace); }
  // quick check if any traces were recorded by the GPU
  bool hasAnyTracesRecorded(RecorderID recorder) const { return getPool(recorder).hasAnyTracesRecorded(recorder); }
  // returns number of trace entries we tried to write
  uint32_t getTraceRecordIssued(RecorderID recorder) const { return getPool(recorder).getTraceRecordIssued(recorder); }
  TraceRunStatus getTraceRunStatus(RecorderID recorder) const
  {
    if (isCompleted(recorder))
    {
      return TraceRunStatus::CompletedExecution;
    }
    else if (hasAnyTracesRecorded(recorder))
    {
      return TraceRunStatus::ErrorDuringExecution;
    }
    else
    {
      return TraceRunStatus::NotLaunched;
    }
  }
};
} // namespace drv3d_dx12::debug