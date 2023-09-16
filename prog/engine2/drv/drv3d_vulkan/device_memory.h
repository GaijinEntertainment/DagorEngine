#pragma once

#include "vulkan_device.h"

namespace drv3d_vulkan
{
enum class DeviceMemoryClass
{
  DEVICE_RESIDENT_IMAGE,
  DEVICE_RESIDENT_BUFFER,
  HOST_RESIDENT_HOST_READ_WRITE_BUFFER,

  HOST_RESIDENT_HOST_READ_ONLY_BUFFER,
#if !_TARGET_C3
  HOST_RESIDENT_HOST_WRITE_ONLY_BUFFER,
#endif
  // special memory type,
  // (NVIDIA with vulkan 1.2 driver and AMD has it)
  // a portion of gpu mem is host
  // visible (256mb).
  DEVICE_RESIDENT_HOST_WRITE_ONLY_BUFFER,

  TRANSIENT_IMAGE,

  COUNT,
  INVALID = COUNT
};

#if _TARGET_C3



#endif

enum class DeviceMemoryConfiguration
{
#if !_TARGET_C3
  // if more than one device memory heap is
  // provided by the driver, then it is assumed
  // that the device has dedicated video memory
  // and device-buffers are not visible to the
  // host and need a staging buffer and copy.
  DEDICATED_DEVICE_MEMORY,
  // Host and device share the same memory heap.
  // All host visible memory types are coherent
  // with an option for cached.
  HOST_SHARED_MEMORY_AUTO,
#endif
  // Similar to HOST_SHARED_MEMORY_AUTO, but
  // Has only host visible memory that is either
  // coherent or cached but never both.
  HOST_SHARED_MEMORY_MANUAL,

  COUNT,
  INVALID = COUNT
};
struct DeviceMemory
{
  VulkanDeviceMemoryHandle memory;
  VkDeviceSize size = 0;
  uint32_t type = (uint32_t)-1;
  uint8_t *pointer = nullptr;
#if USE_NX_MEMORY_TRACKER
  void *trackingPtr;
#endif
};
inline void swap(DeviceMemory &left, DeviceMemory &right)
{
  DeviceMemory t = left;
  left = right;
  right = t;
}
inline bool is_null(const DeviceMemory &mem) { return is_null(mem.memory); }
inline bool operator==(const DeviceMemory &l, const DeviceMemory &r) { return l.memory == r.memory; }
inline bool operator!=(const DeviceMemory &l, const DeviceMemory &r) { return !(l == r); }

// memory pool will select the best suited memory type
struct DeviceMemoryClassAllocationInfo
{
  VkDeviceSize size;
  uint32_t typeMask;
  DeviceMemoryClass memoryClass;
  // target buffer/image for dedicated allocation
  // only used if supported and one of
  // them is not null
  VulkanBufferHandle targetBuffer;
  VulkanImageHandle targetImage;
};

// allocates memory of a specific type
struct DeviceMemoryTypeAllocationInfo
{
  VkDeviceSize size;
  uint32_t typeIndex;
  // target buffer/image for dedicated allocation
  // only used if supported and one of
  // them is not null
  VulkanBufferHandle targetBuffer;
  VulkanImageHandle targetImage;
};

class DeviceMemoryTypeMask
{
  uint32_t value = 0;

public:
  DeviceMemoryTypeMask() = default;
  ~DeviceMemoryTypeMask() = default;

  DeviceMemoryTypeMask(const DeviceMemoryTypeMask &) = default;
  DeviceMemoryTypeMask &operator=(const DeviceMemoryTypeMask &) = default;

  DeviceMemoryTypeMask(uint32_t v) : value(v) {}

  bool operator()(dag::ConstSpan<uint32_t> set, uint32_t index) const { return (value & (1ul << set[index])) != 0; }
};

typedef MaskedSlice<uint32_t, DeviceMemoryTypeMask> DeviceMemoryTypeRange;

class DeviceMemoryPool
{
public:
  struct Heap : VkMemoryHeap
  {
    VkDeviceSize inUse;
    VkDeviceSize limit;
    uint32_t index;
    bool errorOnLimit;
  };
  struct Type
  {
    Heap *heap;
    VkMemoryPropertyFlags flags;
  };

private:
  struct MemoryClassCompare
  {
    VkMemoryPropertyFlags optional;
    VkMemoryPropertyFlags unwanted;
    Tab<Type> &types;

    MemoryClassCompare(DeviceMemoryClass cls, DeviceMemoryConfiguration cfg, Tab<Type> &t);
    int order(uint32_t l, uint32_t r) const;
  };

  Tab<Heap> heaps;
  Tab<Type> types;
  Tab<uint32_t> classTypes[uint32_t(DeviceMemoryClass::COUNT)];
  VkDeviceSize deviceLocalLimit = 0;
  VkDeviceSize hostLocalLimit = 0;
#if !_TARGET_C3
  DeviceMemoryConfiguration memoryConfig = DeviceMemoryConfiguration::DEDICATED_DEVICE_MEMORY;
#else //!_TARGET_C3







#endif
  uint32_t allocations = 0;
  uint32_t frees = 0;

  uint32_t lastAllocationCount = 0;
  uint32_t lastFreeCount = 0;

  void initClassType(DeviceMemoryClass cls);
  DeviceMemoryTypeRange selectMemoryType(VkDeviceSize size, uint32_t mask, DeviceMemoryClass memory_class) const;

public:
  DeviceMemoryPool() = default;
  ~DeviceMemoryPool() = default;

  DeviceMemoryPool(const DeviceMemoryPool &) = delete;
  DeviceMemoryPool &operator=(const DeviceMemoryPool &) = delete;

  DeviceMemoryPool(DeviceMemoryPool &&) = default;
  DeviceMemoryPool &operator=(DeviceMemoryPool &&) = default;

  void init(const VkPhysicalDeviceMemoryProperties &mem_info);
#if VK_EXT_memory_budget
  void applyBudget(const VkPhysicalDeviceMemoryBudgetPropertiesEXT &budget);
#endif

  bool isDeviceLocalMemoryType(uint32_t type) const { return types[type].flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT; }
  bool isHostVisibleMemoryType(uint32_t type) const { return types[type].flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT; }
  bool isCoherentMemoryType(uint32_t type) const { return types[type].flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT; }
  DeviceMemoryTypeRange selectMemoryType(uint32_t mask, DeviceMemoryClass memory_class) const
  {
    return DeviceMemoryTypeRange(classTypes[uint32_t(memory_class)], mask);
  }
  VkDeviceSize getFeeMemoryForMemoryType(uint32_t type) const { return types[type].heap->size - types[type].heap->inUse; }
  DeviceMemory allocate(const DeviceMemoryClassAllocationInfo &info);
  DeviceMemory allocate(const DeviceMemoryTypeAllocationInfo &info);
  void free(const DeviceMemory &memory);
  bool checkAllocationLimits(const DeviceMemoryTypeAllocationInfo &info);
  void logAllocationError(const DeviceMemoryTypeAllocationInfo &info, const char *reason);

  DeviceMemoryConfiguration getMemoryConfiguration() const
  {
#if _TARGET_C3

#else
    return memoryConfig;
#endif
  }
  uint32_t getAllocationCounter() const { return allocations; }
  uint32_t getFreeCounter() const { return frees; }

  template <typename Callback>
  void iterateHeaps(Callback cb) const
  {
    for (const Heap &i : heaps)
      cb(i);
  }

  template <typename Callback>
  void iterateTypes(Callback cb) const
  {
    for (size_t i = 0; i < types.size(); ++i)
    {
      const Type &typeRef = types[i];
      cb(typeRef, i);
    }
  }

  void printStats();
};
} // namespace drv3d_vulkan

DAG_DECLARE_RELOCATABLE(drv3d_vulkan::DeviceMemory);
