// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device_memory.h"
#include <generic/dag_sort.h>
#include <perfMon/dag_statDrv.h>

#include "globals.h"
#include "resource_manager.h"

using namespace drv3d_vulkan;

namespace
{

struct MemoryPropertiesTableEntry
{
  VkMemoryPropertyFlags required[static_cast<uint32_t>(DeviceMemoryConfiguration::COUNT)];
  VkMemoryPropertyFlags desiered[static_cast<uint32_t>(DeviceMemoryConfiguration::COUNT)];
  VkMemoryPropertyFlags undesiered[static_cast<uint32_t>(DeviceMemoryConfiguration::COUNT)];
};

#if _TARGET_C3








#else
#define MAKE_MEMORY_PROPERTY_ENTRY(req_ded, req_sh_a, req_sh_m, des_ded, des_sh_a, des_sh_m, udes_ded, udes_sh_a, udes_sh_m) \
  {                                                                                                                          \
    {req_ded, req_sh_a, req_sh_m}, {des_ded, des_sh_a, des_sh_m},                                                            \
    {                                                                                                                        \
      udes_ded, udes_sh_a, udes_sh_m                                                                                         \
    }                                                                                                                        \
  }
G_STATIC_ASSERT(static_cast<uint32_t>(DeviceMemoryConfiguration::COUNT) == 3);
#endif
static constexpr const MemoryPropertiesTableEntry memory_type_flag_table[static_cast<uint32_t>(DeviceMemoryClass::COUNT)] = //
  {
    // DEVICE_RESIDENT_IMAGE,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    // DEVICE_RESIDENT_BUFFER,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
        VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
    // HOST_RESIDENT_HOST_READ_WRITE_BUFFER,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0),
    // HOST_RESIDENT_HOST_READ_ONLY_BUFFER,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0),
#if !_TARGET_C3
    // HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0),
#endif
    // DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0, 0, 0),
    // TRANSIENT_IMAGE,
    MAKE_MEMORY_PROPERTY_ENTRY(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
      VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT),
};
#undef MAKE_MEMORY_PROPERTY_ENTRY

constexpr VkMemoryPropertyFlags get_required_flags(DeviceMemoryClass cls, DeviceMemoryConfiguration cfg)
{
  return memory_type_flag_table[static_cast<uint32_t>(cls)].required[static_cast<uint32_t>(cfg)];
}
constexpr VkMemoryPropertyFlags get_desired_flags(DeviceMemoryClass cls, DeviceMemoryConfiguration cfg)
{
  return memory_type_flag_table[static_cast<uint32_t>(cls)].desiered[static_cast<uint32_t>(cfg)];
}
constexpr VkMemoryPropertyFlags get_undesired_flags(DeviceMemoryClass cls, DeviceMemoryConfiguration cfg)
{
  return memory_type_flag_table[static_cast<uint32_t>(cls)].undesiered[static_cast<uint32_t>(cfg)];
}
const char *memory_class_name(DeviceMemoryClass cls)
{
  switch (cls)
  {
    case DeviceMemoryClass::DEVICE_RESIDENT_IMAGE: return "device resident image";
    case DeviceMemoryClass::DEVICE_RESIDENT_BUFFER: return "device resident buffer";
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER: return "host resident host read write buffer";
    case DeviceMemoryClass::HOST_RESIDENT_HOST_READ_ONLY_BUFFER: return "host resident host read only buffer";
#if !_TARGET_C3
    case DeviceMemoryClass::HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER: return "host resident host write only buffer";
#endif
    case DeviceMemoryClass::DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER: return "device resident host write only buffer";
    case DeviceMemoryClass::TRANSIENT_IMAGE: return "transient image";
    case DeviceMemoryClass::INVALID: DAG_FATAL("passed DeviceMemoryClass::INVALID"); return "";
  }
  return 0;
}
} // namespace

void DeviceMemoryPool::initClassType(DeviceMemoryClass cls)
{
#if _TARGET_C3

#endif

  const VkMemoryPropertyFlags required = get_required_flags(cls, memoryConfig);

  Tab<uint32_t> &target = classTypes[uint32_t(cls)];
  // can't be more than the total count of types
  target.reserve(types.size());
  for (uint32_t i = 0; i < types.size(); ++i)
  {
    if ((types[i].flags & required) != required)
      continue;
    target.push_back(i);
  }
  target.shrink_to_fit();
  sort(target, MemoryClassCompare(cls, memoryConfig, types));
  String typesDump(64, "vulkan: memory class '%s' uses [", memory_class_name(cls));
  for (auto &&index : target)
    typesDump.aprintf(4, "%u,", index);
  typesDump.pop_back();
  typesDump += "] memory types";
  debug(typesDump);
}

DeviceMemoryTypeRange DeviceMemoryPool::selectMemoryType(VkDeviceSize size, uint32_t mask, DeviceMemoryClass cls) const
{
  const Tab<uint32_t> &set = classTypes[uint32_t(cls)];
  for (uint32_t i = 0; i < set.size(); ++i)
  {
    const uint32_t index = set[i];
    const uint32_t bit = 1ul << index;
    if (mask & bit)
    {
      const Type &type = types[index];
      VkDeviceSize freeSpace = type.heap->size - type.heap->inUse;
      // no rom left, remove from mask
      if (freeSpace < size)
        mask ^= bit;
    }
  }

  return DeviceMemoryTypeRange(set, mask);
}

void DeviceMemoryPool::init(const VkPhysicalDeviceMemoryProperties &mem_info)
{
  const DataBlock *vkCfg = ::dgs_get_settings()->getBlockByNameEx("vulkan");

  heaps.resize(mem_info.memoryHeapCount);
  for (uint32_t i = 0; i < mem_info.memoryHeapCount; ++i)
  {
    const VkMemoryHeap &srcHeap = mem_info.memoryHeaps[i];
    Heap &dstHeap = heaps[i];
    dstHeap.size = srcHeap.size;
    dstHeap.flags = srcHeap.flags;
    dstHeap.index = i;
    dstHeap.inUse = 0;
    int64_t softLimitMb = vkCfg->getBlockByNameEx(String(64, "heap%u", i))->getInt64("softLimitMb", -1);
    if (softLimitMb >= 0)
    {
      dstHeap.limit = softLimitMb << 20;
      dstHeap.errorOnLimit = true;
    }
    else
    {
      dstHeap.limit = srcHeap.size;
      dstHeap.errorOnLimit = false;
    }

    if (srcHeap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
      deviceLocalLimit = max<VkDeviceSize>(dstHeap.limit, deviceLocalLimit);
    else
      hostLocalLimit = max<VkDeviceSize>(dstHeap.limit, hostLocalLimit);
  }

  types.resize(mem_info.memoryTypeCount);
  for (uint32_t i = 0; i < mem_info.memoryTypeCount; ++i)
  {
    const VkMemoryType &srcType = mem_info.memoryTypes[i];
    Type &dstType = types[i];
    dstType.heap = &heaps[srcType.heapIndex];
    dstType.flags = srcType.propertyFlags;
  }

#if !_TARGET_C3
  bool nonDeviceLocalHeapsPresent = false;
  for (const Heap &i : heaps)
    nonDeviceLocalHeapsPresent |= (i.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == 0;

  if (nonDeviceLocalHeapsPresent)
  {
    memoryConfig = DeviceMemoryConfiguration::DEDICATED_DEVICE_MEMORY;
  }
  else
  {
    constexpr VkMemoryPropertyFlags autoFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    auto ref = eastl::find_if(types.cbegin(), types.cend(),
      [=](const Type &type) //
      { return autoFlags == (type.flags & autoFlags); });
    if (ref != types.cend())
      memoryConfig = DeviceMemoryConfiguration::HOST_SHARED_MEMORY_AUTO;
    else
      memoryConfig = DeviceMemoryConfiguration::HOST_SHARED_MEMORY_MANUAL;
  }
#endif

  for (int i = 0; i < array_size(classTypes); ++i)
    initClassType(static_cast<DeviceMemoryClass>(i));
}

#if VK_EXT_memory_budget
void DeviceMemoryPool::applyBudget(const VkPhysicalDeviceMemoryBudgetPropertiesEXT &budget)
{
  G_ASSERT(heaps.size());
  VkDeviceSize oldDeviceLocalLimit = deviceLocalLimit;
  VkDeviceSize oldHostLocalLimit = hostLocalLimit;
  deviceLocalLimit = 0;
  hostLocalLimit = 0;

  for (uint32_t i = 0; i < heaps.size(); ++i)
  {
    Heap &iter = heaps[i];

    // apply only if we are not using config defined soft limit
    if (!iter.errorOnLimit)
    {
      if (!budget.heapBudget[i])
        debug("vulkan: initial budget for Heap %u is zero, silently ignoring it", i);
      iter.limit = budget.heapBudget[i];
    }

    if (iter.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
      deviceLocalLimit += iter.limit;
    else
      hostLocalLimit += iter.limit;
  }

  // avoid error if budget extension will treat heaps as overlapped
  deviceLocalLimit = min<VkDeviceSize>(deviceLocalLimit, oldDeviceLocalLimit);
  hostLocalLimit = min<VkDeviceSize>(hostLocalLimit, oldHostLocalLimit);
}
#endif

bool DeviceMemoryPool::checkAllocationLimits(const DeviceMemoryTypeAllocationInfo &info)
{
  const Heap *targetHeap = types[info.typeIndex].heap;

  // silently ignore unusable heaps
  if (!targetHeap->limit)
    return false;

  // check heap size limit
  if ((targetHeap->inUse + info.size) > targetHeap->size)
  {
    logAllocationError(info, "heap size limit");
    return false;
  }

  // do other limit checks only if heap limit is specified in config
  if (!targetHeap->errorOnLimit)
    return true;

  // check heap limit
  if ((targetHeap->inUse + info.size) > targetHeap->limit)
  {
    logAllocationError(info, "soft limit");
    return false;
  }

  // check total usage limit
  VkDeviceSize totalUsed = 0;
  uint32_t deviceLocalFlag = targetHeap->flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;

  for (uint32_t i = 0; i < heaps.size(); ++i)
  {
    Heap &iter = heaps[i];
    if ((iter.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) == deviceLocalFlag)
      totalUsed += iter.inUse;
  }

  if ((totalUsed + info.size) > (deviceLocalFlag ? deviceLocalLimit : hostLocalLimit))
  {
    logAllocationError(info, "summary soft limit");
    return false;
  }

  return true;
}

DeviceMemory DeviceMemoryPool::allocate(const DeviceMemoryClassAllocationInfo &info)
{
  DeviceMemory result;
  DeviceMemoryTypeAllocationInfo typedInfo;
  typedInfo.size = info.size;
  typedInfo.targetBuffer = info.targetBuffer;
  typedInfo.targetImage = info.targetImage;
  DeviceMemoryTypeRange memoryTypes = selectMemoryType(info.size, info.typeMask, info.memoryClass);
  for (uint32_t type : memoryTypes)
  {
    typedInfo.typeIndex = type;
    result = allocate(typedInfo);
    if (!is_null(result))
      break;
  }
  return result;
}

DeviceMemory DeviceMemoryPool::allocate(const DeviceMemoryTypeAllocationInfo &info)
{
  TIME_PROFILE(vulkan_mem_alloc);
  VulkanDevice &vkDev = Globals::VK::dev;

  DeviceMemory result;

  // do not allocate if we break budget/max size/soft defined limit
  if (!checkAllocationLimits(info))
    return result;

  const void *chainPtr = nullptr;
#if VK_KHR_dedicated_allocation
  const VkMemoryDedicatedAllocateInfoKHR dedKHRInfo = //
    {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO_KHR,
      nullptr,
      info.targetImage,
      info.targetBuffer,
    };
#endif

  if (!is_null(info.targetImage) || !is_null(info.targetBuffer))
  {
#if VK_KHR_dedicated_allocation
    if (vkDev.hasExtension<DedicatedAllocationKHR>())
      chainPtr = &dedKHRInfo;
#endif
  }

#if VK_KHR_buffer_device_address
  VkMemoryAllocateFlagsInfoKHR additionalFlags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR};
  if (vkDev.hasExtension<BufferDeviceAddressKHR>() && isDeviceLocalMemoryType(info.typeIndex))
  {
    additionalFlags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;
  }
  additionalFlags.pNext = chainPtr;
  chainPtr = &additionalFlags;
#endif

  const VkMemoryAllocateInfo allocInfo = //
    {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, chainPtr, info.size, info.typeIndex};

  const VkResult errorCode = VULKAN_CHECK_RESULT(vkDev.vkAllocateMemory(vkDev.get(), &allocInfo, nullptr, ptr(result.memory)));
  if (VULKAN_CHECK_OK(errorCode))
  {
    result.size = info.size;
    result.type = info.typeIndex;
    if (isHostVisibleMemoryType(info.typeIndex))
    {
      VULKAN_EXIT_ON_FAIL(vkDev.vkMapMemory(vkDev.get(), result.memory, 0, VK_WHOLE_SIZE, 0, (void **)&result.pointer));
      if (!result.pointer)
      {
        vkDev.vkFreeMemory(vkDev.get(), result.memory, nullptr);
        logAllocationError(info, "vulkan map memory silent fail (mapped to nullptr!)", true /*verbose*/);
        return DeviceMemory{};
      }
    }
    types[info.typeIndex].heap->inUse += info.size;
    ++allocations;
  }
  else
    logAllocationError(info, "vulkan error");
  return result;
}

void DeviceMemoryPool::free(const DeviceMemory &memory)
{
  VulkanDevice &vkDev = Globals::VK::dev;
  types[memory.type].heap->inUse -= memory.size;
  VULKAN_LOG_CALL(vkDev.vkFreeMemory(vkDev.get(), memory.memory, nullptr));
  ++frees;
}

void DeviceMemoryPool::printStats()
{
  debug("Vulkan memory stats:");
  VkDeviceSize totalSize = 0;
  for (uint32_t i = 0; i < heaps.size(); ++i)
  {
    const Heap &heap = heaps[i];
    const char *deviceLocalInfo = (heap.flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) ? "device local" : "host local";
    debug("Heap %u (%s): %s of %s in use (%f%%, %s max)", i, deviceLocalInfo, byte_size_unit(heap.inUse), byte_size_unit(heap.limit),
      float(heap.inUse) / heap.limit * 100.f, byte_size_unit(heap.size));
    totalSize += heap.inUse;
  }

  debug("%u allocations and %u frees leaves %u allocations in use", allocations, frees, allocations - frees);
  debug("%s in use, %s avrg allocation size", byte_size_unit(totalSize), byte_size_unit(totalSize / (allocations - frees)));

  Globals::Mem::res.printStats(false, false);

  uint32_t deltaAlloc = allocations - lastAllocationCount;
  uint32_t deltaFree = frees - lastFreeCount;
  debug("Allocation count delta: %u", deltaAlloc);
  debug("Free count delta: %u", deltaFree);
  lastAllocationCount += deltaAlloc;
  lastFreeCount += deltaFree;
}

void DeviceMemoryPool::logAllocationError(const DeviceMemoryTypeAllocationInfo &info, const char *reason, bool verbose)
{
  const Heap *targetHeap = types[info.typeIndex].heap;
  if (verbose)
    D3D_ERROR("vulkan: allocation %u failed due to [%s], asked %s of memory type %u from device heap %u (%u of %u(%u max) Mb used)",
      allocations + 1, reason, byte_size_unit(info.size), info.typeIndex, targetHeap->index, targetHeap->inUse >> 20,
      targetHeap->limit >> 20, targetHeap->size >> 20);
#if DAGOR_DBGLEVEL > 0
  else
    debug("vulkan: allocation %u failed due to [%s], asked %s of memory type %u from device heap %u (%u of %u(%u max) Mb used)",
      allocations + 1, reason, byte_size_unit(info.size), info.typeIndex, targetHeap->index, targetHeap->inUse >> 20,
      targetHeap->limit >> 20, targetHeap->size >> 20);
#endif
}

DeviceMemoryPool::MemoryClassCompare::MemoryClassCompare(DeviceMemoryClass cls, DeviceMemoryConfiguration cfg, Tab<Type> &t) : types(t)
{
  optional = get_desired_flags(cls, cfg);
  unwanted = get_undesired_flags(cls, cfg);
}
int DeviceMemoryPool::MemoryClassCompare::order(uint32_t l, uint32_t r) const
{
  const Type &lt = types[l];
  const Type &rt = types[r];
  const int lUnwantedCount = count_bits(lt.flags & unwanted);
  const int rUnwantedCount = count_bits(rt.flags & unwanted);
  const int unwantedDist = lUnwantedCount - rUnwantedCount;
  if (unwantedDist)
    return unwantedDist;
  const bool lHasOpt = optional == (types[l].flags & optional);
  const bool rHasOpt = optional == (types[r].flags & optional);
  if (lHasOpt)
    if (rHasOpt)
      return 0;
    else
      return -1;
  else if (rHasOpt)
    return 1;
  else
    return 0;
}

uint32_t DeviceMemoryPool::getCurrentAvailableDeviceKb()
{
  // TODO: use cached values if reading takes too much time
  return Globals::VK::phy.getCurrentAvailableMemoryKb(Globals::VK::dev.getInstance());
}
