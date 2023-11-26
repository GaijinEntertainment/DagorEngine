#pragma once

#include "driver.h"
#include "device_memory.h"
#include "util/backtrace.h"

namespace drv3d_vulkan
{

struct ResourceMemory;
class Resource;
struct ContextBackend;

enum class ResourceType
{
  BUFFER,
  IMAGE,
  RENDER_PASS,
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
    if (!res.isManaged())
      return;

    G_ASSERT(res.getBaseHandle());

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
    return res.tryReuseHandle(allocDsc);
  }

  bool allocVulkanResource(const typename ResourceImpl::Description &desc)
  {
    if (!res.isManaged())
      return true;

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

  void create()
  {
    if (!allocVulkanResource(res.getDescription()))
    {
      // if possible, keep object non resident at creation
      if (!res.nonResidentCreation())
        res.reportOutOfMemory();
    }
  }

  void shutdown()
  {
    res.shutdown();
    freeVulkanResource();
  }

  // should be called in exec context
  VkDeviceSize evict()
  {
    if (!res.isResident() || !res.isManaged())
      return 0;

    if (!res.isEvictable())
      return 0;

#if DAGOR_DBGLEVEL > 0
    debug("vulkan: evicting %p:%s:%s [%u Kb]", &res, res.resTypeString(), res.getDebugName(), res.getMemory().size >> 10);
#endif

    res.makeSysCopy();
    VkDeviceSize evictAmount = res.getMemory().size;
    res.evictMemory();
    return evictAmount;
  }

  // should be called in exec context
  bool makeResident()
  {
    G_ASSERT(!res.isResident());
    G_ASSERT(res.isManaged());

    // true = we reused old memory
    if (!res.restoreResidency())
    {
      // otherwise we need to recreate object
      if (res.getBaseHandle() && !res.isHandleShared())
        destroyVkObj();

      freeVulkanResource();
      if (!allocVulkanResource(res.getDescription()))
        return false;
    }

    res.restoreFromSysCopy();
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
  bool sharedHandle : 1;
#if VULKAN_TRACK_DEAD_RESOURCE_USAGE
  bool markedDead : 1;
#endif
#if VULKAN_RESOURCE_DEBUG_NAMES
  // names are not always come from const memory
  String debugName;
#endif

protected:
  void setHandle(VulkanHandle new_handle);

public:
  Resource(const Resource &) = delete;
  Resource(ResourceType resource_type, bool manage = true) :
    tid(resource_type),
    managed(manage),
    resident(false),
    sharedHandle(false)
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
  bool restoreResidency();
  void evictMemory();
  void freeMemory();
  ResourceMemoryId getMemoryId() { return memoryId; }
  // this call is thread unsafe, resource manager / res algo should be in calling stack
  const ResourceMemory &getMemory() const;

  String printStatLog();
  VulkanHandle getBaseHandle() const;
  bool isResident();
  bool isManaged();
  bool isHandleShared() { return sharedHandle; }

#if VULKAN_TRACK_DEAD_RESOURCE_USAGE
  bool checkDead() const
  {
    if (markedDead)
    {
      logerr("vulkan: using dead resource %s-%p:%s", resTypeString(), this, getDebugName());
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
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  bool isAccelerationStruct() const { return tid == ResourceType::AS; }
#endif
  const char *resTypeString() const { return resTypeString(tid); }
  void reportOutOfMemory();

  static const char *resTypeString(ResourceType type_id);
};

class ResourceExecutionSyncableExtend
{
  static constexpr size_t invalid_sync_op = -1;
  size_t lastSyncOp = invalid_sync_op;

  struct RoSeal
  {
    bool requested : 1;
    bool active : 1;
    size_t gpuWorkId;
#if EXECUTION_SYNC_TRACK_CALLER > 0
    uint64_t requestCaller;
    uint64_t activateCaller;
#endif
  } roSeal;

public:
  ResourceExecutionSyncableExtend() : roSeal
  {
    false, false, 0
#if EXECUTION_SYNC_TRACK_CALLER > 0
      ,
      0, 0
#endif
  }
  {}
  ~ResourceExecutionSyncableExtend() {}

  void clearLastSyncOp() { lastSyncOp = invalid_sync_op; }
  void setLastSyncOpIndex(size_t v) { lastSyncOp = v; }
  size_t getLastSyncOpIndex() { return lastSyncOp; }
  bool hasLastSyncOpIndex() { return lastSyncOp != invalid_sync_op; }

  void requestRoSeal(size_t gpu_work_id)
  {
    roSeal.requested = true;
    roSeal.gpuWorkId = gpu_work_id;
#if EXECUTION_SYNC_TRACK_CALLER > 0
    roSeal.requestCaller = backtrace::get_hash();
#endif
  }

  void activateRoSeal()
  {
    roSeal.requested = false;
    roSeal.active = true;
#if EXECUTION_SYNC_TRACK_CALLER > 0
    roSeal.activateCaller = backtrace::get_hash();
#endif
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
  bool tryRemoveRoSeal(size_t gpu_work_id)
  {
    if (roSeal.gpuWorkId != gpu_work_id)
    {
      roSeal.active = false;
      return true;
    }
    return false;
  }

  // opportunistic ro seal set, will fail if someone already writed to object or writes to it later on
  void optionallyActivateRoSeal(uint32_t gpu_work_id)
  {
    if (lastSyncOp == invalid_sync_op)
    {
      roSeal.gpuWorkId = gpu_work_id;
      activateRoSeal();
    }
  }
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
