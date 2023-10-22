#pragma once

#include "driver.h"
#include "device_memory.h"
#include "device_resource.h"
#include "device_memory_pages.h"

namespace drv3d_vulkan
{

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

  COUNT,
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
    // TODO:
    // some parts of code need base memory handle for shared handle allocation
    // and this will fail here, need to properly handle without much looses
    G_ASSERT(isDeviceMemory());
    return VulkanDeviceMemoryHandle(handle);
  }

  bool isDeviceMemory() const
  {
    // TODO: enable this when buffer suballocs will use shared handle allocators
    return true;
    // (allocator != AllocationMethodName::BUFFER_SUBALLOC_HEAP_FREE_LIST) &&
    // (allocator != AllocationMethodName::BUFFER_SUBALLOC_RINGED) &&
    // (allocator != AllocationMethodName::BUFFER_SUBALLOC_SMALL_POW2_ALIGNED_PAGES);
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
  ~DedicatedAllocator(){};
  void init() override{};
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;

  const Stats getStats() override;
};

class ObjectBackedAllocator : public AbstractAllocator
{
  uint32_t count;

public:
  ~ObjectBackedAllocator(){};
  void init() override{};
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;

  const Stats getStats() override;
};

class SuballocPow2Allocator final : public AbstractAllocator
{
  typedef PageList<FixedOccupancyPage, SubAllocationMethod::MEM_OFFSET> PageListType;
  Tab<PageListType> categories;
  VkDeviceSize minSize;
  VkDeviceSize maxSize;
  VkDeviceSize pageSize;

public:
  ~SuballocPow2Allocator() = default;
  void init() override;
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;

  const Stats getStats() override;
};

class SuballocFreeListAllocator final : public AbstractAllocator
{
  typedef PageList<FreeListPage, SubAllocationMethod::MEM_OFFSET> PageListType;

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
  ~SuballocFreeListAllocator() = default;
  void init() override;
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;

  const Stats getStats() override;
};

class SuballocRingedAllocator final : public AbstractAllocator
{
  typedef PageList<LinearOccupancyPage, SubAllocationMethod::MEM_OFFSET> PageListType;
  PageListType ring;

#if DAGOR_DBGLEVEL > 0
  int64_t lastGoodAllocTime;
  uint32_t allocFailures;

  static const int64_t failureTimeoutUsec = 60 * 1000;
#endif

public:
  ~SuballocRingedAllocator() = default;
  void init() override;
  void shutdown() override;

  bool alloc(ResourceMemory &target, const AllocationDesc &desc) override;
  void free(ResourceMemory &target) override;

  const Stats getStats() override;

  // allocator compromised if it is used for non temporal resource
  void checkIntegrity();
};

class BufSuballocPow2Allocator : public DedicatedAllocator
{};

class BufSuballocFreeListAllocator : public DedicatedAllocator
{};

class BufSuballocRingedAllocator : public DedicatedAllocator
{};

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

    const AbstractAllocator::Stats printStats();
  };

  bool allowMixedPages;
  AllocationMethodPriorityList getAllocationMethods(const AllocationDesc &desc);

  Tab<MethodsArray> perMemoryTypeMethods;
  Tab<ResourceMemory> resAllocationsPool;
  Tab<ResourceMemoryId> freeMemPool;

  static const uint32_t hotMemPoolCount = FRAME_FRAME_BACKLOG_LENGTH;
  Tab<ResourceMemoryId> hotMemPools[hotMemPoolCount];
  uint32_t hotMemPushIdx;
  uint32_t hotMemClearIdx();
  void processHotMem();
  void shutdownMemory(ResourceMemoryId memory_id);
  ResourceMemoryId allocFromHotMem(const AllocationDesc &desc, const AllocationMethodPriorityList &prio);
  bool allocMemoryIter(const AllocationDesc &desc, const AllocationMethodPriorityList &list, ResourceMemory &mem,
    const DeviceMemoryTypeRange &mem_types);

  //// res storage

  template <typename ResType>
  ObjectPool<ResType> &resPool();

  struct
  {
    ObjectPool<Image> image;
    ObjectPool<Buffer> buffer;
    ObjectPool<RenderPassResource> renderPass;
#if D3D_HAS_RAY_TRACING
    ObjectPool<RaytraceAccelerationStructure> as;
#endif
  } resPools;

  // extern guard
  friend class SharedGuardedObjectAutoLock;
  WinCritSec *sharedGuard;

public:
  // TODO: finish this on fly/in frame residency concept
  //  template<typename TCtx>
  //  class ResidencyContext
  //  {
  //    TCtx& context;
  //    ResourceManager& resources;

  // public:
  //   ResidencyContext(TCtx& in_context, ResourceManager& in_resources)
  //    : context(in_context)
  //    , resources(in_resources)
  //   {
  //     debug("vulkan: residency+");
  //     context.finishPendingFrames();
  //   }

  //   ~ResidencyContext()
  //   {
  //     debug("vulkan: residency-");
  //   }

  //   ResidencyContext(const ResidencyContext&) = delete;

  //   template<typename ResType>
  //   void makeResident(ResType* res)
  //   {
  //     SharedGuardedObjectAutoLock lock(resources);
  //     while (resources.evictResourcesFor(res->getMemory().originalSize))
  //     {
  //       ResourceAlgorithm<ResType> alg(*res);
  //       if (alg.makeResident())
  //         return;
  //     }

  //     //we can't make resource resident when we need it
  //     //this is an OOM
  //     res->reportOutOfMemory();
  //   }

  // };

  bool aliasAcquire(ResourceMemoryId memory_id);
  void aliasRelease(ResourceMemoryId memory_id);
  bool evictResourcesFor(VkDeviceSize desired_size);

  //////////

  void init(WinCritSec *memory_lock, const PhysicalDeviceSet &dev_set);
  void shutdown();

  ResourceMemoryId allocMemory(const AllocationDesc &desc);
  void freeMemory(ResourceMemoryId memory_id);
  const ResourceMemory &getMemory(ResourceMemoryId memory_id);

  template <typename ResType>
  ResType *alloc(const typename ResType::Description desc, bool manage = true)
  {
    ResType *ret = resPool<ResType>().allocate(desc, manage);
    ResourceAlgorithm<ResType> alg(*ret);
    alg.create();
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

  void onBackFrameEnd();

  void printStats(bool list_resources, bool allocator_info);
};

} // namespace drv3d_vulkan
