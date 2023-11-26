#include "device.h"
#include <generic/dag_sort.h>

namespace drv3d_vulkan
{
template <>
ObjectPool<Image> &ResourceManager::resPool()
{
  return resPools.image;
}

template <>
ObjectPool<Buffer> &ResourceManager::resPool()
{
  return resPools.buffer;
}

template <>
ObjectPool<RenderPassResource> &ResourceManager::resPool()
{
  return resPools.renderPass;
}

#if D3D_HAS_RAY_TRACING
template <>
ObjectPool<RaytraceAccelerationStructure> &ResourceManager::resPool()
{
  return resPools.as;
}
#endif
} // namespace drv3d_vulkan

using namespace drv3d_vulkan;

namespace
{

template <typename ResTypeName>
void check_leaks(ObjectPool<ResTypeName> &pool)
{
  pool.iterateAllocated([](ResTypeName *i) {
    ResourceAlgorithm<ResTypeName> alg(*i);
    debug("vulkan: res: %s %p %s leaked", i->resTypeString(), i, i->getDebugName());
    alg.shutdown();
  });
  pool.freeAll();
}

template <typename ResTypeName>
bool evict_from_pool(ObjectPool<ResTypeName> &pool, VkDeviceSize &desired_size)
{
  uint32_t evictedCount = 0;
  pool.iterateAllocatedBreakable([&evictedCount, &desired_size](ResTypeName *i) {
    ResourceAlgorithm<ResTypeName> alg(*i);
    VkDeviceSize evictionSize = alg.evict();

    if (!evictionSize)
      return true;

    ++evictedCount;
    if (evictionSize >= desired_size)
    {
      desired_size = 0;
      return false;
    }
    else
      desired_size -= evictionSize;

    return true;
  });
  return evictedCount > 0;
}

} // namespace

VulkanDeviceMemoryHandle ResourceMemory::deviceMemorySlow() const
{
  Device &device = get_device();
  SharedGuardedObjectAutoLock resLock(device.resources);

  return device.resources.getDeviceMemoryHandle(index);
}

void ResourceManager::init(WinCritSec *memory_lock, const PhysicalDeviceSet &dev_set)
{
  uint32_t memTypesCount = dev_set.memoryProperties.memoryTypeCount;
  allowMixedPages = dev_set.properties.limits.bufferImageGranularity <= 1024;
  allowBufferSuballoc = get_device().getPerDriverPropertyBlock("bufferSuballocation")->getBool("enable", false);
  debug("vulkan: %s type mixed allocators", allowMixedPages ? "using" : "not using");
  hotMemPushIdx = 0;
  sharedGuard = memory_lock;
  perMemoryTypeMethods.resize(memTypesCount);
  for (uint32_t i = 0; i < memTypesCount; ++i)
    perMemoryTypeMethods[i].init(i, allowMixedPages ? dev_set.properties.limits.bufferImageGranularity : 0);
}

void ResourceManager::shutdown()
{
  check_leaks(resPool<Image>());
  check_leaks(resPool<Buffer>());
  check_leaks(resPool<RenderPassResource>());
#if D3D_HAS_RAY_TRACING
  check_leaks(resPool<RaytraceAccelerationStructure>());
#endif

  for (int i = 0; i < hotMemPoolCount; ++i)
    processHotMem();

  for (ResourceMemory &i : resAllocationsPool)
  {
    if (i.isValid())
    {
      debug("vulkan: res mem %u leaked", i.index);
      freeMemory(i.index);
    }
  }

  clear_and_shrink(freeMemPool);

  for (uint32_t i = 0; i < perMemoryTypeMethods.size(); ++i)
    perMemoryTypeMethods[i].shutdown();
  clear_and_shrink(perMemoryTypeMethods);
}

AllocationMethodPriorityList ResourceManager::getAllocationMethods(const AllocationDesc &desc)
{
  // select possible methods and their priority
  // based on allocation desc
  AllocationMethodPriorityList ret;

  // if underlying memory is purely object baked
  if (desc.objectBaked)
  {
    ret.methods[ret.available++] = AllocationMethodName::OBJECT_BACKED_MEMORY;
    return ret;
  }

  if (desc.isSharedHandleAllowed())
  {
    // we want to allocate from shared handle allocators
    if (desc.obj.isBuffer() && allowBufferSuballoc)
    {
      if (desc.temporary)
        ret.methods[ret.available++] = AllocationMethodName::BUFFER_SHARED_RINGED;
      else
      {
        ret.methods[ret.available++] = AllocationMethodName::BUFFER_SHARED_SMALL_POW2_ALIGNED_PAGES;
        ret.methods[ret.available++] = AllocationMethodName::BUFFER_SHARED_HEAP_FREE_LIST;
      }
    }
    // must exit early, shared handle uses separate codepath in ResourceAlgorithm
    return ret;
  }

  // we can allocate via suballoc of resource is not dedicated
  if (!desc.isDedicated())
  {
    // use mixed allocators to avoid memory overhead if possible
    if (allowMixedPages)
    {
      if (desc.temporary)
        ret.methods[ret.available++] = AllocationMethodName::MIXED_SUBALLOC_RINGED;
      else
      {
        ret.methods[ret.available++] = AllocationMethodName::MIXED_SUBALLOC_SMALL_POW2_ALIGNED_PAGES;
        ret.methods[ret.available++] = AllocationMethodName::MIXED_SUBALLOC_HEAP_FREE_LIST;
      }
    }
    else
    {
      // device need 1kb+ (typicall 65kb+ after 1Kb) making mixing useless
      if (desc.obj.isBuffer())
      {
        if (desc.temporary)
          ret.methods[ret.available++] = AllocationMethodName::BUFFER_SUBALLOC_RINGED;
        else
        {
          ret.methods[ret.available++] = AllocationMethodName::BUFFER_SUBALLOC_SMALL_POW2_ALIGNED_PAGES;
          ret.methods[ret.available++] = AllocationMethodName::BUFFER_SUBALLOC_HEAP_FREE_LIST;
        }
      }
      else if (desc.obj.isImage())
      {
        if (desc.temporary)
          ret.methods[ret.available++] = AllocationMethodName::IMAGE_SUBALLOC_RINGED;
        else
        {
          ret.methods[ret.available++] = AllocationMethodName::IMAGE_SUBALLOC_SMALL_POW2_ALIGNED_PAGES;
          ret.methods[ret.available++] = AllocationMethodName::IMAGE_SUBALLOC_HEAP_FREE_LIST;
        }
      }
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
      else if (desc.obj.isAccelerationStruct())
      {
        ret.methods[ret.available++] = AllocationMethodName::RT_AS_SUBALLOC_SMALL_POW2_ALIGNED_PAGES;
        ret.methods[ret.available++] = AllocationMethodName::RT_AS_SUBALLOC_HEAP_FREE_LIST;
      }
#endif
    }
  }

  // otherwise use dedicated allocation
  ret.methods[ret.available++] = AllocationMethodName::DEDICATED;
  return ret;
}

ResourceMemoryId ResourceManager::allocFromHotMem(const AllocationDesc &desc, const AllocationMethodPriorityList &prio)
{
  for (Tab<ResourceMemoryId> &i : hotMemPools)
    for (ResourceMemoryId &j : i)
    {
      const ResourceMemory &mem = getMemory(j);

      if (mem.offset % desc.reqs.requirements.alignment)
        continue;

      // can be tuned to allow reusing with some size delta
      if (mem.originalSize != desc.reqs.requirements.size)
        continue;

      if (mem.isDeviceMemory() != desc.isSharedHandleAllowed())
        continue;

      if (((1 << mem.memType) && desc.reqs.requirements.memoryTypeBits) == 0)
        continue;

      if (!prio.isAllocatorAllowed(mem.allocator))
        continue;

      ResourceMemoryId ret = j;
      j = i.back();
      i.pop_back();

      return ret;
    }

  return -1;
}

bool ResourceManager::allocMemoryIter(const AllocationDesc &desc, const AllocationMethodPriorityList &list, ResourceMemory &mem,
  const DeviceMemoryTypeRange &mem_types)
{
  for (uint32_t type : mem_types)
  {
    if (perMemoryTypeMethods[type].alloc(mem, list, desc))
    {
      mem.memType = type;
      return true;
    }
  }

  return false;
}

ResourceMemoryId ResourceManager::allocMemory(const AllocationDesc &desc)
{
  AllocationMethodPriorityList list = getAllocationMethods(desc);
  if (!list.available)
    return -1;

  // try to reuse hot memory to hide allocation CPU costs
  ResourceMemoryId ret = allocFromHotMem(desc, list);

  if (ret != -1)
    return ret;

  if (!freeMemPool.empty())
  {
    ret = freeMemPool.back();
    freeMemPool.pop_back();
  }
  else
  {
    ret = resAllocationsPool.size();
    resAllocationsPool.push_back();
  }

  ResourceMemory &mem = resAllocationsPool[ret];
  mem.invalidate();
  mem.index = ret;
  mem.originalSize = desc.reqs.requirements.size;
  mem.size = mem.originalSize;

  DeviceMemoryTypeRange memoryTypes = get_device().memory->selectMemoryType(desc.reqs.requirements.memoryTypeBits, desc.memClass);

  // try to allocate
  if (!allocMemoryIter(desc, list, mem, memoryTypes))
  {
    // if we cant - clear hot memory and try again
    for (int i = 0; i < hotMemPoolCount; ++i)
    {
      processHotMem();
      if (allocMemoryIter(desc, list, mem, memoryTypes))
        break;
    }
  }

  // return whatever we got, even empty memory
  return ret;
}

uint32_t ResourceManager::hotMemClearIdx() { return (hotMemPushIdx + (hotMemPoolCount - 1)) % hotMemPoolCount; }

void ResourceManager::processHotMem()
{
  Tab<ResourceMemoryId> &hotPool = hotMemPools[hotMemClearIdx()];
  for (ResourceMemoryId &i : hotPool)
    shutdownMemory(i);
  clear_and_shrink(hotPool);
  hotMemPushIdx = (hotMemPushIdx + 1) % hotMemPoolCount;
}

void ResourceManager::shutdownMemory(ResourceMemoryId memory_id)
{
  ResourceMemory &mem = resAllocationsPool[memory_id];
  G_ASSERT(mem.isValid());
  perMemoryTypeMethods[(int)mem.memType].free(mem);
  mem.invalidate();
  freeMemPool.push_back(memory_id);
}

void ResourceManager::freeMemory(ResourceMemoryId memory_id)
{
  ResourceMemory &mem = resAllocationsPool[memory_id];
  // do not hot pool empty memory
  if (!mem.isValid())
    freeMemPool.push_back(memory_id);
  // we can't reuse dedicated allocations (they are tied to object handle), simply shutdown them
  else if (mem.allocator == AllocationMethodName::DEDICATED)
    shutdownMemory(memory_id);
  else
    hotMemPools[hotMemPushIdx].push_back(memory_id);
}

const ResourceMemory &ResourceManager::getMemory(ResourceMemoryId memory_id) { return resAllocationsPool[memory_id]; }

VulkanDeviceMemoryHandle ResourceManager::getDeviceMemoryHandle(ResourceMemoryId memory_id)
{
  ResourceMemory &mem = resAllocationsPool[memory_id];
  return perMemoryTypeMethods[(int)mem.memType].getDeviceMemoryHandle(mem);
}

bool ResourceManager::evictResourcesFor(VkDeviceSize desired_size)
{
  debug("vulkan: trying to evict resources for %u Kb memory", desired_size >> 10);

  // TODO: this should be more precise
  if (evict_from_pool(resPool<Image>(), desired_size))
    return true;
  if (evict_from_pool(resPool<Buffer>(), desired_size))
    return true;
#if D3D_HAS_RAY_TRACING
  if (evict_from_pool(resPool<RaytraceAccelerationStructure>(), desired_size))
    return true;
#endif

  debug("vulkan: can't evict resources to fit %u Kb memory", desired_size >> 10);

  return false;
}

bool ResourceManager::aliasAcquire(ResourceMemoryId memory_id)
{
  ResourceMemory &mem = resAllocationsPool[memory_id];
  if (mem.isValid())
  {
    // maybe some internal aliasing managing
    return true;
  }

  // full memory id recreation is needed
  // resource algo should free this memory
  return false;
}

void ResourceManager::aliasRelease(ResourceMemoryId memory_id)
{
  // add id to "aliased/non-resident" pool that can be reused or deallocated
  // but for now just free it here as shortcut
  ResourceMemory &mem = resAllocationsPool[memory_id];
  G_ASSERT(mem.isValid());
  perMemoryTypeMethods[(int)mem.memType].free(mem);
  mem.invalidate();
}

void ResourceManager::onBackFrameEnd() { processHotMem(); }

struct ResourceStatData
{
  uint32_t totalSz;
  String info;
};

static int resourceStatSizeSort(const ResourceStatData *a, const ResourceStatData *b) { return a->totalSz > b->totalSz ? -1 : 1; }

void ResourceManager::printStats(bool list_resources, bool allocator_info)
{
  debug("vulkan: RMS| resource manager stats start ====");

#if 0
  {
    struct UsedMemBlock
    {
      VkDeviceSize offset;
      VkDeviceSize size;
      ResourceMemoryId id;
    };
    eastl::map<VulkanHandle, eastl::vector<UsedMemBlock>> usedFragments;
    for (const ResourceMemory& i : resAllocationsPool)
    {
      if (!i.isValid())
        continue;

      if (usedFragments.find(i.handle) == usedFragments.end())
      {
        eastl::vector<UsedMemBlock> newElement;
        newElement.push_back({i.offset, i.size, i.index});
        usedFragments[i.handle] = newElement;
      } else
        usedFragments[i.handle].push_back({i.offset, i.size, i.index});
    }

    for (auto i = usedFragments.begin(); i != usedFragments.end(); ++i)
    {
      for (int j = 0; j < (int)i->second.size()-1;++j)
      {
        UsedMemBlock l = i->second[j];
        auto istart = begin(i->second) + j + 1;
        eastl::find_if(istart, end(i->second),
          [&l, &pool = resAllocationsPool](const UsedMemBlock& r)
          {
            VkDeviceSize le = l.offset + l.size;
            VkDeviceSize re = r.offset + r.size;
            if ((l.offset < re) && (le > r.offset))
            {
              debug("vulkan: memory allocations %u and %u are overlapping!", r.id, l.id);
              pool[r.id].dumpDebug();
              pool[l.id].dumpDebug();
              return true;
            } else
              return false;
          }
        );
      }
    }
  }
#endif

  // allocations info : logged regardless of parameters
  {
    VkDeviceSize rawSize = 0;
    VkDeviceSize alignedSize = 0;
    VkDeviceSize hotSize = 0;
    VkDeviceSize hotAllocs = 0;

    for (const ResourceMemory &i : resAllocationsPool)
    {
      if (!i.isValid())
        continue;

      rawSize += i.originalSize;
      alignedSize += i.size;
    }

    for (Tab<ResourceMemoryId> &i : hotMemPools)
      for (ResourceMemoryId &j : i)
      {
        const ResourceMemory &mem = getMemory(j);
        hotSize += mem.size;
        ++hotAllocs;
      }

    VkDeviceSize alignOverhead = max<VkDeviceSize>(alignedSize - rawSize, 0);

    debug("vulkan: RMS| ResourceMemory info");
    debug("vulkan: RMS|  %05u    active allocs", resAllocationsPool.size() - hotAllocs - freeMemPool.size());
    debug("vulkan: RMS|  %05u    peak allocs", resAllocationsPool.size());
    debug("vulkan: RMS|  %05u Mb active usage", (rawSize - alignOverhead - hotSize) >> 20);
    debug("vulkan: RMS|  %05u Mb alignment overhead", alignOverhead >> 20);
    debug("vulkan: RMS|  %05u Mb hotswap", hotSize >> 20);
  }

  if (allocator_info)
  {
    debug("vulkan: RMS| Allocator info");
    AbstractAllocator::Stats sum = {};
    for (int i = 0; i < perMemoryTypeMethods.size(); ++i)
    {
      debug("vulkan: RMS|  Memory type %03u", i);
      AbstractAllocator::Stats memTypeStat = perMemoryTypeMethods[i].printStats();
      if (memTypeStat.allocs)
        sum.add(memTypeStat);
      else
        debug("vulkan: RMS|   < empty >");
    }

    sum.print("Summary");
  }

  if (list_resources)
  {
    debug("vulkan: RMS| Resources list");
    debug("vulkan: RMS|  resptr           | handle           | res type  |memid |type|meth| memsize | page       | w x h     | name");
    VkDeviceSize perResTypeSize[(int)ResourceType::COUNT] = {};
    Tab<ResourceStatData> statBuffer;

    auto statPrintCb = [&perResTypeSize, &statBuffer](Resource *i) {
      ResourceStatData ret{0, i->printStatLog()};
      if (i->isManaged())
      {
        ret.totalSz = i->getMemory().size;
        perResTypeSize[(int)i->getResType()] += i->getMemory().originalSize;
      }
      statBuffer.push_back(ret);
    };

    resPool<Image>().iterateAllocated(statPrintCb);
    resPool<Buffer>().iterateAllocated(statPrintCb);
    resPool<RenderPassResource>().iterateAllocated(statPrintCb);
#if D3D_HAS_RAY_TRACING
    resPool<RaytraceAccelerationStructure>().iterateAllocated(statPrintCb);
#endif

    sort(statBuffer, &resourceStatSizeSort);
    for (ResourceStatData &i : statBuffer)
      debug("vulkan: RMS|  %s", i.info);

    for (int i = 0; i < (int)ResourceType::COUNT; ++i)
    {
      debug("vulkan: RMS| total %u Mb for %s resources", perResTypeSize[i] >> 20, Resource::resTypeString((ResourceType)i));
    }
  }

  debug("vulkan: RMS| resource manager stats end   ====");
}

void ResourceManager::MethodsArray::init(uint32_t mem_type, VkDeviceSize mixing_granularity)
{
  methods[(int)AllocationMethodName::DEDICATED] = new DedicatedAllocator();
  methods[(int)AllocationMethodName::OBJECT_BACKED_MEMORY] = new ObjectBackedAllocator();

  methods[(int)AllocationMethodName::MIXED_SUBALLOC_SMALL_POW2_ALIGNED_PAGES] = new SuballocPow2Allocator();
  methods[(int)AllocationMethodName::BUFFER_SUBALLOC_SMALL_POW2_ALIGNED_PAGES] = new SuballocPow2Allocator();
  methods[(int)AllocationMethodName::IMAGE_SUBALLOC_SMALL_POW2_ALIGNED_PAGES] = new SuballocPow2Allocator();
  methods[(int)AllocationMethodName::RT_AS_SUBALLOC_SMALL_POW2_ALIGNED_PAGES] = new SuballocPow2Allocator();
  methods[(int)AllocationMethodName::BUFFER_SHARED_SMALL_POW2_ALIGNED_PAGES] =
    new SuballocPow2Allocator<SubAllocationMethod::BUF_OFFSET>();

  methods[(int)AllocationMethodName::MIXED_SUBALLOC_HEAP_FREE_LIST] = new SuballocFreeListAllocator();
  methods[(int)AllocationMethodName::BUFFER_SUBALLOC_HEAP_FREE_LIST] = new SuballocFreeListAllocator();
  methods[(int)AllocationMethodName::IMAGE_SUBALLOC_HEAP_FREE_LIST] = new SuballocFreeListAllocator();
  methods[(int)AllocationMethodName::RT_AS_SUBALLOC_HEAP_FREE_LIST] = new SuballocFreeListAllocator();
  methods[(int)AllocationMethodName::BUFFER_SHARED_HEAP_FREE_LIST] = new SuballocFreeListAllocator<SubAllocationMethod::BUF_OFFSET>();

  methods[(int)AllocationMethodName::MIXED_SUBALLOC_RINGED] = new SuballocRingedAllocator();
  methods[(int)AllocationMethodName::BUFFER_SUBALLOC_RINGED] = new SuballocRingedAllocator();
  methods[(int)AllocationMethodName::IMAGE_SUBALLOC_RINGED] = new SuballocRingedAllocator();
  methods[(int)AllocationMethodName::BUFFER_SHARED_RINGED] = new SuballocRingedAllocator<SubAllocationMethod::BUF_OFFSET>();

  for (AbstractAllocator *i : methods)
  {
    i->initBase(mem_type, mixing_granularity);
    i->init();
  }
}

void ResourceManager::MethodsArray::shutdown()
{
  for (AbstractAllocator *i : methods)
  {
    i->shutdown();
    delete i;
  }
}

bool ResourceManager::MethodsArray::alloc(ResourceMemory &target, AllocationMethodPriorityList prio, const AllocationDesc &desc)
{
  for (int i = 0; i < prio.available; ++i)
  {
    if (methods[(int)prio.methods[i]]->alloc(target, desc))
    {
      target.allocator = prio.methods[i];
      return true;
    }
  }

  return false;
}

void ResourceManager::MethodsArray::free(ResourceMemory &target) { methods[(int)target.allocator]->free(target); }

VulkanDeviceMemoryHandle ResourceManager::MethodsArray::getDeviceMemoryHandle(ResourceMemory &target)
{
  return methods[(int)target.allocator]->getDeviceMemoryHandle(target);
}

const AbstractAllocator::Stats ResourceManager::MethodsArray::printStats()
{
  AbstractAllocator::Stats ret = {};
  for (int i = 0; i < (int)AllocationMethodName::COUNT; ++i)
  {
    AbstractAllocator::Stats st = methods[i]->getStats();
    if (st.allocs)
    {
      debug("vulkan: RMS|    Method %02i", i);
      st.print(st.name, true);
      ret.add(st);
    }
  }
  if (ret.allocs)
    ret.print("Memory type summary");
  return ret;
}
