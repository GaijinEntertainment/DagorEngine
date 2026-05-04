// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "heap_components.h"
#include <device.h>
#include <ioSys/dag_dataBlock.h>
#include <3d/tql.h>


using namespace drv3d_dx12;
using namespace drv3d_dx12::resource_manager;

namespace
{
constexpr float usageSizeReportingThreshold = 0.05f;

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
  if ((lockedHeapIndex < group.size()) && !((heapIndex < group.size()) && group[heapIndex].isValidRange(selectedRange)))
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
      [budget = getAvailablePoolBudget(device_local_memory_pool)](auto level) //
      { return level >= budget; });
  poolBudgetLevelstatus[device_local_memory_pool] =
    static_cast<BudgetPressureLevels>(ref - eastl::begin(poolBudgetLevels[device_local_memory_pool]));

  ref = eastl::find_if(eastl::begin(poolBudgetLevels[host_local_memory_pool]), eastl::end(poolBudgetLevels[host_local_memory_pool]),
    [budget = getAvailablePoolBudget(host_local_memory_pool)](auto level) //
    { return level >= budget; });
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
  if (getFeatureSet().isUMA)
  {
    // on UMA everything is device memory
    poolStates[device_local_memory_pool].reportedSize += adapterDesc.DedicatedSystemMemory;
    poolStates[device_local_memory_pool].reportedSize += adapterDesc.SharedSystemMemory;
  }

  ByteUnits deviceLocalMemoryPoolSize = poolStates[device_local_memory_pool].reportedSize;
  logdbg("DX12: Computed device local memory pool size %.3f %s", deviceLocalMemoryPoolSize.units(), deviceLocalMemoryPoolSize.name());

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
bool is_memory_size_equal(uint64_t reported_size, uint64_t new_size, float threshold)
{
  auto reportedSizeUnits = size_to_unit_table(reported_size);
  auto newSizeUnits = size_to_unit_table(new_size);
  if (reportedSizeUnits != newSizeUnits)
    return false;
  if (fabsf(compute_unit_type_size(reported_size, reportedSizeUnits) - compute_unit_type_size(new_size, newSizeUnits)) > threshold)
  {
    return false;
  }
  return true;
}

bool is_video_memory_info_equal(const DXGI_QUERY_VIDEO_MEMORY_INFO &reported_info, const DXGI_QUERY_VIDEO_MEMORY_INFO &new_info,
  float threshold)
{
  if ((reported_info.CurrentUsage > reported_info.Budget) != (new_info.CurrentUsage > new_info.Budget))
    return false;
  if (!is_memory_size_equal(reported_info.Budget, new_info.Budget, threshold))
    return false;
  if (!is_memory_size_equal(reported_info.CurrentUsage, new_info.CurrentUsage, threshold))
    return false;
  if (!is_memory_size_equal(reported_info.AvailableForReservation, new_info.AvailableForReservation, threshold))
    return false;
  if (!is_memory_size_equal(reported_info.CurrentReservation, new_info.CurrentReservation, threshold))
    return false;
  return true;
}

void report_budget_info(const DXGI_QUERY_VIDEO_MEMORY_INFO &info, const char *name, const char *level)
{
  logdbg("DX12: QueryVideoMemoryInfo of %s:", name);
  auto BudgetUnits = size_to_unit_table(info.Budget);
  logdbg("DX12: Budget %.2f %s", compute_unit_type_size(info.Budget, BudgetUnits), get_unit_name(BudgetUnits));
  auto CurrentUsageUnits = size_to_unit_table(info.CurrentUsage);
  logdbg("DX12: CurrentUsage %.2f %s (level: %s)", compute_unit_type_size(info.CurrentUsage, CurrentUsageUnits),
    get_unit_name(CurrentUsageUnits), level);
  auto AvailableForReservationUnits = size_to_unit_table(info.AvailableForReservation);
  logdbg("DX12: AvailableForReservation %.2f %s", compute_unit_type_size(info.AvailableForReservation, AvailableForReservationUnits),
    get_unit_name(AvailableForReservationUnits));
  auto CurrentReservationUnits = size_to_unit_table(info.CurrentReservation);
  logdbg("DX12: CurrentReservation %.2f %s", compute_unit_type_size(info.CurrentReservation, CurrentReservationUnits),
    get_unit_name(CurrentReservationUnits));
}

void update_reservation(DXGIAdapter *adapter, DXGI_QUERY_VIDEO_MEMORY_INFO &info, DXGI_MEMORY_SEGMENT_GROUP group, const char *name,
  bool should_print)
{
  if (info.CurrentReservation != info.CurrentUsage)
  {
    auto nextReserve = min(info.CurrentUsage, info.AvailableForReservation);
    auto nextReserveUnits = size_to_unit_table(nextReserve);
    if (info.CurrentUsage > info.AvailableForReservation && should_print)
    {
      auto usageUnits = size_to_unit_table(info.CurrentUsage);
      logwarn("DX12: %s memory usage %.2f %s is larger memory available for reservation %.2f %s", name,
        compute_unit_type_size(info.CurrentUsage, usageUnits), get_unit_name(usageUnits),
        compute_unit_type_size(nextReserve, nextReserveUnits), get_unit_name(nextReserveUnits));
    }
    if (nextReserve != info.CurrentReservation)
    {
      if (should_print)
      {
        logdbg("DX12: Adjusted %s memory reservation to %.2f %s", name, compute_unit_type_size(nextReserve, nextReserveUnits),
          get_unit_name(nextReserveUnits));
      }
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

    const bool isEqual = is_video_memory_info_equal(reportedPoolStates[device_local_memory_pool], poolStates[device_local_memory_pool],
      usageSizeReportingThreshold);
    if (!isEqual)
      report_budget_info(poolStates[device_local_memory_pool], "Device Local", as_string(getDeviceLocalBudgetLevel()));
    update_reservation(adapter, poolStates[device_local_memory_pool], DXGI_MEMORY_SEGMENT_GROUP_LOCAL, "Device Local", !isEqual);
    if (!isEqual)
      reportedPoolStates[device_local_memory_pool] = poolStates[device_local_memory_pool];
  }
  if (!behaviorStatus.test(BehaviorBits::DISABLE_HOST_MEMORY_STATUS_QUERY) && adapter)
  {
    adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &poolStates[host_local_memory_pool]);
    behaviorStatus.set(BehaviorBits::DISABLE_HOST_MEMORY_STATUS_QUERY);
    behaviorStatus.reset(BehaviorBits::DISABLE_VIRTUAL_ADDRESS_SPACE_STATUS_QUERY);

    const bool isEqual = is_video_memory_info_equal(reportedPoolStates[host_local_memory_pool], poolStates[host_local_memory_pool],
      usageSizeReportingThreshold);
    if (!isEqual)
      report_budget_info(poolStates[host_local_memory_pool], "Host Local", as_string(getHostLocalBudgetLevel()));
    update_reservation(adapter, poolStates[host_local_memory_pool], DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, "Host Local", !isEqual);
    if (!isEqual)
      reportedPoolStates[host_local_memory_pool] = poolStates[host_local_memory_pool];
  }

  if (!behaviorStatus.test(BehaviorBits::DISABLE_VIRTUAL_ADDRESS_SPACE_STATUS_QUERY))
  {
    MEMORYSTATUSEX systemMemoryStatus{sizeof(MEMORYSTATUSEX)};
    GlobalMemoryStatusEx(&systemMemoryStatus);
    // PROCESS_MEMORY_COUNTERS_EX memUsage{sizeof(PROCESS_MEMORY_COUNTERS_EX)};
    // GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS
    // *>(&memUsage),
    //                     sizeof(memUsage));

    processVirtualAddressUse = systemMemoryStatus.ullTotalVirtual - systemMemoryStatus.ullAvailVirtual;
    behaviorStatus.set(BehaviorBits::DISABLE_VIRTUAL_ADDRESS_SPACE_STATUS_QUERY);

    logdbg("DX12: GlobalMemoryStatusEx report:");
    logdbg("DX12: dwLoad: %u", systemMemoryStatus.dwMemoryLoad);
    ByteUnits totalPhys = systemMemoryStatus.ullTotalPhys;
    logdbg("DX12: ullTotalPhys: %.3f %s", totalPhys.units(), totalPhys.name());
    ByteUnits availPhys = systemMemoryStatus.ullAvailPhys;
    logdbg("DX12: ullAvailPhys: %.3f %s", availPhys.units(), availPhys.name());
    ByteUnits totalPage = systemMemoryStatus.ullTotalPageFile;
    logdbg("DX12: ullTotalPageFile: %.3f %s", totalPage.units(), totalPage.name());
    ByteUnits availPage = systemMemoryStatus.ullAvailPageFile;
    logdbg("DX12: ullAvailPageFile: %.3f %s", availPage.units(), availPage.name());
    ByteUnits totalVirtual = systemMemoryStatus.ullTotalVirtual;
    logdbg("DX12: ullTotalVirtual: %.3f %s", totalVirtual.units(), totalVirtual.name());
    ByteUnits availVirtual = systemMemoryStatus.ullAvailVirtual;
    logdbg("DX12: ullAvailVirtual: %.3f %s", availVirtual.units(), availVirtual.name());
  }

  auto oldTrimFramePushRingBuffer = shouldTrimFramePushRingBuffer();
  auto oldTrimUploadRingBuffer = shouldTrimUploadRingBuffer();
  auto oldDeviceLocalBudgetLevel = getDeviceLocalBudgetLevel();

  updateBudgetLevelStatus();

  if (getDeviceLocalBudgetLevel() <= BudgetPressureLevels::PANIC && oldDeviceLocalBudgetLevel > BudgetPressureLevels::PANIC)
  {
    logwarn("DX12: Device local budget reached PANIC level, scheduling texture memory trim");
    tql::schedule_trim_discardable_tex_mem();
  }

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
  uint64_t pageCount;
  uint64_t scale;

  if (ResourceHeapProperties::MemoryTypes::HostWriteBack == properties.type)
  {
    pageCount = read_back_page_count;
    scale = read_back_heap_size_scale;
  }
  else if (ResourceHeapProperties::MemoryTypes::HostWriteCombine == properties.type)
  {
    pageCount = upload_page_count;
    scale = upload_heap_size_scale;
  }
  else
  {
    if (0 == (D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES & properties.getFlags(getFeatureSet())))
    {
      pageCount = static_texture_page_count;
      scale = static_texture_heap_size_scale;
    }
    else if (0 == (D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES & properties.getFlags(getFeatureSet())))
    {
      pageCount = rtdsv_texture_page_count;
      scale = rtdsv_texture_heap_size_scale;
    }
    else // if (0 == (D3D12_HEAP_FLAG_DENY_BUFFERS & properties.getFlags(getFeatureSet())))
    {
      pageCount = buffer_page_count;
      scale = buffer_heap_size_scale;
    }
  }

  uint64_t budget;
  uint64_t totalBudget;
  if (properties.isOnDevice(getFeatureSet()))
  {
    budget = getDeviceLocalAvailablePoolBudget();
    totalBudget = getDeviceLocalBudget();
  }
  else
  {
    budget = getHostLocalAvailablePoolBudget();
    totalBudget = getHostLocalBudget();
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
    [budget = getDeviceLocalAvailablePoolBudget()](auto level) //
    { return level >= budget; });
  budgetPressureLevel = static_cast<BudgetPressureLevels>(ref - eastl::begin(poolBudgetLevels));
}

void MemoryBudgetObserver::setup(const SetupInfo &info)
{
  BaseType::setup(info);

  size_t gameLimit = 0, gameUsed = 0;
  xbox_get_memory_status(gameUsed, gameLimit);
  auto totalMemoryBudgetUnits = size_to_unit_table(gameLimit);
  logdbg("DX12: Game memory budget is %.2f %s (%I64u bytes)", compute_unit_type_size(gameLimit, totalMemoryBudgetUnits),
    get_unit_name(totalMemoryBudgetUnits), gameLimit);

  memoryBudget = gameLimit;
  currentUsage = gameUsed;
  checkEnoughSizeForRender(gameLimit);

#if _TARGET_SCARLETT
  static constexpr uint64_t budget_limit_base_mib = 512;
  static constexpr uint64_t memory_budget_devider = 16;
#else
  static constexpr uint64_t budget_limit_base_mib = 1024;
  static constexpr uint64_t memory_budget_devider = 8;
#endif

  for (auto bl : {BudgetPressureLevels::PANIC, BudgetPressureLevels::HIGH, BudgetPressureLevels::MEDIUM})
  {
    auto i = as_uint(bl);
    poolBudgetLevels[i] = min(budget_limit_base_mib * (1 + i) * 1024 * 1024, memoryBudget / (memory_budget_devider / (1 + i)));

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
    checkEnoughSizeForRender(gameLimit);
    behaviorStatus.set(BehaviorBits::DISABLE_MEMORY_STATUS_QUERY);
    const auto currentUsageUnitsIndex = size_to_unit_table(currentUsage);
    const auto currentUsageUnitsSize = compute_unit_type_size(currentUsage, currentUsageUnitsIndex);
    const auto lastUsageUnitsIndex = size_to_unit_table(lastReportedUsage);
    const auto lastUsageUnitsSize = compute_unit_type_size(lastReportedUsage, lastUsageUnitsIndex);
    const bool isOverBudget = gameUsed > gameLimit;
    const bool wasOverBudget = lastReportedUsage > memoryBudget;
    if (fabsf(lastUsageUnitsSize - currentUsageUnitsSize) > usageSizeReportingThreshold ||
        lastUsageUnitsIndex != currentUsageUnitsIndex || isOverBudget != wasOverBudget)
    {
      logdbg("DX12: Current memory budget usage is %u%% %.2f %s (level: %s)", gameUsed * 100 / gameLimit, currentUsageUnitsSize,
        get_unit_name(currentUsageUnitsIndex), as_string(getDeviceLocalBudgetLevel()));
      lastReportedUsage = currentUsage;
    }
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

void MemoryBudgetObserver::checkEnoughSizeForRender(uint64_t game_limit) const
{
  if (renderMemoryLimitBytes == 0)
    return; // no dx12 limit configured
  G_ASSERT(renderMemoryUsedBytes <= currentUsage);
  const uint64_t freeSize = game_limit - currentUsage;
  const uint64_t freeSizeRenderReserved =
    renderMemoryLimitBytes > renderMemoryUsedBytes ? renderMemoryLimitBytes - renderMemoryUsedBytes : 0;
  if (freeSize < freeSizeRenderReserved)
  {
    D3D_ERROR_ONCE("DX12: Not enough size for render reservation (free: %llu, render usage: %llu, render reserved: %llu)", freeSize,
      renderMemoryUsedBytes, renderMemoryLimitBytes);
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
  uint64_t maxPerHeapBudget = align_value(totalBudget * scale / budget_scale_divider, page_size);

  // dedicated heaps do not scale
  if (flags.test(AllocationFlag::DEDICATED_HEAP))
  {
    scale = 1;
    minSize = page_size;
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

  const auto pressureLevel = getDeviceLocalBudgetLevel();
  if ((BudgetPressureLevels::HIGH == pressureLevel) || (BudgetPressureLevels::PANIC == pressureLevel))
  {
    scale = min(scale, max_high_memory_pressure_heap_size_scale);
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
  const bool hasUploadHeapsGPU = getFeatureSet().hasUploadHeapsGPU;
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
          if (gpuHeapMemoryMappingConfig.mapRenderTargetTextureMSAAMemory && hasUploadHeapsGPU && getFeatureSet().resourceHeapTier2)
          {
            result.setAnyWriteCombinedGPUMemory(getFeatureSet());
          }
          else
          {
            result.setMSAARenderTargetMemory();
          }
        }
        else
        {
          if (gpuHeapMemoryMappingConfig.mapRenderTargetTextureMemory && hasUploadHeapsGPU && getFeatureSet().resourceHeapTier2)
          {
            result.setAnyWriteCombinedGPUMemory(getFeatureSet());
          }
          else
          {
            result.setRenderTargetMemory(getFeatureSet());
          }
        }
      }
      else
      {
        if (gpuHeapMemoryMappingConfig.mapTextureMemory && hasUploadHeapsGPU && getFeatureSet().resourceHeapTier2)
        {
          result.setAnyWriteCombinedGPUMemory(getFeatureSet());
        }
        else
        {
          result.setTextureMemory(getFeatureSet());
        }
      }
      break;
    case DeviceMemoryClass::DEVICE_RESIDENT_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapDeviceResidentBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferMemory(getFeatureSet());
      }
      break;
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapHostResidentHostReadOnlyBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteBackCPUMemory();
      }
      break;
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapHostResidentHostReadWriteBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteBackCPUMemory();
      }
      break;
    case DeviceMemoryClass::READ_BACK_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapReadBackBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteBackCPUMemory();
      }
      break;
    case DeviceMemoryClass::BIDIRECTIONAL_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapBidirectionalBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteBackCPUMemory();
      }
      break;
    case DeviceMemoryClass::TEMPORARY_UPLOAD_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapTemporaryUploadBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteCombinedCPUMemory(getFeatureSet());
      }
      break;
    case DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapHostResidentHostWriteOnlyBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteCombinedCPUMemory(getFeatureSet());
      }
      break;
    case DeviceMemoryClass::PUSH_RING_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapPushRingBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteCombinedCPUMemory(getFeatureSet());
      }
      break;
    case DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER:
      if (gpuHeapMemoryMappingConfig.mapDeviceResidentHostWriteOnlyBufferMemory && hasUploadHeapsGPU)
      {
        result.setAnyWriteCombinedGPUMemory(getFeatureSet());
      }
      else
      {
        result.setBufferWriteCombinedCPUMemory(getFeatureSet());
      }
      break;
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

ResourceMemory ResourceMemoryHeapProvider::tryAllocateFromMemoryWithProperties(ID3D12Device *device,
  ResourceHeapProperties heap_properties, AllocationFlags flags, const D3D12_RESOURCE_ALLOCATION_INFO &alloc_info, HRESULT *error_code)
{
  ResourceMemory result;
  auto hasSpace = getHeapGroupFreeMemorySize(heap_properties.raw) >= alloc_info.SizeInBytes;

  auto &group = groups[heap_properties.raw];

  if (!flags.test(AllocationFlag::DEDICATED_HEAP) && hasSpace)
  {
    struct SourceMaskType
    {
      AllocationFlags flags;
      bool canAllocateFromLockedRange() const { return !flags.test(AllocationFlag::DISALLOW_LOCKED_RANGES); }
    };
    result = try_allocate_from_heap_group(alloc_info, group, heap_properties.raw, SourceMaskType{flags});
  }

  if (!result && error_code != nullptr)
    *error_code = E_OUTOFMEMORY;

  if (!result && !flags.test(AllocationFlag::EXISTING_HEAPS_ONLY))
  {
    TIME_PROFILE_DEV(DX12_AllocateHeap);
    // neither a active nor a zombie heap could provide memory, create a new one
    D3D12_HEAP_DESC newHeapDesc;
    newHeapDesc.SizeInBytes = getHeapSizeFromAllocationSize(alloc_info.SizeInBytes, heap_properties, flags);
    newHeapDesc.Properties.Type = heap_properties.getHeapType();
    newHeapDesc.Properties.CPUPageProperty = heap_properties.getCpuPageProperty(getFeatureSet());
    newHeapDesc.Properties.MemoryPoolPreference = heap_properties.getMemoryPool(getFeatureSet());
    newHeapDesc.Properties.CreationNodeMask = 0;
    newHeapDesc.Properties.VisibleNodeMask = 0;
    newHeapDesc.Alignment = heap_properties.getAlignment();
    newHeapDesc.Flags = heap_properties.getFlags(getFeatureSet());

    G_ASSERT(newHeapDesc.SizeInBytes >= alloc_info.SizeInBytes);
    ResourceHeap newHeap;
    newHeap.totalSize = newHeapDesc.SizeInBytes;
    newHeap.maxFreeRange = newHeapDesc.SizeInBytes;
    G_ASSERT(newHeap.totalSize == newHeapDesc.SizeInBytes);
    newHeap.freeRanges.push_back(make_value_range(0ull, newHeap.totalSize));

    auto errorCode = DX12_CHECK_RESULT_NO_OOM_CHECK(device->CreateHeap(&newHeapDesc, COM_ARGS(&newHeap.heap)));
    if (error_code != nullptr)
      *error_code = errorCode;
    if (SUCCEEDED(errorCode) && newHeap.heap)
    {
      addHeapGroupFreeSpace(heap_properties.raw, newHeap.totalSize);

      HeapID heapID;
      G_STATIC_ASSERT(heapID.group_bits >= heap_properties.bits);
      heapID.group = heap_properties.raw;
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

      recordHeapAllocated(newHeap.totalSize, heap_properties.isOnDevice(getFeatureSet()));
    }
  }

  if (result)
  {
    subtractHeapGroupFreeSpace(heap_properties.raw, result.size());

    recordMemoryAllocated(result.size(), heap_properties.isOnDevice(getFeatureSet()));

    updateHeapGroupGeneration(heap_properties.raw);
  }

  return result;
}

ResourceMemory ResourceMemoryHeapProvider::allocate(DXGIAdapter *adapter, ID3D12Device *device, ResourceHeapProperties properties,
  const D3D12_RESOURCE_ALLOCATION_INFO &alloc_info, AllocationFlags flags, HRESULT *error_code)
{
  TIME_PROFILE_DEV(DX12_AllocateMemory);
  G_UNUSED(adapter);
  if (error_code != nullptr)
    *error_code = S_OK;

  OSSpinlockScopedLock lock{heapGroupMutex};

  if (flags.test(AllocationFlag::DEFRAGMENTATION_OPERATION) && !checkDefragmentationGeneration(properties.raw))
    return {};

  ResourceMemory result = tryAllocateFromMemoryWithProperties(device, properties, flags, alloc_info, error_code);

  if (!flags.test(AllocationFlag::DISABLE_ALTERNATE_HEAPS))
  {
    for (ResourceHeapProperties currentProperties = {}; !result && currentProperties.raw < groups.size(); currentProperties.raw++)
    {
      if (currentProperties.raw == properties.raw)
        continue;
      if (!properties.isCompatible(currentProperties, getFeatureSet(), flags.test(AllocationFlag::IS_UAV)))
        continue;
      result = tryAllocateFromMemoryWithProperties(device, currentProperties, flags, alloc_info, error_code);
      if (result)
        logwarn("DX12: Alternate heap allocation: size: %u, group: %u (requested group: %u)", alloc_info.SizeInBytes,
          currentProperties.raw, properties.raw);
    }
  }

  return result;
}

void ResourceMemoryHeapProvider::free(ResourceMemory allocation)
{
  TIME_PROFILE_DEV(DX12_FreeMemory);

  OSSpinlockScopedLock lock{heapGroupMutex};
  freeNoLock(allocation, true);
}

void ResourceMemoryHeapProvider::freeNoLock(ResourceMemory allocation, bool is_heap_deletion_allowed)
{
  auto heapID = allocation.getHeapID();
  auto &group = groups[heapID.group];
  auto &heap = group[heapID.index];
  G_ASSERTF(heap.heap.Get() == allocation.getHeap(),
    "DX12: Provided Heap %p for HeapID (%u, isAlias=%u, group=%u, index=%u) does not match heap table entry %p", allocation.getHeap(),
    heapID.raw, heapID.isAlias, heapID.group, heapID.index, heap.heap.Get());
  updateHeapGroupGeneration(heapID.group);
  addHeapGroupFreeSpace(heapID.group, allocation.size());

  ResourceHeapProperties properties;
  properties.raw = heapID.group;

  heap.free(allocation);
  recordMemoryFreed(allocation.size(), properties.isOnDevice(getFeatureSet()));
  if (is_heap_deletion_allowed && heap.isFree())
  {
    TIME_PROFILE_DEV(DX12_FreeHeap);
    subtractHeapGroupFreeSpace(heapID.group, heap.totalSize);

    recordHeapFreed(heap.totalSize, properties.isOnDevice(getFeatureSet()));
    heap = {};
  }
}

#endif

ResourceMemory ResourceMemoryHeapProvider::allocateMemoryInPlace(HeapID heap_id, uint32_t free_range_index,
  const D3D12_RESOURCE_ALLOCATION_INFO &allocInfo)
{
  auto &newGroup = groups[heap_id.group];
  auto &newHeap = newGroup[heap_id.index];
  const auto selectedRangeIt = newHeap.freeRanges.begin() + free_range_index;
  auto memory = newHeap.allocate(allocInfo, heap_id, selectedRangeIt);
  subtractHeapGroupFreeSpace(heap_id.group, memory.size());
#if _TARGET_XBOX
  const bool isGpu = XMEM_GPU_OPTIMAL_BANDWIDTH_REQUIRED & ResourceHeapProperties{.raw = heap_id.group}.getXMemoryFlags();
#else
  const bool isGpu = ResourceHeapProperties{.raw = heap_id.group}.isOnDevice(getFeatureSet());
#endif
  recordMemoryAllocated(memory.size(), isGpu);
  updateHeapGroupGeneration(heap_id.group);
  return memory;
}
