#pragma once

#include <EASTL/utility.h>
#include <EASTL/vector.h>
#include <osApiWrappers/dag_lockProfiler.h>
#include <3d/dag_drv3d.h>

#include "d3d12_error_handling.h"
#include "d3d12_debug_names.h"


#if _TARGET_XBOXONE
#include <xg.h>
#elif _TARGET_SCARLETT
#include <xg_xs.h>
#endif

namespace drv3d_dx12
{
// Xbox One mapping:
// Graphics:
// Engine | Mapping
// 0      | GRAPHICS
//
// Compute:
// Engine | Queue | Mapping
// 0      | 0     | COMPUTE
// 0      | 1     | unused
// 0      | 2     | unused
// 0      | 3     | unused
// 0      | 4     | unused
// 0      | 5     | unused
// 1      | 0     | unused
// 1      | 1     | unused
// 1      | 2     | unused
//
// Copy:
// Engine | Mapping
// 1      | UPLOAD
// 2      | READ_BACK
// 3      | unused
enum class DeviceQueueType
{
  GRAPHICS,
  COMPUTE,
  UPLOAD,
  READ_BACK,

  COUNT,
  INVALID = COUNT
};

class DeviceQueue
{
  ComPtr<ID3D12CommandQueue> queue;

public:
  DeviceQueue() = default;
  ~DeviceQueue();

  DeviceQueue(const DeviceQueue &) = default;
  DeviceQueue &operator=(const DeviceQueue &) = default;

  DeviceQueue(DeviceQueue &&) = default;
  DeviceQueue &operator=(DeviceQueue &&) = default;

  DeviceQueue(ComPtr<ID3D12CommandQueue> q) : queue{eastl::move(q)} {}

  ID3D12CommandQueue *getHandle() const { return queue.Get(); }
  void enqueueCommands(uint32_t count, ID3D12CommandList *const *cmds) { queue->ExecuteCommandLists(count, cmds); }
  bool enqueueProgressUpdate(ID3D12Fence *f, uint64_t progress) { return DX12_CHECK_OK(queue->Signal(f, progress)); }
  bool syncWith(ID3D12Fence *f, uint64_t progress) { return DX12_CHECK_OK(queue->Wait(f, progress)); }

#if _TARGET_PC_WIN
  void beginTileMapping(ID3D12Resource *tex, ID3D12Heap *heap, size_t heap_base, size_t mapping_count);
#else
  void beginTileMapping(ID3D12Resource *tex, XGTextureAddressComputer *texAddressComputer, uintptr_t address, uint32_t size,
    size_t mapping_count);
#endif
  void addTileMappings(const TileMapping *mapping, size_t mapping_count);
  void endTileMapping();

private:
  static struct TileMapperAccel
  {
#if _TARGET_PC_WIN
    eastl::vector<D3D12_TILED_RESOURCE_COORDINATE> trcs;
    eastl::vector<D3D12_TILE_REGION_SIZE> trss;
    eastl::vector<UINT> offsets;
    eastl::vector<UINT> counts;
    eastl::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags;
    ID3D12Resource *tex = nullptr;
    ID3D12Heap *heap = nullptr;
    size_t heapBase = 0;
#else
    eastl::vector<D3D12XBOX_PAGE_MAPPING_BATCH> trBatches;
    eastl::vector<D3D12XBOX_PAGE_MAPPING_RANGE> trRanges;
    D3D12_GPU_VIRTUAL_ADDRESS sourcePoolPageVA = 0;
    uint32_t sourcePoolPageSize = 0;
    ID3D12Resource *tex = nullptr;
    ComPtr<XGTextureAddressComputer> pTexComputer;
#endif

    void clear();
  } tileMapperAccel;
};

class AsyncProgress
{
  ComPtr<ID3D12Fence> fence;
  uint64_t progress = 0;

public:
  AsyncProgress() = default;
  ~AsyncProgress() = default;

  AsyncProgress(const AsyncProgress &) = delete;
  AsyncProgress &operator=(const AsyncProgress &) = delete;

  AsyncProgress(AsyncProgress &&) = default;
  AsyncProgress &operator=(AsyncProgress &&) = default;

  void reset()
  {
    fence.Reset();
    progress = 0;
  }

  bool reset(ID3D12Device *device, const wchar_t *name)
  {
    reset();
    if (!DX12_CHECK_OK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, COM_ARGS(&fence))))
    {
      return false;
    }
    if (fence && name)
    {
      DX12_SET_DEBUG_OBJ_NAME(fence, name);
    }
    return true;
  }

  bool enqueueAdvance(DeviceQueue &queue) { return queue.enqueueProgressUpdate(fence.Get(), ++progress); }

  bool enqueueProgress(DeviceQueue &queue, uint64_t value)
  {
    progress = value;
    return queue.enqueueProgressUpdate(fence.Get(), value);
  }

  bool wait(DeviceQueue &queue) { return queue.syncWith(fence.Get(), progress); }

  uint64_t currentProgress() { return fence->GetCompletedValue(); }

  uint64_t expectedProgress() { return progress; }

  bool waitForProgress(uint64_t value, HANDLE handle) { return DX12_CHECK_OK(fence->SetEventOnCompletion(value, handle)); }

  bool wait(HANDLE handle) { return DX12_CHECK_OK(fence->SetEventOnCompletion(progress, handle)); }

  // sends a signale from 'signaler' to be waited on by 'signaled'
  // short cut for enqueueAdvance('signaler') and wait('signaled') pair
  bool synchronize(DeviceQueue &signaler, DeviceQueue &signaled)
  {
    if (!enqueueAdvance(signaler))
    {
      return false;
    }
    return wait(signaled);
  }

  explicit operator bool() const { return static_cast<bool>(fence); }
};

class DeviceQueueGroup
{
  DeviceQueue group[uint32_t(DeviceQueueType::COUNT)];
  AsyncProgress uploadProgress;
  AsyncProgress graphicsProgress;
  AsyncProgress frameProgress;

public:
  bool init(ID3D12Device *device, bool give_names);
  void shutdown();
  DeviceQueue &operator[](DeviceQueueType type) { return group[static_cast<uint32_t>(type)]; }
  const DeviceQueue &operator[](DeviceQueueType type) const { return group[static_cast<uint32_t>(type)]; }
#if _TARGET_XBOX
  void enterSuspendedState();
  void leaveSuspendedState();
#endif
  uint64_t checkFrameProgress() { return frameProgress.currentProgress(); }
  bool updateFrameProgress(uint64_t progress)
  {
    return frameProgress.enqueueProgress(group[static_cast<uint32_t>(DeviceQueueType::READ_BACK)], progress);
  }
  bool updateFrameProgressOnGraphics(uint64_t progress)
  {
    return frameProgress.enqueueProgress(group[static_cast<uint32_t>(DeviceQueueType::GRAPHICS)], progress);
  }
  bool waitForFrameProgress(uint64_t progress, HANDLE wait) { return frameProgress.waitForProgress(progress, wait); }
  bool synchronizeWithPreviousFrame() { return frameProgress.wait(group[static_cast<uint32_t>(DeviceQueueType::GRAPHICS)]); }
  bool canWaitOnFrameProgress() { return static_cast<bool>(frameProgress); }

  // sync method name schema: synchronize<waiting queue>With<signaling queue>
  void synchronizeReadBackWithGraphics()
  {
    graphicsProgress.synchronize(group[static_cast<uint32_t>(DeviceQueueType::GRAPHICS)],
      group[static_cast<uint32_t>(DeviceQueueType::READ_BACK)]);
  }

  void synchronizeUploadWithGraphics()
  {
    graphicsProgress.synchronize(group[static_cast<uint32_t>(DeviceQueueType::GRAPHICS)],
      group[static_cast<uint32_t>(DeviceQueueType::UPLOAD)]);
  }

  void synchronizeGraphicsWithUpload()
  {
    uploadProgress.synchronize(group[static_cast<uint32_t>(DeviceQueueType::UPLOAD)],
      group[static_cast<uint32_t>(DeviceQueueType::GRAPHICS)]);
  }
};

// Some driver/device struggle with long running frames and then suddenly
// WaitForSingleObject returns errors that make no sense.
// This functions handles this with some fall back strategies.
inline DAGOR_NOINLINE bool wait_for_frame_progress_with_event_slow_path(DeviceQueueGroup &qs, uint64_t progress, HANDLE event,
  const char *what)
{
#if LOCK_PROFILER_ENABLED
  using namespace da_profiler;
  static desc_id_t wffpdesc =
    add_description(DA_PROFILE_FILE_NAMES ? __FILE__ : nullptr, __LINE__, IsWait, "DX12_waitForFrameProgress");
  ScopeLockProfiler<da_profiler::NoDesc> lp(wffpdesc);
#endif
  if (!qs.waitForFrameProgress(progress, event))
  {
    return false;
  }
  auto result = WaitForSingleObject(event, INFINITE);
  if (WAIT_OBJECT_0 == result)
  {
    return true;
  }
  if (EASTL_UNLIKELY(WAIT_FAILED == result))
  {
    logwarn("DX12: While waiting for frame progress %u - %s, WaitForSingleObject(%p, INFINITE) "
            "failed with 0x%08X, trying to continue with alternatives",
      progress, what, event, GetLastError());
  }
  if (qs.checkFrameProgress() >= progress)
  {
    debug("DX12: Progress check allowed continuation");
    return true;
  }
  debug("DX12: Calling SetEventOnCompletion with nullptr");
  return qs.waitForFrameProgress(progress, nullptr);
}

__forceinline bool wait_for_frame_progress_with_event(DeviceQueueGroup &qs, uint64_t progress, HANDLE event, const char *what)
{
  if (EASTL_LIKELY(qs.checkFrameProgress() >= progress))
    return true;
  return wait_for_frame_progress_with_event_slow_path(qs, progress, event, what);
}

} // namespace drv3d_dx12
