#include "device.h"
#include <math/dag_adjpow2.h>

using namespace drv3d_vulkan;

void AbstractAllocator::initBase(uint32_t mem_type, VkDeviceSize mixing_granularity)
{
  memType = mem_type;
  mixingGranularity = mixing_granularity;
}

void AbstractAllocator::onWrongFree(ResourceMemory &target)
{
  logerr("vulkan: double free / wrong allocator for res mem %u [%08lX]", target.index, target.allocatorIndex);
}

void DedicatedAllocator::shutdown()
{
  for (DeviceMemory &i : list)
  {
    if (!is_null(i))
    {
      logerr("vulkan: leaked %016llX : %u Kb memory in dedicated allocator", generalize(i.memory), i.size);
      get_device().memory->free(i);
    }
  }
  clear_and_shrink(list);
}

bool DedicatedAllocator::alloc(ResourceMemory &target, const AllocationDesc &desc)
{
  DeviceMemoryTypeAllocationInfo devMemDsc = {};
  devMemDsc.typeIndex = memType;
  devMemDsc.size = desc.reqs.requirements.size;
  if (desc.isDedicated())
  {
    if (desc.obj.isBuffer())
    {
      devMemDsc.targetBuffer = VulkanBufferHandle(desc.obj.getBaseHandle());
    }
    else if (desc.obj.isImage())
    {
      devMemDsc.targetImage = VulkanImageHandle(desc.obj.getBaseHandle());
    }
    else
      DAG_FATAL("vulkan: unsupported force dedicated resource type");
  }

  DeviceMemory ret = get_device().memory->allocate(devMemDsc);
  if (is_null(ret))
    return false;

  target.handle = generalize(ret.memory);
  target.mappedPointer = ret.pointer;
  target.size = ret.size;
  target.offset = 0;

  for (intptr_t i = 0; i < list.size(); ++i)
  {
    if (is_null(list[i]))
    {
      list[i] = ret;
      target.allocatorIndex = i;
      return true;
    }
  }
  target.allocatorIndex = list.size();
  list.push_back(ret);

  return true;
}

void DedicatedAllocator::free(ResourceMemory &target)
{
  if (target.allocatorIndex != -1)
  {
    DeviceMemory &element = list[target.allocatorIndex];
    if (!is_null(element.memory) && (generalize(element.memory) == target.handle))
    {
      get_device().memory->free(element);
      element.memory = VulkanNullHandle();
      target.allocatorIndex = -1;
      return;
    }
  }

  onWrongFree(target);
}

VulkanDeviceMemoryHandle DedicatedAllocator::getDeviceMemoryHandle(ResourceMemory &target)
{
  // we should not ask dedicated allocator about memory handle as we already know it via ResourceMemory object!
  G_ASSERTF(0, "vulkan: trying to get device memory handle from dedicated allocator!");
  if (target.allocatorIndex != -1)
    return list[target.allocatorIndex].memory;
  return VulkanDeviceMemoryHandle{};
}

const AbstractAllocator::Stats DedicatedAllocator::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  for (DeviceMemory &i : list)
  {
    if (is_null(i.memory))
      continue;
    ret.usage += i.size;
    ++ret.vkAllocs;
  }
  ret.allocs = ret.vkAllocs;
  ret.name = "generic dedicated allocator";
  ret.pageMap = "";
  return ret;
}

//////////////////////////////

template <SubAllocationMethod subAllocMethod>
void SuballocPow2Allocator<subAllocMethod>::init()
{
  const DataBlock *settings =
    ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBlockByNameEx("allocators")->getBlockByNameEx("pow2");
  VkDeviceSize minSizeBit = settings->getInt("minSizeBit", 10);
  VkDeviceSize maxSizeBit = settings->getInt("maxSizeBit", 19);
  VkDeviceSize pageSizeBit = settings->getInt("pageSizeBit", 21);
  G_ASSERT(minSizeBit < maxSizeBit);
  G_ASSERT(pageSizeBit > maxSizeBit);

  minSize = 1ULL << minSizeBit;

  // support mixing granularity of device
  while (mixingGranularity > minSize)
    minSize = minSize << 1;

  maxSize = 1ULL << maxSizeBit;

  if (maxSize <= minSize)
  {
    // we support mixing granularity only if it sub or equal to 1Kb
    // having this error is a bad configuration
    DAG_FATAL("vulkan: pow2 allocator misconfigured (minb %u maxb %u mixGran %u)", minSizeBit, maxSizeBit, mixingGranularity);
  }

  pageSize = 1ULL << pageSizeBit;
  int categoryBits = maxSizeBit - minSizeBit;
  uint32_t maxPagesPerCat = settings->getInt("maxPages", 64);

  for (int i = 0; i < categoryBits; ++i)
  {
    PageListType cat;
    cat.size = pageSize;
    cat.maxPages = maxPagesPerCat;
    categories.push_back(cat);
  }
}

template <SubAllocationMethod subAllocMethod>
void SuballocPow2Allocator<subAllocMethod>::shutdown()
{
  for (PageListType &i : categories)
    i.shutdown();
  clear_and_shrink(categories);
}

template <SubAllocationMethod subAllocMethod>
bool SuballocPow2Allocator<subAllocMethod>::alloc(ResourceMemory &target, const AllocationDesc &desc)
{
  VkDeviceSize resSize = desc.reqs.requirements.size;
  if (resSize > maxSize)
    return false;

  // TODO: optimize
  int catIdx = 0;
  while (resSize > (minSize << catIdx))
  {
    ++catIdx;
    if (catIdx >= categories.size())
      return false;
  }

  PageListType &cat = categories[catIdx];
  VkDeviceSize catSize = minSize << catIdx;
  G_ASSERT(catSize >= resSize);

  if (desc.reqs.requirements.alignment > catSize)
    return false;

  target.size = catSize;
  return cat.alloc(this, target, desc, catIdx, mixingGranularity, catSize);
}

template <SubAllocationMethod subAllocMethod>
void SuballocPow2Allocator<subAllocMethod>::free(ResourceMemory &target)
{
  uint32_t &allocatorIndex = target.allocatorIndex;
  if (allocatorIndex != -1)
  {
    categories[DeviceMemoryPage::getCatIdx(allocatorIndex)].list[DeviceMemoryPage::getPageIdx(allocatorIndex)].free(target);
    allocatorIndex = -1;
    return;
  }

  onWrongFree(target);
}

template <SubAllocationMethod subAllocMethod>
VulkanDeviceMemoryHandle SuballocPow2Allocator<subAllocMethod>::getDeviceMemoryHandle(ResourceMemory &target)
{
  G_ASSERTF(subAllocMethod != SubAllocationMethod::MEM_OFFSET,
    "vulkan: trying to get device memory handle from raw memory allocator!");
  uint32_t allocatorIndex = target.allocatorIndex;
  if (allocatorIndex != -1)
    return categories[DeviceMemoryPage::getCatIdx(allocatorIndex)]
      .list[DeviceMemoryPage::getPageIdx(allocatorIndex)]
      .getDeviceMemoryHandle();
  return VulkanDeviceMemoryHandle{};
}

template <SubAllocationMethod subAllocMethod>
const AbstractAllocator::Stats SuballocPow2Allocator<subAllocMethod>::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.allocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.name = subAllocMethod == SubAllocationMethod::MEM_OFFSET ? "pow2 mem offset allocator" : "pow2 buf offset allocator";
  ret.pageMap = "";

  for (intptr_t i = 0; i < categories.size(); ++i)
    categories[i].accumulateStats(ret, i);

  return ret;
}

namespace drv3d_vulkan
{
template class SuballocPow2Allocator<SubAllocationMethod::MEM_OFFSET>;
template class SuballocPow2Allocator<SubAllocationMethod::BUF_OFFSET>;
} // namespace drv3d_vulkan

//////////////////////////////

template <SubAllocationMethod subAllocMethod>
void SuballocFreeListAllocator<subAllocMethod>::init()
{
  const DataBlock *settings =
    ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBlockByNameEx("allocators")->getBlockByNameEx("freelist");

  VkDeviceSize pageSize = settings->getInt64("pageSizeMb", 16) << 20;
  for (PageListType &i : pages)
    i.size = pageSize;

  // do not make too much free lists for small objects
  pages[BUCKET_SMALL].maxPages = settings->getInt("maxSmallPages", 64);
  // this is most big category where almost anything drops
  pages[BUCKET_BIG].maxPages = settings->getInt("maxBigPages", 2048);
  bigBucketMinSize = settings->getInt64("bigBucketMinSizeKb", 1024) << 10;
  maxElementSize = settings->getInt64("maxElementSizeMb", 2) << 20;

  G_ASSERT(pageSize > maxElementSize);
}

template <SubAllocationMethod subAllocMethod>
void SuballocFreeListAllocator<subAllocMethod>::shutdown()
{
  for (PageListType &i : pages)
    i.shutdown();
}

template <SubAllocationMethod subAllocMethod>
bool SuballocFreeListAllocator<subAllocMethod>::alloc(ResourceMemory &target, const AllocationDesc &desc)
{
  VkDeviceSize size = desc.reqs.requirements.size;
  if (size >= maxElementSize)
    return false;

  int bucket = size >= bigBucketMinSize ? BUCKET_BIG : BUCKET_SMALL;
  return pages[bucket].alloc(this, target, desc, bucket, mixingGranularity);
}

template <SubAllocationMethod subAllocMethod>
void SuballocFreeListAllocator<subAllocMethod>::free(ResourceMemory &target)
{
  uint32_t &allocatorIndex = target.allocatorIndex;
  if (allocatorIndex != -1)
  {
    pages[DeviceMemoryPage::getCatIdx(allocatorIndex)].list[DeviceMemoryPage::getPageIdx(allocatorIndex)].free(target);
    allocatorIndex = -1;
    return;
  }

  onWrongFree(target);
}

template <SubAllocationMethod subAllocMethod>
VulkanDeviceMemoryHandle SuballocFreeListAllocator<subAllocMethod>::getDeviceMemoryHandle(ResourceMemory &target)
{
  G_ASSERTF(subAllocMethod != SubAllocationMethod::MEM_OFFSET,
    "vulkan: trying to get device memory handle from raw memory allocator!");
  uint32_t allocatorIndex = target.allocatorIndex;
  if (allocatorIndex != -1)
    return pages[DeviceMemoryPage::getCatIdx(allocatorIndex)]
      .list[DeviceMemoryPage::getPageIdx(allocatorIndex)]
      .getDeviceMemoryHandle();
  return VulkanDeviceMemoryHandle{};
}

template <SubAllocationMethod subAllocMethod>
const AbstractAllocator::Stats SuballocFreeListAllocator<subAllocMethod>::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.allocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.name = subAllocMethod == SubAllocationMethod::MEM_OFFSET ? "free list mem offset allocator" : "free list buf offset allocator";
  ret.pageMap = "";

  for (int i = BUCKET_SMALL; i < BUCKET_COUNT; ++i)
    pages[i].accumulateStats(ret, i);
  return ret;
}

namespace drv3d_vulkan
{
template class SuballocFreeListAllocator<SubAllocationMethod::MEM_OFFSET>;
template class SuballocFreeListAllocator<SubAllocationMethod::BUF_OFFSET>;
} // namespace drv3d_vulkan

//////////////////////////////

template <SubAllocationMethod subAllocMethod>
void SuballocRingedAllocator<subAllocMethod>::init()
{
  const DataBlock *settings =
    ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBlockByNameEx("allocators")->getBlockByNameEx("ring");

  ring.size = settings->getInt64("pageSizeMb", 8) << 20;
  ring.maxPages = settings->getInt("maxPages", 4);

#if DAGOR_DBGLEVEL > 0
  lastGoodAllocTime = 0;
  allocFailures = 0;
#endif
}

template <SubAllocationMethod subAllocMethod>
void SuballocRingedAllocator<subAllocMethod>::shutdown()
{
  ring.shutdown();
}

template <SubAllocationMethod subAllocMethod>
bool SuballocRingedAllocator<subAllocMethod>::alloc(ResourceMemory &target, const AllocationDesc &desc)
{
  // no point to keep 1 element, so allow 2 minimum
  if ((ring.size >> 1) <= desc.reqs.requirements.size)
    return false;

  bool ret = ring.alloc(this, target, desc, 0, mixingGranularity);
#if DAGOR_DBGLEVEL > 0
  if (!ret)
    ++allocFailures;
  else
  {
    allocFailures = 0;
    lastGoodAllocTime = ref_time_ticks();
  }
#endif
  return ret;
}

template <SubAllocationMethod subAllocMethod>
void SuballocRingedAllocator<subAllocMethod>::checkIntegrity()
{
#if DAGOR_DBGLEVEL > 0
  if (!allocFailures)
    return;

  // if allocator keep failing in a whole timeout period - it is surely broken
  if (lastGoodAllocTime && (ref_time_delta_to_usec(ref_time_ticks() - lastGoodAllocTime) >= failureTimeoutUsec))
  {
    logerr("vulkan: ringed allocator are compromised");
    for (int i = 0; i < ring.list.size(); ++i)
    {
      if (ring.list[i].isValid())
        debug("vulkan: ring page %u have %u refs", i, ring.list[i].getOccupiedCount());
    }
  }
#endif
  // we should catch all non temporals in ring before release
}

template <SubAllocationMethod subAllocMethod>
void SuballocRingedAllocator<subAllocMethod>::free(ResourceMemory &target)
{
  if (target.allocatorIndex != -1)
  {
    ring.list[target.allocatorIndex].free(target);
    target.allocatorIndex = -1;
    return;
  }

  onWrongFree(target);
}

template <SubAllocationMethod subAllocMethod>
VulkanDeviceMemoryHandle SuballocRingedAllocator<subAllocMethod>::getDeviceMemoryHandle(ResourceMemory &target)
{
  G_ASSERTF(subAllocMethod != SubAllocationMethod::MEM_OFFSET,
    "vulkan: trying to get device memory handle from raw memory allocator!");
  if (target.allocatorIndex != -1)
    return ring.list[target.allocatorIndex].getDeviceMemoryHandle();
  return VulkanDeviceMemoryHandle{};
}

template <SubAllocationMethod subAllocMethod>
const AbstractAllocator::Stats SuballocRingedAllocator<subAllocMethod>::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.allocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.name = subAllocMethod == SubAllocationMethod::MEM_OFFSET ? "ring mem offset allocator" : "ring buf offset allocator";
  ret.pageMap = "";

  checkIntegrity();
  ring.accumulateStats(ret, 0);
  return ret;
}

namespace drv3d_vulkan
{
template class SuballocRingedAllocator<SubAllocationMethod::MEM_OFFSET>;
template class SuballocRingedAllocator<SubAllocationMethod::BUF_OFFSET>;
} // namespace drv3d_vulkan

//////////////////////////////

void ObjectBackedAllocator::shutdown()
{
  if (count)
    logerr("vulkan: leaked %u objects in object backed allocator", count);
  count = 0;
}

bool ObjectBackedAllocator::alloc(ResourceMemory &target, const AllocationDesc &desc)
{
  G_UNUSED(desc);
  G_ASSERTF(desc.objectBaked, "vulkan: can't use this allocator if object is not providing allocation");

  target.handle = 0;
  target.mappedPointer = nullptr;
  target.size = 0;
  target.offset = 0;
  target.allocatorIndex = 0;
  ++count;

  return true;
}

void ObjectBackedAllocator::free(ResourceMemory &) { --count; }

VulkanDeviceMemoryHandle ObjectBackedAllocator::getDeviceMemoryHandle(ResourceMemory &) { return VulkanDeviceMemoryHandle{}; }

const AbstractAllocator::Stats ObjectBackedAllocator::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.allocs = count;
  ret.name = "object backed allocator";
  ret.pageMap = "";
  return ret;
}
