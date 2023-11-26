#include "device_resource.h"
#include "device.h"

using namespace drv3d_vulkan;

#if VULKAN_RESOURCE_DEBUG_NAMES
void Resource::setStagingDebugName(Resource *target_resource)
{
  if (target_resource && get_device().getDebugLevel() > 0)
  {
    String newDebugName(256, "%s (%s) staging buf", target_resource->getDebugName(), target_resource->resTypeString());
    setDebugName(newDebugName);
  }
}
#endif

bool Resource::tryAllocMemory(const AllocationDesc &dsc)
{
  if (memoryId != -1)
    freeMemory();

  memoryId = get_device().resources.allocMemory(dsc);
  resident = memoryId != -1 ? getMemory().isValid() : false;
  resident |= dsc.objectBaked && getBaseHandle();
  return resident;
}

bool Resource::tryReuseHandle(const AllocationDesc &dsc)
{
  sharedHandle = tryAllocMemory(dsc);
  if (!sharedHandle && memoryId != -1)
  {
    get_device().resources.freeMemory(memoryId);
    memoryId = -1;
  }
  return sharedHandle;
}

bool Resource::restoreResidency()
{
  resident = get_device().resources.aliasAcquire(memoryId);
  return resident;
}

void Resource::evictMemory()
{
  get_device().resources.aliasRelease(memoryId);
  resident = false;
}

void Resource::freeMemory()
{
  if (!isResident())
    return;

  G_ASSERT(memoryId != -1);
  get_device().resources.freeMemory(memoryId);
  memoryId = -1;
}

bool Resource::isResident() { return resident || !managed; }

bool Resource::isManaged() { return managed; }

const ResourceMemory &Resource::getMemory() const
{
  G_ASSERT(memoryId != -1);
  return get_device().resources.getMemory(memoryId);
}

VulkanHandle Resource::getBaseHandle() const { return handle; }

void Resource::setHandle(VulkanHandle new_handle)
{
  // TODO: ensure that handle changing is safe
  handle = new_handle;
}

static const char *resTypeStrings[(uint32_t)ResourceType::COUNT] = {"buffer", "image", "renderPass",
#if VK_KHR_ray_tracing_pipeline || VK_KHR_ray_query
  "as"
#endif
};

const char *Resource::resTypeString(ResourceType type_id) { return resTypeStrings[(int)type_id]; }

String Resource::printStatLog()
{
  if (managed)
  {
    const ResourceMemory &mem = getMemory();
    String dimensionStr;
    if (isImage())
    {
      VkExtent3D extent = static_cast<Image *>(this)->getMipExtents(0);
      dimensionStr = String(0, "%dx%d", extent.width, extent.height);
    }
    return String(256, "%016p | %016p | %9s | %04i | %02i | %02i | %06uK | {%08lX} | %9s | %s", this, handle, resTypeString(),
      memoryId, (int)mem.memType, (int)mem.allocator, mem.size >> 10, mem.allocatorIndex, dimensionStr, getDebugName());
  }
  else
  {
    return String(256, "%016p | %016p | %9s | %s [not managed/extern]", this, handle, resTypeString(), getDebugName());
  }
}

void Resource::reportOutOfMemory()
{
  // print full info dump so crash can be analyzed for good
  get_device().resources.printStats(true, true);
  fatal("vulkan: Out of memory. Try lowering graphics settings and/or closing other memory consuming applications");
}