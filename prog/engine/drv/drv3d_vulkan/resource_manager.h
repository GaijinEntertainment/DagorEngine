// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver.h"
#include "device_memory.h"
#include "device_resource.h"
#include "device_memory_pages.h"
#if VULKAN_HAS_RAYTRACING
#include "raytrace_as_resource.h"
#endif
#include "sampler_resource.h"
#include "render_pass_resource.h"
#include "memory_heap_resource.h"
#include <atomic>
#include <generic/dag_objectPool.h>
#include "physical_device_set.h"
#include "buffer_resource.h"

namespace drv3d_vulkan
{

class ExecutionContext;

enum class AllocationMethodName
{
  DEDICATED,
  MIXED_SUBALLOC_SMALL_POW2_ALIGNED_PAGES,
  BUFFER_SUBALLOC_SMALL_POW2_ALIGNED_PAGES,
  IMAGE_SUBALLOC_SMALL_POW2_ALIGNED_PAGES,
  RT_AS_SUBALLOC_SMALL_POW2_ALIGNED_PAGES,

  MIXED_SUBALLOC_HEAP_FREE_LIST,
  BUFFER_SUBALLOC_HEAP_FREE_LIST,
  IMAGE_SUBALLOC_HEAP_FREE_LIST,
  RT_AS_SUBALLOC_HEAP_FREE_LIST,

  MIXED_SUBALLOC_RINGED,
  BUFFER_SUBALLOC_RINGED,
  IMAGE_SUBALLOC_RINGED,

  // for resources that does not allocate heap memory
  OBJECT_BACKED_MEMORY,

  // allocators with shared handles
  BUFFER_SHARED_SMALL_POW2_ALIGNED_PAGES,
  BUFFER_SHARED_HEAP_FREE_LIST,
  BUFFER_SHARED_RINGED,

  COUNT,
  USER_MEMORY_HEAP,
  INVALID
};

struct ResourceMemory
{
  // TODO: this struct can be interleaved for speed

  // external usable
  VulkanHandle handle;
  VkDeviceSize offset;
  VkDeviceSize size;

  // rare usable
  uint8_t *mappedPointer;

  // private data used inside resource manager
  ResourceMemoryId index;
  uint32_t allocatorIndex;
  AllocationMethodName allocator;
  uint32_t memType;
  VkDeviceSize originalSize;

  bool isValid() const { return handle != 0; }
  void invalidate() { handle = 0; }

  VulkanBufferHandle bufferSharedHandle() const
  {
    G_ASSERT(!isDeviceMemory());
    return VulkanBufferHandle(handle);
  }

  VulkanDeviceMemoryHandle deviceMemory() const
  {
    G_ASSERT(isDeviceMemory());
    return VulkanDeviceMemoryHandle(handle);
  }

  VulkanDeviceMemoryHandle deviceMemorySlow() const;

  bool isDeviceMemory() const
  {
    return (allocator < AllocationMethodName::BUFFER_SHARED_SMALL_POW2_ALIGNED_PAGES) ||
           (allocator == AllocationMethodName::USER_MEMORY_HEAP);
  }

  uint8_t *mappedPtrOffset(uint32_t ofs) const { return mappedPointer + offset + ofs; }

  void dumpDebug() const
  {
    debug("vulkan: === resource memory %0u", index);
    debug("vulkan: handle %016llX type %2u shared: %s", handle, memType, isDeviceMemory() ? "n" : "y");
    debug("vulkan: alc %2i pg %08lX", (int)allocator, allocatorIndex);
    debug("vulkan: size %016llX offset %016llX original size %016llX", size, offset, originalSize);
    debug("vulkan: mapped address %p", mappedPointer);
  }

  bool intersects(const ResourceMemory &obj) const
  {
    if (isDeviceMemory() ^ obj.isDeviceMemory())
      return false;
    if (handle != obj.handle)
      return false;

    VkDeviceSize le = offset + size;
    VkDeviceSize re = obj.offset + obj.size;
    return (offset < re) && (le > obj.offset);
  }
};

// reduced struct for fast access in backend at alias sync tracking
struct AliasedResourceMemory
{
  bool deviceMemory;
  VulkanHandle handle;
  VkDeviceSize offset;
  VkDeviceSize size;

  AliasedResourceMemory() //-V730
  {}

  AliasedResourceMemory(const ResourceMemory &src_mem)
  {
    handle = src_mem.handle;
    offset = src_mem.offset;
    size = src_mem.size;
    deviceMemory = src_mem.isDeviceMemory();
  }

  bool intersects(const AliasedResourceMemory &obj) const
  {
    if (deviceMemory ^ obj.deviceMemory)
      return false;
    if (handle != obj.handle)
      return false;

    VkDeviceSize le = offset + size;
    VkDeviceSize re = obj.offset + obj.size;
    return (offset < re) && (le > obj.offset);
  }
};

class AliasedMemoryStorage
{
  Tab<AliasedResourceMemory> data;

public:
  AliasedMemoryStorage() = default;
  ~AliasedMemoryStorage() {}

  void update(ResourceMemoryId id, const AliasedResourceMemory &content)
  {
    if (data.size() <= id)
      data.resize(id + 1);
    data[id] = content;
  }
  const AliasedResourceMemory &get(ResourceMemoryId id) const { return data[id]; }
  void shutdown() { clear_and_shrink(data); }
};

class AbstractAllocator
{
protected:
  uint32_t memType;
  VkDeviceSize mixingGranularity;

public:
  struct Stats
  {
    uint32_t vkAllocs;
    uint32_t allocs;
    VkDeviceSize overhead;
    VkDeviceSize usage;
    const char *name;
    String pageMap;

    void add(const Stats &r)
    {
      vkAllocs += r.vkAllocs;
      allocs += r.allocs;
      overhead += r.overhead;
      usage += r.usage;
      pageMap += r.pageMap;
    }

    void print(const char *prefix, bool withPageMap = false)
    {
      debug("vulkan: RMS|     %s", prefix);
      debug("vulkan: RMS|      %05u vk allocs", vkAllocs);
      debug("vulkan: RMS|      %05u allocs", allocs);
      debug("vulkan: RMS|      %05u Mb total", usage >> 20);
      debug("vulkan: RMS|      %05u Mb overhead", overhead >> 20);
      if (withPageMap && pageMap.length())
        debug("vulkan: RMS|      Page map\n\n%s", pageMap);
    }
  };

  virtual ~AbstractAllocator() = default;
  virtual void init() = 0;
  virtual void shutdown() = 0;

  virtual bool alloc(ResourceMemory &target, const AllocationDesc &desc) = 0;
  virtual void free(ResourceMemory &target) = 0;
  virtual VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target) = 0;

  virtual const Stats getStats() = 0;

  void initBase(uint32_t mem_type, VkDeviceSize mixing_granularity);
  void onWrongFree(ResourceMemory &target);
  uint32_t getMemType() { return memType; }
};

template <typename PageType, SubAllocationMethod subAllocMethod>
struct PageList
{
  VkDeviceSize size;
  uint32_t maxPages;
  Tab<PageType> list;

  void accumulateStats(AbstractAllocator::Stats &ret, uint32_t category)
  {
    for (intptr_t j = 0; j < list.size(); ++j)
    {
      PageType &pg = list[j];
      if (pg.isValid())
      {
        ++ret.vkAllocs;
        ret.allocs += pg.getOccupiedCount();
        ret.overhead += pg.getUnusedMemorySize();
        ret.usage += size;
#if VULKAN_LOG_DEVICE_MEMORY_PAGES_MAP
        ret.pageMap += pg.printMemoryMap(pg.toAllocatorIndex(category, j));
        ret.pageMap += String("\n");
#else
        G_UNUSED(category);
#endif
      }
    }
  }

  void shutdown()
  {
    for (PageType &j : list)
    {
      if (j.isValid())
        j.shutdown();
    }
    clear_and_shrink(list);
  }

  template <typename AllocatorType, typename... InitArgs>
  bool alloc(AllocatorType allocator, ResourceMemory &target, const AllocationDesc &desc, uint32_t category,
    VkDeviceSize mixing_granularity, InitArgs... init_args)
  {
    uint32_t invalidPageIdx = -1;
    for (intptr_t i = 0; i < list.size(); ++i)
    {
      PageType &page = list[i];
      if (!page.isValid())
      {
        invalidPageIdx = i;
        continue;
      }
      if (page.occupy(target, desc, mixing_granularity))
      {
        target.allocatorIndex = page.toAllocatorIndex(category, i);
        return true;
      }
    }
    if (invalidPageIdx == -1)
    {
      if (list.size() >= maxPages)
        return false;

      invalidPageIdx = list.size();
      PageType newPage;
      list.push_back(newPage);
    }

    PageType &page = list[invalidPageIdx];
    if (!page.init(allocator, subAllocMethod, size, init_args...))
      return false;
    target.allocatorIndex = page.toAllocatorIndex(category, invalidPageIdx);
    bool occupied = page.occupy(target, desc, mixing_granularity);
    // should not happen as we allocating in empty page
    G_ASSERT(occupied);
    G_UNUSED(occupied);
    return true;
  }
};

class DedicatedAllocator : public AbstractAllocator
{
  Tab<DeviceMemory> list;

public:
  ~DedicatedAllocator() {}
  void init() override {}
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;
  VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target) override;

  const Stats getStats() override;
};

class ObjectBackedAllocator : public AbstractAllocator
{
  uint32_t count;

public:
  ~ObjectBackedAllocator() {}
  void init() override {}
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;
  VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target) override;

  const Stats getStats() override;
};

template <SubAllocationMethod subAllocMethod = SubAllocationMethod::MEM_OFFSET>
class SuballocPow2Allocator final : public AbstractAllocator
{
  typedef PageList<FixedOccupancyPage, subAllocMethod> PageListType;
  Tab<PageListType> categories;
  VkDeviceSize minSize;
  VkDeviceSize maxSize;
  VkDeviceSize pageSize;

public:
  ~SuballocPow2Allocator() {}
  void init() override;
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;
  VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target) override;

  const Stats getStats() override;
};

extern template class SuballocPow2Allocator<SubAllocationMethod::MEM_OFFSET>;
extern template class SuballocPow2Allocator<SubAllocationMethod::BUF_OFFSET>;

template <SubAllocationMethod subAllocMethod = SubAllocationMethod::MEM_OFFSET>
class SuballocFreeListAllocator final : public AbstractAllocator
{
  typedef PageList<FreeListPage, subAllocMethod> PageListType;

  enum
  {
    BUCKET_SMALL = 0,
    BUCKET_BIG = 1,
    BUCKET_COUNT = 2
  };

  PageListType pages[BUCKET_COUNT];
  VkDeviceSize maxElementSize;
  VkDeviceSize bigBucketMinSize;

public:
  ~SuballocFreeListAllocator() {}
  void init() override;
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;
  VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target) override;

  const Stats getStats() override;
};

extern template class SuballocFreeListAllocator<SubAllocationMethod::MEM_OFFSET>;
extern template class SuballocFreeListAllocator<SubAllocationMethod::BUF_OFFSET>;

template <SubAllocationMethod subAllocMethod = SubAllocationMethod::MEM_OFFSET>
class SuballocRingedAllocator final : public AbstractAllocator
{
  typedef PageList<LinearOccupancyPage, subAllocMethod> PageListType;
  PageListType ring;

#if DAGOR_DBGLEVEL > 0
  int64_t lastGoodAllocTime;
  uint32_t allocFailures;

  // may stuck for quite a long time when app is under heavy load
  static const int64_t failureTimeoutUsec = 60 * 1000 * 1000;
#endif

public:
  ~SuballocRingedAllocator() {}
  void init() override;
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;
  VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target) override;

  const Stats getStats() override;

  // allocator compromised if it is used for non temporal resource
  void checkIntegrity();
};

extern template class SuballocRingedAllocator<SubAllocationMethod::MEM_OFFSET>;
extern template class SuballocRingedAllocator<SubAllocationMethod::BUF_OFFSET>;

struct AllocationMethodPriorityList
{
  int available = 0;
  AllocationMethodName methods[(uint32_t)AllocationMethodName::COUNT] = {};

  bool isAllocatorAllowed(AllocationMethodName r) const
  {
    G_ASSERT(available);
    for (int i = 0; i < available; ++i)
    {
      if (methods[i] == r)
        return true;
    }

    return false;
  }
};

class ResourceManager
{
  //// memory managing
  // FIXME: avoid virtuals
  class MethodsArray
  {
    AbstractAllocator *methods[(uint32_t)AllocationMethodName::COUNT];

  public:
    void init(uint32_t mem_type, VkDeviceSize mixing_granularity);
    void shutdown();

    bool alloc(ResourceMemory &target, AllocationMethodPriorityList prio, const AllocationDesc &desc);
    void free(ResourceMemory &target);
    VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemory &target);

    const AbstractAllocator::Stats printStats();

    void onDeviceReset();
    void afterDeviceReset();
  };

  bool allowMixedPages;
  bool allowBufferSuballoc;
  bool allowAligmentTailOverlap;
  AllocationMethodPriorityList getAllocationMethods(const AllocationDesc &desc);

  Tab<MethodsArray> perMemoryTypeMethods;
  Tab<ResourceMemory> resAllocationsPool;
  Tab<ResourceMemoryId> freeMemPool;

  static const uint32_t hotMemPoolCount = GPU_TIMELINE_HISTORY_SIZE;
  Tab<ResourceMemoryId> hotMemPools[hotMemPoolCount];
  uint32_t hotMemPushIdx;
  uint32_t hotMemClearIdx();
  void processHotMem();
  void shutdownMemory(ResourceMemoryId memory_id);
  ResourceMemoryId allocFromHotMem(const AllocationDesc &desc, const AllocationMethodPriorityList &prio);
  bool allocMemoryIter(const AllocationDesc &desc, const AllocationMethodPriorityList &list, ResourceMemory &mem,
    const DeviceMemoryTypeRange &mem_types);
  ResourceMemoryId getUnusedMemoryId();

  std::atomic<uint32_t> outOfMemorySignal;

  //// res storage

  template <typename ResType>
  ObjectPool<ResType> &resPool();

  struct
  {
    ObjectPool<Image> image;
    ObjectPool<Buffer> buffer;
    ObjectPool<RenderPassResource> renderPass;
    ObjectPool<SamplerResource> sampler;
    ObjectPool<MemoryHeapResource> heap;
#if VULKAN_HAS_RAYTRACING
    ObjectPool<RaytraceAccelerationStructure> as;
#endif
  } resPools;

public:
  bool evictResourcesFor(ExecutionContext &ctx, VkDeviceSize desired_size, bool evict_used);
  void processPendingEvictions();

  //////////

  void init(const PhysicalDeviceSet &dev_set);
  void shutdown();
  void onDeviceReset();
  void afterDeviceReset();

  ResourceMemoryId allocAliasedMemory(ResourceMemoryId src_memory_id, VkDeviceSize size, VkDeviceSize offset);
  void freeAliasedMemory(ResourceMemoryId memory_id);

  ResourceMemoryId allocMemory(AllocationDesc desc);
  void freeMemory(ResourceMemoryId memory_id);
  const ResourceMemory &getMemory(ResourceMemoryId memory_id);
  VulkanDeviceMemoryHandle getDeviceMemoryHandle(ResourceMemoryId memory_id);

  void consumeOutOfMemorySignal()
  {
    if (outOfMemorySignal > 0)
      --outOfMemorySignal;
  }

  bool isOutOfMemorySignalReceived() { return outOfMemorySignal > 0; }

  void triggerOutOfMemorySignal() { ++outOfMemorySignal; }

  template <typename ResType>
  ResType *alloc(const typename ResType::Description desc, bool manage = true)
  {
    ResType *ret = resPool<ResType>().allocate(desc, manage);
    ResourceAlgorithm<ResType> alg(*ret);
    if (alg.create() != ResourceAlgorithm<ResType>::CreateResult::OK)
      triggerOutOfMemorySignal();
    return ret;
  }

  template <typename ResType>
  void free(ResType *res)
  {
    ResourceAlgorithm<ResType> alg(*res);
    alg.shutdown();
    resPool<ResType>().free(res);
  }

  template <typename ResType, typename CbType>
  void iterateAllocated(CbType cb)
  {
    resPool<ResType>().iterateAllocated(cb);
  }

  template <typename ResType, typename CbType>
  void iterateAllocatedBreakable(CbType cb)
  {
    resPool<ResType>().iterateAllocatedBreakable(cb);
  }

  template <typename ResType>
  size_t sizeAllocated()
  {
    return resPool<ResType>().size();
  }

  void onBackFrameEnd();

  void printStats(bool list_resources, bool allocator_info);
};

} // namespace drv3d_vulkan
