#include "device.h"
#include <ioSys/dag_dataBlock.h>

using namespace drv3d_dx12;
using namespace drv3d_dx12::resource_manager;

namespace
{
struct AlwaysConvertToTrueType
{
  constexpr explicit operator bool() const { return true; }
};
struct AlwaysConvertToFlaseType
{
  constexpr explicit operator bool() const { return false; }
};
struct DefaultMemorySourceMaskType
{
  constexpr bool canAllocateFromLockedHeap() const { return true; }
  constexpr bool canAllocateFromLockedRange() const { return true; }
};
// Tries to find the best place to allocate the needed amount of memory.
template <typename T, typename U>
ResourceMemory try_allocate_from_heap_group(const D3D12_RESOURCE_ALLOCATION_INFO &alloc_info, T &group, uint32_t group_index,
  U source_mask)
{
  ResourceMemory result;
  decltype(group[0].selectRange({}, AlwaysConvertToTrueType{})) selectedRange{};
  uint32_t heapIndex = 0;
  uint32_t lockedHeapIndex = group.size();
  // First try to find any heap that can provide memory.
  for (; heapIndex < group.size(); ++heapIndex)
  {
    auto &heap = group[heapIndex];
    if (!heap)
    {
      continue;
    }
    // Ignore locked heaps in this round.
    if (heap.isLocked())
    {
      // Store the index of the locked heap for later fallback.
      // There can only be one locked heap per group at a time, so simple index is enough.
      lockedHeapIndex = heapIndex;
      continue;
    }
    // Here we can ignore locked ranges as we ignore heaps with locked ranges.
    selectedRange = heap.selectRange(alloc_info, AlwaysConvertToTrueType{});
    if (heap.isValidRange(selectedRange))
    {
      break;
    }
  }
  // Now try to find a better match.
  for (uint32_t candidateIndex = heapIndex + 1; candidateIndex < group.size(); ++candidateIndex)
  {
    auto &candidate = group[candidateIndex];
    if (!candidate)
    {
      continue;
    }
    // Keep ignoring locked heaps.
    if (candidate.isLocked())
    {
      lockedHeapIndex = candidateIndex;
      continue;
    }
    // Here we can also ignore locked ranges as the heap has no locked range anyway.
    auto candidateRange = candidate.selectRange(alloc_info, AlwaysConvertToTrueType{});
    if (!candidate.isValidRange(candidateRange))
    {
      continue;
    }

    if (candidateRange->size() > selectedRange->size())
    {
      continue;
    }

    if (candidateRange->size() == selectedRange->size())
    {
      // prefer larger heaps
      if (candidate.totalSize < group[heapIndex].totalSize)
      {
        continue;
      }
    }

    heapIndex = candidateIndex;
    selectedRange = candidateRange;
  }

  // If nothing could be found but a locked heap exists we now try it with that heap.
  if (source_mask.canAllocateFromLockedHeap() && (lockedHeapIndex < group.size()) &&
      !((heapIndex < group.size()) && group[heapIndex].isValidRange(selectedRange)))
  {
    heapIndex = lockedHeapIndex;
    // First try to use ranges that are not covered by the locked range.
    selectedRange = group[heapIndex].selectRange(alloc_info, AlwaysConvertToFlaseType{});

    if (source_mask.canAllocateFromLockedRange() && !group[heapIndex].isValidRange(selectedRange))
    {
      // Now as a last attempt we try also ranges that are locked.
      selectedRange = group[heapIndex].selectRange(alloc_info, AlwaysConvertToTrueType{});
    }
  }

  if ((heapIndex < group.size()) && group[heapIndex].isValidRange(selectedRange))
  {
    HeapID heapID;
    heapID.group = group_index;
    heapID.index = heapIndex;
    result = group[heapIndex].allocate(alloc_info, heapID, selectedRange);
  }

  return result;
}

// Optimized first allocation of a fresh heap.
template <typename T>
ResourceMemory first_allocate_from_heap(const D3D12_RESOURCE_ALLOCATION_INFO &alloc_info, T &heap, HeapID heap_id)
{
  G_ASSERT_RETURN(!heap.freeRanges.empty(), {});
  G_ASSERT_RETURN(0 == heap.freeRanges.front().start, {});
  G_ASSERT_RETURN(heap.freeRanges.front().size() >= alloc_info.SizeInBytes, {});
  return heap.allocate(alloc_info, heap_id, begin(heap.freeRanges));
}
} // namespace

#if _TARGET_PC_WIN
void MemoryBudgetObserver::updateBudgetLevelStatus()
{
  auto ref =
    eastl::find_if(eastl::begin(poolBudgetLevels[device_local_memory_pool]), eastl::end(poolBudgetLevels[device_local_memory_pool]),
      [budged = getAvailablePoolBudget(device_local_memory_pool)](auto level) //
      { return level >= budged; });
  poolBudgetLevelstatus[device_local_memory_pool] =
    static_cast<BudgetPressureLevels>(ref - eastl::begin(poolBudgetLevels[device_local_memory_pool]));

  ref = eastl::find_if(eastl::begin(poolBudgetLevels[host_local_memory_pool]), eastl::end(poolBudgetLevels[host_local_memory_pool]),
    [budged = getAvailablePoolBudget(host_local_memory_pool)](auto level) //
    { return level >= budged; });
  poolBudgetLevelstatus[host_local_memory_pool] =
    static_cast<BudgetPressureLevels>(ref - eastl::begin(poolBudgetLevels[host_local_memory_pool]));
}

void MemoryBudgetObserver::setup(const SetupInfo &info)
{
  BaseType::setup(info);

  logdbg("DX12: Checking memory sizes...");
  DXGI_ADAPTER_DESC2 adapterDesc{};
  info.adapter->GetDesc2(&adapterDesc);
  auto DedicatedVideoMemoryUnits = size_to_unit_table(adapterDesc.DedicatedVideoMemory);
  logdbg("DX12: DedicatedVideoMemory %.2f %s", compute_unit_type_size(adapterDesc.DedicatedVideoMemory, DedicatedVideoMemoryUnits),
    get_unit_name(DedicatedVideoMemoryUnits));
  auto DedicatedSystemMemoryUnits = size_to_unit_table(adapterDesc.DedicatedSystemMemory);
  logdbg("DX12: DedicatedSystemMemory %.2f %s", compute_unit_type_size(adapterDesc.DedicatedSystemMemory, DedicatedSystemMemoryUnits),
    get_unit_name(DedicatedSystemMemoryUnits));
  auto SharedSystemMemoryUnits = size_to_unit_table(adapterDesc.SharedSystemMemory);
  logdbg("DX12: SharedSystemMemory %.2f %s", compute_unit_type_size(adapterDesc.SharedSystemMemory, SharedSystemMemoryUnits),
    get_unit_name(SharedSystemMemoryUnits));
  poolStates[device_local_memory_pool].reportedSize = adapterDesc.DedicatedVideoMemory;

  MEMORYSTATUSEX sysdtemMemoryStatus{sizeof(MEMORYSTATUSEX)};
  GlobalMemoryStatusEx(&sysdtemMemoryStatus);

  auto ullTotalPhysUnits = size_to_unit_table(sysdtemMemoryStatus.ullTotalPhys);
  logdbg("DX12: System memory %.2f %s", compute_unit_type_size(sysdtemMemoryStatus.ullTotalPhys, ullTotalPhysUnits),
    get_unit_name(ullTotalPhysUnits));
  auto ullTotalVirtualUnits = size_to_unit_table(sysdtemMemoryStatus.ullTotalVirtual);
  logdbg("DX12: Process usable system memory %.2f %s",
    compute_unit_type_size(sysdtemMemoryStatus.ullTotalVirtual, ullTotalVirtualUnits), get_unit_name(ullTotalVirtualUnits));

  poolStates[host_local_memory_pool].reportedSize = min(sysdtemMemoryStatus.ullTotalPhys, sysdtemMemoryStatus.ullTotalVirtual);

  processVirtualTotal = sysdtemMemoryStatus.ullTotalVirtual;

  info.adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &poolStates[device_local_memory_pool]);
  info.adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &poolStates[host_local_memory_pool]);

  behaviorStatus.set(BehaviorBits::DISABLE_DEVICE_MEMORY_STATUS_QUERY);
  behaviorStatus.set(BehaviorBits::DISABLE_HOST_MEMORY_STATUS_QUERY);

  const char *memorySectionNames[total_memory_pool_count];
  uint64_t minMemoryReq[total_memory_pool_count];
  minMemoryReq[device_local_memory_pool] = 128;
  memorySectionNames[device_local_memory_pool] = "Device Local";
  minMemoryReq[host_local_memory_pool] = 512;
  memorySectionNames[host_local_memory_pool] = "Host Local";

  for (auto bl : {BudgetPressureLevels::PANIC, BudgetPressureLevels::HIGH, BudgetPressureLevels::MEDIUM})
  {
    auto i = as_uint(bl);
    for (uint32_t j : {device_local_memory_pool, host_local_memory_pool})
    {
      poolBudgetLevels[j][i] = min(minMemoryReq[j] * (1 + i) * 1024 * 1024, poolStates[j].Budget / (16 / (1 + i)));

      auto units = size_to_unit_table(poolBudgetLevels[j][i]);
      logdbg("DX12: Budged Level %s for %s set to %.2f %s", as_string(bl), memorySectionNames[j],
        compute_unit_type_size(poolBudgetLevels[j][i], units), get_unit_name(units));
    }
  }

  updateBudgetLevelStatus();
}

namespace
{
void report_budget_info(const DXGI_QUERY_VIDEO_MEMORY_INFO &info, const char *name)
{
  logdbg("DX12: QueryVideoMemoryInfo of %s:", name);
  auto BudgetUnits = size_to_unit_table(info.Budget);
  logdbg("DX12: Budget %.2f %s", compute_unit_type_size(info.Budget, BudgetUnits), get_unit_name(BudgetUnits));
  auto CurrentUsageUnits = size_to_unit_table(info.CurrentUsage);
  logdbg("DX12: CurrentUsage %.2f %s", compute_unit_type_size(info.CurrentUsage, CurrentUsageUnits), get_unit_name(CurrentUsageUnits));
  auto AvailableForReservationUnits = size_to_unit_table(info.AvailableForReservation);
  logdbg("DX12: AvailableForReservation %.2f %s", compute_unit_type_size(info.AvailableForReservation, AvailableForReservationUnits),
    get_unit_name(AvailableForReservationUnits));
  auto CurrentReservationUnits = size_to_unit_table(info.CurrentReservation);
  logdbg("DX12: CurrentReservation %.2f %s", compute_unit_type_size(info.CurrentReservation, CurrentReservationUnits),
    get_unit_name(CurrentReservationUnits));
}

void update_reservation(DXGIAdapter *adapter, DXGI_QUERY_VIDEO_MEMORY_INFO &info, DXGI_MEMORY_SEGMENT_GROUP group, const char *name)
{
  if (info.CurrentReservation != info.CurrentUsage)
  {
    auto nextReserve = min(info.CurrentUsage, info.AvailableForReservation);
    auto nextReserveUnits = size_to_unit_table(nextReserve);
    if (info.CurrentUsage > info.AvailableForReservation)
    {
      auto usageUnits = size_to_unit_table(info.CurrentUsage);
      logwarn("DX12: %s memory usage %.2f %s is larger memory available for reservation %.2f %s", name,
        compute_unit_type_size(info.CurrentUsage, usageUnits), get_unit_name(usageUnits),
        compute_unit_type_size(nextReserve, nextReserveUnits), get_unit_name(nextReserveUnits));
    }
    if (nextReserve != info.CurrentReservation)
    {
      logdbg("DX12: Adjusted %s memory reservation to %.2f %s", name, compute_unit_type_size(nextReserve, nextReserveUnits),
        get_unit_name(nextReserveUnits));
      adapter->SetVideoMemoryReservation(0, group, nextReserve);
      info.CurrentReservation = nextReserve;
    }
  }
}
} // namespace

void MemoryBudgetObserver::completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
{
  BaseType::completeFrameExecution(info, data);

  auto adapter = info.adapter;

  if (!behaviorStatus.test(BehaviorBits::DISABLE_DEVICE_MEMORY_STATUS_QUERY) && adapter)
  {
    adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &poolStates[device_local_memory_pool]);
    behaviorStatus.set(BehaviorBits::DISABLE_DEVICE_MEMORY_STATUS_QUERY);

    report_budget_info(poolStates[device_local_memory_pool], "Device Local");
    update_reservation(adapter, poolStates[device_local_memory_pool], DXGI_MEMORY_SEGMENT_GROUP_LOCAL, "Device Local");
  }
  if (!behaviorStatus.test(BehaviorBits::DISABLE_HOST_MEMORY_STATUS_QUERY) && adapter)
  {
    adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &poolStates[host_local_memory_pool]);
    behaviorStatus.set(BehaviorBits::DISABLE_HOST_MEMORY_STATUS_QUERY);
    behaviorStatus.reset(BehaviorBits::DISABLE_VIRTUAL_ADDRESS_SPACE_STATUS_QUERY);

    report_budget_info(poolStates[host_local_memory_pool], "Host Local");
    update_reservation(adapter, poolStates[host_local_memory_pool], DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, "Host Local");
  }

  if (!behaviorStatus.test(BehaviorBits::DISABLE_VIRTUAL_ADDRESS_SPACE_STATUS_QUERY))
  {
    MEMORYSTATUSEX sysdtemMemoryStatus{sizeof(MEMORYSTATUSEX)};
    GlobalMemoryStatusEx(&sysdtemMemoryStatus);
    // PROCESS_MEMORY_COUNTERS_EX memUsage{sizeof(PROCESS_MEMORY_COUNTERS_EX)};
    // GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS
    // *>(&memUsage),
    //                     sizeof(memUsage));

    processVirtualAddressUse = sysdtemMemoryStatus.ullTotalVirtual - sysdtemMemoryStatus.ullAvailVirtual;
    behaviorStatus.set(BehaviorBits::DISABLE_VIRTUAL_ADDRESS_SPACE_STATUS_QUERY);

    auto processVirtualAddressUseUnits = size_to_unit_table(processVirtualAddressUse);
    logdbg("DX12: Process virtual memory usage %.2f %s",
      compute_unit_type_size(processVirtualAddressUse, processVirtualAddressUseUnits), get_unit_name(processVirtualAddressUseUnits));
  }

  auto oldTrimFramePushRingBuffer = shouldTrimUploadRingBuffer();
  auto oldTrimUploadRingBuffer = shouldTrimFramePushRingBuffer();

  updateBudgetLevelStatus();

  if (oldTrimFramePushRingBuffer != shouldTrimUploadRingBuffer())
  {
    if (!oldTrimFramePushRingBuffer)
    {
      logdbg("DX12: Starting to trim FramePushRingBuffer");
    }
    else
    {
      logdbg("DX12: Stopped to trim FramePushRingBuffer");
    }
  }
  if (oldTrimUploadRingBuffer != shouldTrimFramePushRingBuffer())
  {
    if (!oldTrimUploadRingBuffer)
    {
      logdbg("DX12: Starting to trim UploadRingBuffer");
    }
    else
    {
      logdbg("DX12: Stopped to trim UploadRingBuffer");
    }
  }
}

uint64_t MemoryBudgetObserver::getHeapSizeFromAllocationSize(uint64_t size, ResourceHeapProperties properties, AllocationFlags flags)
{
  const bool isCPUCachedMemory = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK == properties.getCpuPageProperty(isUMASystem());
  const bool isCPUMemory = D3D12_MEMORY_POOL_L0 == properties.getMemoryPool(isUMASystem());
  const bool allowsBuffers = 0 == (D3D12_HEAP_FLAG_DENY_BUFFERS & properties.getFlags(canMixResources()));
  const bool allowsRTDSV = 0 == (D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES & properties.getFlags(canMixResources()));
  const bool allowsStaticTextures = 0 == (D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES & properties.getFlags(canMixResources()));

  uint64_t pageCount = 0;
  uint64_t scale = 0;
  uint64_t budget = 0;
  uint64_t totalBudget = 0;

  if (isCPUMemory)
  {
    if (isCPUCachedMemory)
    {
      pageCount = read_back_page_count;
      scale = read_back_heap_size_scale;
    }
    else
    {
      pageCount = upload_page_count;
      scale = upload_heap_size_scale;
    }

    budget = getHostLocalAvailablePoolBudget();
    totalBudget = getHostLocalBudget();
  }
  else
  {
    if (allowsStaticTextures)
    {
      pageCount = static_texture_page_count;
      scale = static_texture_heap_size_scale;
    }
    else if (allowsRTDSV)
    {
      pageCount = rtdsv_texture_page_count;
      scale = rtdsv_texture_heap_size_scale;
    }
    else
    {
      G_UNUSED(allowsBuffers);
      pageCount = buffer_page_count;
      scale = buffer_heap_size_scale;
    }

    budget = getDeviceLocalAvailablePoolBudget();
    totalBudget = getDeviceLocalBudget();
  }

  // we have a max budget per heap type, heap size calculation can only exceed this value if size is
  // larger
  uint64_t maxPerHeapBudget = align_value(totalBudget * scale / budget_scale_range, page_size);

  // dedicated heaps do not scale
  if (flags.test(AllocationFlag::DEDICATED_HEAP))
  {
    scale = 1;
    pageCount = 1;
  }

  // ensure budget is aligned *down* to page size
  budget = budget & ~(page_size - 1);
  if (budget <= size)
  {
    auto sizeUnits = size_to_unit_table(size);
    auto budgetUnits = size_to_unit_table(budget);
    logwarn("DX12: Allocating heap for %.2f %s, which is more than the available budget of %.2f %s",
      compute_unit_type_size(size, sizeUnits), get_unit_name(sizeUnits), compute_unit_type_size(budget, budgetUnits),
      get_unit_name(budgetUnits));
    budget = align_value(size, page_size);
  }
  else if (maxPerHeapBudget > size)
  {
    budget = min(budget, maxPerHeapBudget);
  }

  return min(budget, align_value(align_value(size * scale, pageCount * page_size), properties.getAlignment()));
}

#else

void MemoryBudgetObserver::updateBudgetLevelStatus()
{
  auto ref = eastl::find_if(eastl::begin(poolBudgetLevels), eastl::end(poolBudgetLevels),
    [budged = getDeviceLocalAvailablePoolBudget()](auto level) //
    { return level >= budged; });
  budgedPressureLevel = static_cast<BudgetPressureLevels>(ref - eastl::begin(poolBudgetLevels));
}

void MemoryBudgetObserver::setup(const SetupInfo &info)
{
  BaseType::setup(info);

  size_t gameLimit = 0, gameUsed = 0;
  xbox_get_memory_status(gameUsed, gameLimit);
  auto totalMemoryBudgedUnits = size_to_unit_table(gameLimit);
  logdbg("DX12: Game memory budget is %.2f %s (%I64u bytes)", compute_unit_type_size(gameLimit, totalMemoryBudgedUnits),
    get_unit_name(totalMemoryBudgedUnits), gameLimit);

  memoryBudget = gameLimit;
  currentUsage = gameUsed;

  static constexpr uint64_t budget_limit_base_mib = 512;

  for (auto bl : {BudgetPressureLevels::PANIC, BudgetPressureLevels::HIGH, BudgetPressureLevels::MEDIUM})
  {
    auto i = as_uint(bl);
    poolBudgetLevels[i] = min(budget_limit_base_mib * (1 + i) * 1024 * 1024, memoryBudget / (16 / (1 + i)));

    auto units = size_to_unit_table(poolBudgetLevels[i]);
    logdbg("DX12: Budged Level %s set to %.2f %s", as_string(bl), compute_unit_type_size(poolBudgetLevels[i], units),
      get_unit_name(units));
  }

  updateBudgetLevelStatus();
}

void MemoryBudgetObserver::completeFrameExecution(const CompletedFrameExecutionInfo &info, PendingForCompletedFrameData &data)
{
  BaseType::completeFrameExecution(info, data);

  if (!behaviorStatus.test(BehaviorBits::DISABLE_MEMORY_STATUS_QUERY))
  {
    size_t gameLimit = 0, gameUsed = 0;
    xbox_get_memory_status(gameUsed, gameLimit);
    currentUsage = gameUsed;
    auto currentUsageUnits = size_to_unit_table(currentUsage);
    behaviorStatus.set(BehaviorBits::DISABLE_MEMORY_STATUS_QUERY);
    logdbg("DX12: Current memory budget usage is %u%% %.2f %s", gameUsed * 100 / gameLimit,
      compute_unit_type_size(currentUsage, currentUsageUnits), get_unit_name(currentUsageUnits));
  }

  auto oldTrimFramePushRingBuffer = shouldTrimFramePushRingBuffer();
  auto oldTrimUploadRingBuffer = shouldTrimUploadRingBuffer();

  updateBudgetLevelStatus();

  if (oldTrimFramePushRingBuffer != shouldTrimFramePushRingBuffer())
  {
    if (!oldTrimFramePushRingBuffer)
    {
      logdbg("DX12: Starting to trim FramePushRingBuffer");
    }
    else
    {
      logdbg("DX12: Stopped to trim FramePushRingBuffer");
    }
  }
  if (oldTrimUploadRingBuffer != shouldTrimUploadRingBuffer())
  {
    if (!oldTrimUploadRingBuffer)
    {
      logdbg("DX12: Starting to trim UploadRingBuffer");
    }
    else
    {
      logdbg("DX12: Stopped to trim UploadRingBuffer");
    }
  }
}

uint64_t MemoryBudgetObserver::getHeapSizeFromAllocationSize(uint64_t size, ResourceHeapProperties properties, AllocationFlags flags)
{
  uint64_t minSize = min_heap_size;
  uint64_t scale = heap_scale;

  if (properties.isGPUExecutable)
  {
    minSize = min_executable_heap_size;
    scale = executable_heap_size_scale;
  }
  else if (properties.isCPUCoherent)
  {
    minSize = min_cpu_coherent_heap_size;
    scale = cpu_coherent_heap_size_scale;
  }

  uint64_t budget = getDeviceLocalAvailablePoolBudget();
  uint64_t totalBudget = getDeviceLocalBudget();

  // we have a max budget per heap type, heap size calculation can only exceed this value if size is
  // larger
  uint64_t maxPerHeapBudget = align_value(totalBudget * scale / budget_scale_range, page_size);

  // dedicated heaps do not scale
  if (flags.test(AllocationFlag::DEDICATED_HEAP))
  {
    scale = 1;
  }

  // ensure budget is aligned *down* to page size
  budget = budget & ~(page_size - 1);
  if (budget <= size)
  {
    auto sizeUnits = size_to_unit_table(size);
    auto budgetUnits = size_to_unit_table(budget);
    logwarn("DX12: Allocating heap for %.2f %s, which is more than the available budget of %.2f %s",
      compute_unit_type_size(size, sizeUnits), get_unit_name(sizeUnits), compute_unit_type_size(budget, budgetUnits),
      get_unit_name(budgetUnits));
    budget = align_value(size, page_size);
  }
  else if (maxPerHeapBudget > size)
  {
    budget = min(budget, maxPerHeapBudget);
  }

  return min(budget, align_value(size * scale, minSize));
}
#endif

#if _TARGET_XBOX
#include "resource_manager/heap_components_xbox.inl.cpp"
#else
D3D12_RESOURCE_STATES
ResourceMemoryHeapProvider::propertiesToInitialState(D3D12_RESOURCE_DIMENSION dim, D3D12_RESOURCE_FLAGS flags, DeviceMemoryClass)
{
  if (D3D12_RESOURCE_DIMENSION_BUFFER == dim)
  {
    return D3D12_RESOURCE_STATE_INITIAL_BUFFER_STATE;
  }
  else
  {
    if (flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET)
    {
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    else if (flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
    {
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    else if (flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
    {
      return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    else
    {
      return D3D12_RESOURCE_STATE_COPY_QUEUE_TARGET;
    }
  }
}

ResourceMemoryHeapProvider::ResourceHeapProperties ResourceMemoryHeapProvider::getProperties(D3D12_RESOURCE_FLAGS flags,
  DeviceMemoryClass memory_class, uint64_t alignment)
{
  ResourceHeapProperties result;
  switch (memory_class)
  {
    // host local textures only really work on UMA systems, esp the render target stuff
    // currently not used yet.
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_IMAGE:
    case DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_IMAGE:
    case DeviceMemoryClass::DEVICE_RESIDENT_IMAGE:
      if (flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
      {
        if (alignment > D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT)
        {
          result.setMSAARenderTargetMemory();
        }
        else
        {
          result.setRenderTargetMemory(canMixResources(), isUMASystem());
        }
      }
      else
      {
        result.setTextureMemory(canMixResources(), isUMASystem());
      }
      break;
#if D3D_HAS_RAY_TRACING
    case DeviceMemoryClass::DEVICE_RESIDENT_ACCELERATION_STRUCTURE:
#endif
    case DeviceMemoryClass::DEVICE_RESIDENT_BUFFER: result.setBufferMemory(isUMASystem()); break;
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER:
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER:
    case DeviceMemoryClass::READ_BACK_BUFFER:
    case DeviceMemoryClass::BIDIRECTIONAL_BUFFER: result.setBufferWriteBackCPUMemory(); break;
    // maps to dedicated upload heap type
    case DeviceMemoryClass::PUSH_RING_BUFFER:
    // also uses upload heap type
    case DeviceMemoryClass::TEMPORARY_UPLOAD_BUFFER:
    case DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER:
      // for now use a custom heap type to allow gpu copy into it to work
    case DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER: result.setBufferWriteCombinedCPUMemory(); break;
  }
  return result;
}

void ResourceMemoryHeapProvider::shutdown()
{
  BaseType::shutdown();

  OSSpinlockScopedLock lock{heapGroupMutex};
  for (auto &group : groups)
    group.clear();
}

void ResourceMemoryHeapProvider::preRecovery()
{
  BaseType::preRecovery();

  OSSpinlockScopedLock lock{heapGroupMutex};
  for (auto &group : groups)
    group.clear();
}

D3D12_RESOURCE_STATES
ResourceMemoryHeapProvider::getInitialTextureResourceState(D3D12_RESOURCE_FLAGS flags)
{
  // for all texture types this is the same
  return propertiesToInitialState(D3D12_RESOURCE_DIMENSION_TEXTURE2D, flags, DeviceMemoryClass::DEVICE_RESIDENT_IMAGE);
}

ResourceMemory ResourceMemoryHeapProvider::allocate(DXGIAdapter *adapter, ID3D12Device *device, ResourceHeapProperties properties,
  const D3D12_RESOURCE_ALLOCATION_INFO &alloc_info, AllocationFlags flags)
{
  TIME_PROFILE_DEV(DX12_AllocateMemory);
  G_UNUSED(adapter);
  ResourceMemory result;
  auto &group = groups[properties.raw];

  {
    OSSpinlockScopedLock lock{heapGroupMutex};
    auto hasSpace = getHeapGroupFreeMemorySize(properties.raw) >= alloc_info.SizeInBytes;

    if (!flags.test(AllocationFlag::DEDICATED_HEAP) && hasSpace)
    {
      struct SourceMaskType
      {
        AllocationFlags flags;
        bool canAllocateFromLockedHeap() const { return !flags.test(AllocationFlag::DISALLOW_LOCKED_HEAP); }
        bool canAllocateFromLockedRange() const { return !flags.test(AllocationFlag::DISALLOW_LOCKED_RANGES); }
      };
      result = try_allocate_from_heap_group(alloc_info, group, properties.raw, SourceMaskType{flags});
    }

#if _TARGET_PC_WIN
    // Budget limit is the maximum of available for reservation and ~66% of budget
    static const uint64_t budgetScaleNumerator = ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("budgetScaleNumerator", 2);
    static const uint64_t budgetScaleDenominator = ::dgs_get_settings()->getBlockByNameEx("dx12")->getInt("budgetScaleDenominator", 3);
    const uint64_t scaledBudget = budgetScaleNumerator * getDeviceLocalBudget() / budgetScaleDenominator;
    const uint64_t budgetLimit = eastl::max(getDeviceLocalAvailableForReservation(), scaledBudget);
    bool hasBudget = !flags.test(AllocationFlag::NEW_HEAPS_ONLY_WITH_BUDGET) || properties.isL0Pool || isUMASystem() ||
                     (poolStates[device_local_memory_pool].CurrentUsage < budgetLimit);
#else
    constexpr bool hasBudget = true;
#endif

    if (!result && !flags.test(AllocationFlag::EXISTING_HEAPS_ONLY) && hasBudget)
    {
      TIME_PROFILE_DEV(DX12_AllocateHeap);
      // neither a active nor a zombie heap could provide memory, create a new one
      D3D12_HEAP_DESC newHeapDesc;
      newHeapDesc.SizeInBytes = getHeapSizeFromAllocationSize(alloc_info.SizeInBytes, properties, flags);
      newHeapDesc.Properties.Type = properties.getHeapType();
      newHeapDesc.Properties.CPUPageProperty = properties.getCpuPageProperty(isUMASystem());
      newHeapDesc.Properties.MemoryPoolPreference = properties.getMemoryPool(isUMASystem());
      newHeapDesc.Properties.CreationNodeMask = 0;
      newHeapDesc.Properties.VisibleNodeMask = 0;
      newHeapDesc.Alignment = properties.getAlignment();
      newHeapDesc.Flags = properties.getFlags(canMixResources());

      G_ASSERT(newHeapDesc.SizeInBytes >= alloc_info.SizeInBytes);
      ResourceHeap newHeap;
      newHeap.totalSize = newHeapDesc.SizeInBytes;
      G_ASSERT(newHeap.totalSize == newHeapDesc.SizeInBytes);
      newHeap.freeRanges.push_back(make_value_range(0ull, newHeap.totalSize));

      auto errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(device->CreateHeap(&newHeapDesc, COM_ARGS(&newHeap.heap)));
      if (SUCCEEDED(errorCode) && newHeap.heap)
      {
        addHeapGroupFreeSpace(properties.raw, newHeap.totalSize);

        HeapID heapID;
        G_STATIC_ASSERT(heapID.group_bits >= properties.bits);
        heapID.group = properties.raw;
        for (heapID.index = 0; heapID.index < group.size(); ++heapID.index)
        {
          if (group[heapID.index])
          {
            continue;
          }
          group[heapID.index] = eastl::move(newHeap);
          break;
        }

        if (heapID.index == group.size())
        {
          group.push_back(eastl::move(newHeap));
        }

        auto &heap = group[heapID.index];
        result = first_allocate_from_heap(alloc_info, heap, heapID);

        HEAP_LOG("DX12: Allocated new memory heap %p with a size of %.2f %s", heap.heap.Get(), ByteUnits{heap.totalSize}.units(),
          ByteUnits{heap.totalSize}.name());

        recordHeapAllocated(newHeap.totalSize, D3D12_MEMORY_POOL_L1 == newHeapDesc.Properties.MemoryPoolPreference);

        if (!flags.test(AllocationFlag::DEDICATED_HEAP) && hasSpace)
        {
          updateHeapGroupNeedsDefragmentation(properties.raw, DefragmentationReason::FRAGMENTED_FREE_SPACE);
        }
      }
    }

    if (result)
    {
      subtractHeapGroupFreeSpace(properties.raw, result.size());

      recordMemoryAllocated(result.size(), D3D12_MEMORY_POOL_L1 == properties.getMemoryPool(isUMASystem()));

      updateHeapGroupGeneration(properties.raw);
    }
  }

  return result;
}

void ResourceMemoryHeapProvider::free(ResourceMemory allocation)
{
  TIME_PROFILE_DEV(DX12_FreeMemory);
  auto heapID = allocation.getHeapID();
  auto &group = groups[heapID.group];

  OSSpinlockScopedLock lock{heapGroupMutex};
  auto &heap = group[heapID.index];
  G_ASSERT(heap.heap.Get() == allocation.getHeap());
  updateHeapGroupGeneration(heapID.group);
  addHeapGroupFreeSpace(heapID.group, allocation.size());

  ResourceHeapProperties properties;
  properties.raw = heapID.group;

  heap.free(allocation);
  const bool isGPUMemory = D3D12_MEMORY_POOL_L1 == properties.getMemoryPool(isUMASystem());
  recordMemoryFreed(allocation.size(), isGPUMemory);
  if (heap.isFree())
  {
    TIME_PROFILE_DEV(DX12_FreeHeap);
    subtractHeapGroupFreeSpace(heapID.group, heap.totalSize);

    recordHeapFreed(heap.totalSize, isGPUMemory);
    heap = {};
  }
  else
  {
    auto freeSize = getHeapGroupFreeMemorySize(heapID.group);
    auto ref = eastl::find_if(begin(group), end(group),
      [freeSize](auto &heap) { return static_cast<bool>(heap) && heap.totalSize <= freeSize; });
    if (ref != end(group))
    {
      updateHeapGroupNeedsDefragmentation(heapID.group, DefragmentationReason::FREE_UNUSED_HEAPS);
    }
  }
}

#endif
