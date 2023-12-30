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
  if (!resident && memoryId != -1)
  {
    get_device().resources.freeMemory(memoryId);
    memoryId = -1;
  }
  return resident;
}

bool Resource::tryReuseHandle(const AllocationDesc &dsc)
{
  sharedHandle = tryAllocMemory(dsc);
  return sharedHandle;
}

void Resource::markForEviction() { evicting = true; }

void Resource::freeMemory()
{
  if (!isResident())
    return;

  G_ASSERT(memoryId != -1);
  get_device().resources.freeMemory(memoryId);
  memoryId = -1;
  resident = false;
  evicting = false;
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
    if (resident)
    {
      const ResourceMemory &mem = getMemory();
      String dimensionStr;
      if (isImage())
      {
        VkExtent3D extent = static_cast<Image *>(this)->getMipExtents(0);
        dimensionStr = String(0, "%dx%d", extent.width, extent.height);
      }
      return String(256, "%016p | %016p | %9s | %04i | %02i | %02i | %06uK | {%08lX} | %9s | %6s | %s", this, handle, resTypeString(),
        memoryId, (int)mem.memType, (int)mem.allocator, mem.size >> 10, mem.allocatorIndex, dimensionStr,
        isUsedInRendering() ? "used" : "unused", getDebugName());
    }
    else
    {
      String dimensionStr;
      if (isImage())
      {
        VkExtent3D extent = static_cast<Image *>(this)->getMipExtents(0);
        dimensionStr = String(0, "%dx%d", extent.width, extent.height);
      }
      return String(256, "%016p | %016p | %9s | <evicted> | %9s | %s", this, handle, resTypeString(), dimensionStr, getDebugName());
    }
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
  DAG_FATAL("vulkan: Out of memory. Try lowering graphics settings and/or closing other memory consuming applications");
}