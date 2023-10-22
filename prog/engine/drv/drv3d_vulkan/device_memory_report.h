#pragma once
#include "driver.h"
#include "physical_device_set.h"
#include <EASTL/hash_map.h>
#include <osApiWrappers/dag_critSec.h>

namespace drv3d_vulkan
{

#if VK_EXT_device_memory_report

class DeviceMemoryReport
{
  bool active = false;

  struct CreationCounter
  {
    uint64_t added;
    uint64_t removed;

    uint64_t alive() const { return added - removed; }
  };

  struct PerObjectTypeStat
  {
    CreationCounter allocs;
    CreationCounter imports;
    VkDeviceSize totalUsedMemory;
  };

  struct PerHeapStat
  {
    CreationCounter allocs;
    VkDeviceSize totalUsedMemory;
  };

  struct MemoryObjectInfo
  {
    VkDeviceSize size;
    uint64_t objectHandle;
    VkObjectType objectType;
    uint32_t heapIndex : 31;
    uint32_t external : 1;
  };

  eastl::hash_map<VkObjectType, PerObjectTypeStat> perObjectTypeStats;
  eastl::vector<PerHeapStat> perHeapStats;
  eastl::hash_map<uint64_t, MemoryObjectInfo> activeObjects;

  // callback can be called from anywhere
  WinCritSec reportsRWSync;

  static void VKAPI_PTR dmr_callback(const VkDeviceMemoryReportCallbackDataEXT *data, void *dmr_ptr);

  void markDriverBroken(const char *msg);
  void reportOutOfMemory(MemoryObjectInfo data);
  void addObject(uint64_t memory_object_id, MemoryObjectInfo data);
  void removeObject(uint64_t memory_object_id, MemoryObjectInfo data);

public:
  typedef VkDeviceDeviceMemoryReportCreateInfoEXT CallbackDesc;

  DeviceMemoryReport() = default;

  bool init(const PhysicalDeviceSet &dev_info, CallbackDesc &cb_desc);
  void dumpInfo();
};

#else // VK_EXT_device_memory_report

class DeviceMemoryReport
{
public:
  struct CallbackDesc
  {
    void *pNext;
  };

  DeviceMemoryReport() = default;

  bool init(const PhysicalDeviceSet &, CallbackDesc &) { return false; }
  void dumpInfo() {}
};

#endif // VK_EXT_device_memory_report

} // namespace drv3d_vulkan
