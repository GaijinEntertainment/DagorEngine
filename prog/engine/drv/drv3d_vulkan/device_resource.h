// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/rayTrace/dag_drvRayTrace.h> // for D3D_HAS_RAY_TRACING

#include "device_memory.h"
#include "util/backtrace.h"
#include "logic_address.h"

namespace drv3d_vulkan
{

struct ResourceMemory;
class Resource;
class ExecutionContext;
class MemoryHeapResource;

enum class ResourceType
{
  BUFFER,
  IMAGE,
  RENDER_PASS,
  SAMPLER,
  HEAP,
#if D3D_HAS_RAY_TRACING
  AS,
#endif
  COUNT
};

struct AllocationDesc
{
  const Resource &obj;
  MemoryRequirementInfo reqs;
  DeviceMemoryClass memClass;

  uint8_t forceDedicated : 1;
  uint8_t canUseSharedHandle : 1;
  uint8_t temporary : 1;
  uint8_t objectBaked : 1;

  bool isDedicated() const { return forceDedicated || reqs.needDedicated(); }
  bool isSharedHandleAllowed() const { return canUseSharedHandle && !isDedicated(); }

  AllocationDesc() = delete;
};

template <typename ResourceImpl>
class ResourceAlgorithm
{
  ResourceImpl &res;

  void destroyVkObj()
  {
    res.destroyVulkanObject();
    G_ASSERT(!res.getBaseHandle());
  }

  void freeVulkanResource()
  {
    if (!res.isManaged() || !res.getBaseHandle())
      return;

    if (!res.isHandleShared())
      destroyVkObj();
    else
      res.releaseSharedHandle();

    res.freeMemory();
  }

  bool tryReuseHandle(AllocationDesc &allocDsc)
  {
    if (!allocDsc.isSharedHandleAllowed())
      return false;

    allocDsc.reqs.requirements = res.getSharedHandleMemoryReq();
    if (allocDsc.reqs.requirements.size == 0)
      return false;
    return res.tryReuseHandle(allocDsc);
  }

  bool allocVulkanResource(const typename ResourceImpl::Description &desc)
  {
    G_ASSERT(res.isManaged());
    G_ASSERT(!res.getBaseHandle());

    AllocationDesc allocDsc{res};
    desc.fillAllocationDesc(allocDsc);
    if (!tryReuseHandle(allocDsc))
    {
      allocDsc.canUseSharedHandle = false;
      res.createVulkanObject();
      // some vulkan objects need buffer suballocation at creation time
      // if resource allocated memory this way - skip binding & alloc steps
      if (res.getMemoryId() != -1)
        return true;
      allocDsc.reqs = res.getMemoryReq();

      if (res.tryAllocMemory(allocDsc))
      {
        // prevent possible misaligment
        // before memory bind, as vk will fail binding if misaligned
        G_ASSERT((res.getMemory().offset % allocDsc.reqs.requirements.alignment) == 0);
        res.bindMemory();
      }
      else
        return false;
    }
    else
      res.reuseHandle();

    return true;
  }

public:
  ResourceAlgorithm(ResourceImpl &target) : res(target) { G_ASSERT(res.getResType() == res.getImplType()); }

  enum class CreateResult
  {
    OK,
    NON_RESIDENT,
    FAILED
  };

  CreateResult create()
  {
    if (!res.isManaged())
      return CreateResult::OK;

    if (!allocVulkanResource(res.getDescription()))
    {
      // if possible, keep object non resident at creation
      if (!res.nonResidentCreation())
      {
        res.reportOutOfMemory();
        return CreateResult::FAILED;
      }
      return CreateResult::NON_RESIDENT;
    }
    return CreateResult::OK;
  }

  void shutdown()
  {
    res.shutdown();
    freeVulkanResource();
  }

  VkDeviceSize tryEvict(ExecutionContext &ctx, bool evict_used)
  {
    if (!res.isResident() || !res.isManaged() || !res.isEvictable() || res.isEvicting())
      return 0;

    if (res.isUsedInRendering())
    {
      res.clearUsedInRendering();
      if (!evict_used)
        return 0;
    }

    res.makeSysCopy(ctx);
    res.markForEviction();
    return res.getMemory().size;
  }

  void evict()
  {
#if DAGOR_DBGLEVEL > 0
    debug("vulkan: evicting %p:%s:%s [%u Kb]", &res, res.resTypeString(), res.getDebugName(), res.getMemory().size >> 10);
#endif

    G_ASSERT(res.isResident());
    G_ASSERT(res.isManaged());
    G_ASSERT(res.isEvictable());


    res.evict();
    freeVulkanResource();

    if (!res.nonResidentCreation())
    {
      G_ASSERTF(0, "vulkan: resource evictable but non resident creation failed");
    }
  }

  bool makeResident(ExecutionContext &ctx)
  {
    G_ASSERT(!res.isResident());
    G_ASSERT(res.isManaged());
    G_ASSERT(res.getBaseHandle());
    // object allocated memory is not supported in residency logic
    G_ASSERT(res.getMemoryId() == -1);

    const typename ResourceImpl::Description &desc = res.getDescription();
    AllocationDesc allocDsc{res};
    desc.fillAllocationDesc(allocDsc);
    allocDsc.canUseSharedHandle = false;
    allocDsc.reqs = res.getMemoryReq();
    if (res.tryAllocMemory(allocDsc))
    {
      // prevent possible misaligment
      // before memory bind, as vk will fail binding if misaligned
      G_ASSERT((res.getMemory().offset % allocDsc.reqs.requirements.alignment) == 0);
      res.bindMemory();
    }
    else
      return false;

    G_ASSERT(res.isResident());
    res.restoreFromSysCopy(ctx);
    return true;
  }
};

class Resource
{
  VulkanHandle handle = 0;
  ResourceType tid;
  ResourceMemoryId memoryId = -1;
  bool managed : 1;
  bool resident : 1;
  bool evicting : 1;
  bool sharedHandle : 1;
#if VULKAN_TRACK_DEAD_RESOURCE_USAGE
  bool markedDead : 1;
#endif

#if DAGOR_DBGLEVEL > 0
  uint32_t usedInRendering;
#endif

#if VULKAN_RESOURCE_DEBUG_NAMES
  // names are not always come from const memory
  String debugName;
#endif

protected:
  void setHandle(VulkanHandle new_handle);
  void reportToTQL(bool is_allocating);
  void setMemoryId(ResourceMemoryId ext_mem_id) { memoryId = ext_mem_id; }

public:
  Resource(const Resource &) = delete;
  Resource(ResourceType resource_type, bool manage = true) :
    tid(resource_type),
    managed(manage),
    resident(false),
    evicting(false),
    sharedHandle(false)
#if DAGOR_DBGLEVEL > 0
    ,
    usedInRendering(0)
#endif
#if VULKAN_TRACK_DEAD_RESOURCE_USAGE
    ,
    markedDead(false)
#endif
#if VULKAN_RESOURCE_DEBUG_NAMES
    ,
    debugName()
#endif
  {}
  ~Resource() = default;

#if VULKAN_RESOURCE_DEBUG_NAMES
  void setDebugName(const char *name) { debugName = name; }
  const char *getDebugName() const { return debugName; }
  void setStagingDebugName(Resource *target_resource);
#else
  void setDebugName(const char *) {}
  const char *getDebugName() const { return "<unknown>"; }
  void setStagingDebugName(Resource *){};
#endif
  bool tryAllocMemory(const AllocationDesc &dsc);
  bool tryReuseHandle(const AllocationDesc &dsc);
  void markForEviction() { evicting = true; }
  void freeMemory();
  ResourceMemoryId getMemoryId() const { return memoryId; }
  // this call is thread unsafe, resource manager / res algo should be in calling stack
  const ResourceMemory &getMemory() const;

  String printStatLog();
  VulkanHandle getBaseHandle() const { return handle; }
  bool isResident() const { return resident || !managed; }
  bool isManaged() const { return managed; }
  bool isHandleShared() const { return sharedHandle; }
  bool isEvicting() const { return evicting; }
#if DAGOR_DBGLEVEL > 0
  void setUsedInRendering() { usedInRendering++; }
  void clearUsedInRendering() { usedInRendering = 0; }
  bool isUsedInRendering() { return usedInRendering > 1; }
#else
  void setUsedInRendering() {}
  void clearUsedInRendering() {}
  bool isUsedInRendering() { return true; }
#endif

#if VULKAN_TRACK_DEAD_RESOURCE_USAGE
  bool checkDead() const
  {
    if (markedDead)
    {
      D3D_ERROR("vulkan: using dead resource %s-%p:%s", resTypeString(), this, getDebugName());
      return true;
    }
    return false;
  }
  void markDead() { markedDead = true; }
#else
  bool checkDead() const { return false; }
  void markDead(){};
#endif

  ResourceType getResType() const { return tid; };
  bool isBuffer() const { return tid == ResourceType::BUFFER; }
  bool isImage() const { return tid == ResourceType::IMAGE; }
  bool isRenderPass() const { return tid == ResourceType::RENDER_PASS; }
  bool isSampler() const { return tid == ResourceType::SAMPLER; }
  bool isHeap() const { return tid == ResourceType::HEAP; }
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  bool isAccelerationStruct() const { return tid == ResourceType::AS; }
#endif
  const char *resTypeString() const { return resTypeString(tid); }
  void reportOutOfMemory();

  static const char *resTypeString(ResourceType type_id);
};

struct ResourcePlaceableExtend
{
#if DAGOR_DBGLEVEL > 0
  MemoryHeapResource *heap = nullptr;
  void setHeap(MemoryHeapResource *in_heap);
  void releaseHeap();
  // workaround for global state cleanup issues
  void releaseHeapEarly(const char *res_name)
  {
    debug("vulkan: early release for state referenced resource %s placed in heap %p", res_name, heap);
    releaseHeap();
  };
#else
  void releaseHeapEarly(const char *) {}
  void setHeap(MemoryHeapResource *){};
  void releaseHeap() {}
#endif
};

struct ResourceBindlessExtend
{
  void addBindlessSlot(uint32_t slot) { bindlessSlots.push_back(slot); }

  void removeBindlessSlot(uint32_t slot)
  {
    bindlessSlots.erase(eastl::remove_if(begin(bindlessSlots), end(bindlessSlots), [slot](uint32_t r_slot) { return r_slot == slot; }),
      end(bindlessSlots));
  }

  bool isUsedInBindless() { return bindlessSlots.size() > 0; }

protected:
  eastl::vector<uint32_t> bindlessSlots;
};

class ResourceExecutionSyncableExtend
{
  static constexpr uint32_t invalid_sync_op = -1;
  uint32_t firstSyncOp = invalid_sync_op;
  uint32_t lastSyncOp = invalid_sync_op;

  struct RoSeal
  {
    bool requested : 1;
    bool active : 1;
    size_t gpuWorkId;
    LogicAddress reads;
#if EXECUTION_SYNC_TRACK_CALLER > 0
    uint64_t requestCaller;
    uint64_t activateCaller;
#endif
  } roSeal;

public:
  ResourceExecutionSyncableExtend() :
    roSeal{false, false, 0, {VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE}
#if EXECUTION_SYNC_TRACK_CALLER > 0
      ,
      0, 0
#endif
    }
  {}
  ~ResourceExecutionSyncableExtend() {}

  void clearLastSyncOp() { lastSyncOp = invalid_sync_op; }
  void setLastSyncOpIndex(size_t v)
  {
    G_ASSERT(v <= UINT32_MAX);
    lastSyncOp = v;
  }
  size_t getFirstSyncOpIndex() { return firstSyncOp; }
  size_t getLastSyncOpIndex() { return lastSyncOp; }
  bool hasLastSyncOpIndex() { return lastSyncOp != invalid_sync_op; }

  void setLastSyncOpRange(size_t s, size_t e)
  {
    G_ASSERT(s <= UINT32_MAX);
    G_ASSERT(e <= UINT32_MAX);
    firstSyncOp = s;
    lastSyncOp = e;
  }

#if EXECUTION_SYNC_TRACK_CALLER > 0
  String getCallers()
  {
    return String(512, "rqCaller:\n%s\nacCaller:\n%s", backtrace::get_stack_by_hash(roSeal.requestCaller),
      backtrace::get_stack_by_hash(roSeal.activateCaller));
  }
#else
  String getCallers() { return String("<unknown>"); }
#endif

  bool hasRoSeal() { return roSeal.active; }
  bool requestedRoSeal(size_t gpu_work_id) { return roSeal.requested && roSeal.gpuWorkId == gpu_work_id; }
  bool removeRoSealSilently(size_t gpu_work_id)
  {
    roSeal.active = false;
    return roSeal.gpuWorkId != gpu_work_id;
  }

  // opportunistic ro seal set, will fail if someone already writed to object or writes to it later on
  void optionallyActivateRoSeal(uint32_t gpu_work_id)
  {
    if (lastSyncOp == invalid_sync_op)
    {
      roSeal.gpuWorkId = gpu_work_id;
      roSeal.reads = {VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE};
      roSeal.requested = false;
      roSeal.active = true;
#if EXECUTION_SYNC_TRACK_CALLER > 0
      roSeal.activateCaller = backtrace::get_hash();
#endif
    }
  }

  void updateRoSeal(uint32_t gpu_work_id, LogicAddress laddr)
  {
    roSeal.gpuWorkId = gpu_work_id;
    roSeal.reads.merge(laddr);
  }

  LogicAddress getRoSealReads() { return roSeal.reads; }
};

// template for resource implementation
// to be able reuse code and use simple Resource* class at same time
template <typename DescType, typename HandleType, ResourceType resTypeVal>
class ResourceImplBase : public Resource
{
public:
  typedef HandleType Handle;
  typedef DescType Description;

  static ResourceType getImplType() { return resTypeVal; }
  const Description &getDescription() const { return desc; }
  Handle getHandle() const { return Handle(getBaseHandle()); };

  ResourceImplBase(const Description &in_desc, bool manage = true) : Resource(resTypeVal, manage), desc(in_desc) {}

protected:
  Description desc;
};

} // namespace drv3d_vulkan
