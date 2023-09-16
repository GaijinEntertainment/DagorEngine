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
      fatal("vulkan: unsupported force dedicated resource type");
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

void SuballocPow2Allocator::init()
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
    fatal("vulkan: pow2 allocator misconfigured (minb %u maxb %u mixGran %u)", minSizeBit, maxSizeBit, mixingGranularity);
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

void SuballocPow2Allocator::shutdown()
{
  for (PageListType &i : categories)
    i.shutdown();
  clear_and_shrink(categories);
}

bool SuballocPow2Allocator::alloc(ResourceMemory &target, const AllocationDesc &desc)
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

void SuballocPow2Allocator::free(ResourceMemory &target)
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

const AbstractAllocator::Stats SuballocPow2Allocator::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.allocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.name = "pow2 mem offset allocator";
  ret.pageMap = "";

  for (intptr_t i = 0; i < categories.size(); ++i)
    categories[i].accumulateStats(ret, i);

  return ret;
}

//////////////////////////////

void SuballocFreeListAllocator::init()
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

void SuballocFreeListAllocator::shutdown()
{
  for (PageListType &i : pages)
    i.shutdown();
}

bool SuballocFreeListAllocator::alloc(ResourceMemory &target, const AllocationDesc &desc)
{
  VkDeviceSize size = desc.reqs.requirements.size;
  if (size >= maxElementSize)
    return false;

  int bucket = size >= bigBucketMinSize ? BUCKET_BIG : BUCKET_SMALL;
  return pages[bucket].alloc(this, target, desc, bucket, mixingGranularity);
}

void SuballocFreeListAllocator::free(ResourceMemory &target)
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

const AbstractAllocator::Stats SuballocFreeListAllocator::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.allocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.name = "free list mem offset allocator";
  ret.pageMap = "";

  for (int i = BUCKET_SMALL; i < BUCKET_COUNT; ++i)
    pages[i].accumulateStats(ret, i);
  return ret;
}

//////////////////////////////

void SuballocRingedAllocator::init()
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

void SuballocRingedAllocator::shutdown() { ring.shutdown(); }

bool SuballocRingedAllocator::alloc(ResourceMemory &target, const AllocationDesc &desc)
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

void SuballocRingedAllocator::checkIntegrity()
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

void SuballocRingedAllocator::free(ResourceMemory &target)
{
  if (target.allocatorIndex != -1)
  {
    ring.list[target.allocatorIndex].free(target);
    target.allocatorIndex = -1;
    return;
  }

  onWrongFree(target);
}

const AbstractAllocator::Stats SuballocRingedAllocator::getStats()
{
  Stats ret;
  ret.vkAllocs = 0;
  ret.allocs = 0;
  ret.overhead = 0;
  ret.usage = 0;
  ret.name = "ring mem offset allocator";
  ret.pageMap = "";

  checkIntegrity();
  ring.accumulateStats(ret, 0);
  return ret;
}

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
