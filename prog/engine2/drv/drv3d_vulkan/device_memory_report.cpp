#include "device_memory_report.h"
#include <startup/dag_globalSettings.h>
#include "device.h"

using namespace drv3d_vulkan;

#if VK_EXT_device_memory_report

void DeviceMemoryReport::dmr_callback(const VkDeviceMemoryReportCallbackDataEXT *data, void *dmr_ptr)
{
  DeviceMemoryReport &dmr = *(DeviceMemoryReport *)dmr_ptr;
  DeviceMemoryReport::MemoryObjectInfo rdata{data->size, data->objectHandle, data->objectType, data->heapIndex, 0};

  switch (data->type)
  {
    case VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_IMPORT_EXT: rdata.external = 1; [[fallthrough]]; // to addition
    case VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_ALLOCATE_EXT: dmr.addObject(data->memoryObjectId, rdata); break;

    case VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_UNIMPORT_EXT: rdata.external = 1; [[fallthrough]]; // to removal
    case VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_FREE_EXT: dmr.removeObject(data->memoryObjectId, rdata); break;

    case VK_DEVICE_MEMORY_REPORT_EVENT_TYPE_ALLOCATION_FAILED_EXT: dmr.reportOutOfMemory(rdata); break;

    default: logerr("vulkan: unknown device memory report type %u", data->type); break;
  }
}

bool DeviceMemoryReport::init(const PhysicalDeviceSet &dev_info, CallbackDesc &cb_desc)
{
  active = ::dgs_get_settings()->getBlockByNameEx("vulkan")->getBlockByNameEx("debug")->getBool("enableDeviceMemoryReport",
    DAGOR_DBGLEVEL > 0 && dev_info.vendorId != D3D_VENDOR_ARM); //-V560
  active &= dev_info.hasDeviceMemoryReport;

  if (!active)
    return false;

  cb_desc = {VK_STRUCTURE_TYPE_DEVICE_DEVICE_MEMORY_REPORT_CREATE_INFO_EXT, nullptr, 0, &dmr_callback, this};

  activeObjects.clear();
  perObjectTypeStats.clear();
  perHeapStats.resize(dev_info.memoryProperties.memoryHeapCount);

  debug("vulkan: device memory report enabled, reports will be logged with DMR mark");
  return true;
}

void DeviceMemoryReport::dumpInfo()
{
  if (!active)
    return;
  WinAutoLock lock(reportsRWSync);

  VkDeviceSize totalUsed = 0;
  VkDeviceSize totalHeapUsed = 0;

  debug("vulkan: DMR: === per object type info");
  for (const auto &itr : perObjectTypeStats)
  {
    const PerObjectTypeStat &stat = itr.second;

    if (!stat.totalUsedMemory)
      continue;

    totalUsed += stat.totalUsedMemory;
    debug("vulkan: DMR: %-40s %-20s in [%04u+%04u]", formatObjectType(itr.first), byte_size_unit(stat.totalUsedMemory),
      stat.allocs.alive(), stat.imports.alive());
  }

  debug("vulkan: DMR: === per heap info");
  for (uint32_t i = 0; i < perHeapStats.size(); ++i)
  {
    PerHeapStat &stat = perHeapStats[i];

    totalHeapUsed += stat.totalUsedMemory;
    debug("vulkan: DMR: %-20s in [%04u+%04u]", byte_size_unit(stat.totalUsedMemory), stat.allocs.alive(), i);
  }

  debug("vulkan: DMR: %s total, %s heaps, %s imports", byte_size_unit(totalUsed), byte_size_unit(totalHeapUsed),
    byte_size_unit(totalUsed - totalHeapUsed));

// in case something must be debugged around
#if 0
  for (const auto &itr : activeObjects)
  {
    const MemoryObjectInfo &i = itr.second;
    debug("vulkan: DMR: obj %u sz %u heap %i type %s",
      i.objectHandle, i.size, i.external ? -1 : i.heapIndex, formatObjectType(i.objectType));
  }
#endif

  debug("vulkan: DMR: === ");
}

void DeviceMemoryReport::reportOutOfMemory(MemoryObjectInfo data)
{
  debug("vulkan: DMR: %s OOM on asking %s memory for object type %s", active ? "" : "(unreliable)", byte_size_unit(data.size),
    formatObjectType(data.objectType));
  dumpInfo();
}

void DeviceMemoryReport::markDriverBroken(const char *msg)
{
  active = false;
  debug("vulkan: DMR: disabled due to %s", msg);
}

void DeviceMemoryReport::addObject(uint64_t memory_object_id, MemoryObjectInfo data)
{
  if (!active)
    return;
  WinAutoLock lock(reportsRWSync);

  if (activeObjects.count(memory_object_id))
  {
    markDriverBroken("duplicated memoryObjectId");
    return;
  }

  activeObjects[memory_object_id] = data;

  {
    if (!perObjectTypeStats.count(data.objectType))
      perObjectTypeStats[data.objectType] = PerObjectTypeStat{};

    PerObjectTypeStat &stat = perObjectTypeStats[data.objectType];
    if (data.external)
      ++stat.imports.added;
    else
      ++stat.allocs.added;
    stat.totalUsedMemory += data.size;
  }

  if (!data.external)
  {
    if (data.heapIndex >= perHeapStats.size())
    {
      markDriverBroken("invalid heap index at add time");
      return;
    }
    PerHeapStat &stat = perHeapStats[data.heapIndex];

    ++stat.allocs.added;
    stat.totalUsedMemory += data.size;
  }
}

void DeviceMemoryReport::removeObject(uint64_t memory_object_id, MemoryObjectInfo data)
{
  if (!active)
    return;
  WinAutoLock lock(reportsRWSync);

  if (!activeObjects.count(memory_object_id))
  {
    markDriverBroken("memoryObjectId double remove");
    return;
  }

  MemoryObjectInfo &original = activeObjects[memory_object_id];

  if (original.objectType == data.objectType)
  {
    PerObjectTypeStat &stat = perObjectTypeStats[data.objectType];
    if (data.external)
      ++stat.imports.removed;
    else
      ++stat.allocs.removed;
    stat.totalUsedMemory -= original.size;
  }
  else
  {
    markDriverBroken("alloc/free object type mismatch");
    return;
  }

  if (!data.external)
  {
    if (original.heapIndex >= perHeapStats.size())
    {
      markDriverBroken("invalid heap index at remove time");
      return;
    }
    PerHeapStat &stat = perHeapStats[original.heapIndex];

    ++stat.allocs.removed;
    stat.totalUsedMemory -= original.size;
  }

  activeObjects.erase(memory_object_id);
}

#endif // VK_EXT_device_memory_report
