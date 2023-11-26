#pragma once

#include "driver.h"
#include "device_memory.h"
#include "device_resource.h"
#include "util/free_list.h"

namespace drv3d_vulkan
{

struct ResourceMemory;
class AbstractAllocator;

enum class SubAllocationMethod
{
  MEM_OFFSET,
  BUF_OFFSET
};

class DeviceMemoryPage
{
  SubAllocationMethod subType;
  DeviceMemory mem;
  VulkanBufferHandle buffer;

public:
  static uint32_t toAllocatorIndex(uint32_t cat, uint32_t page);
  static uint32_t getPageIdx(uint32_t allocator_idx);
  static uint32_t getCatIdx(uint32_t allocator_idx);

  bool init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize in_size);
  void shutdown();

  bool isAllocAllowedFor(const AllocationDesc &desc);
  void useOffset(ResourceMemory &target, VkDeviceSize offset);

  bool isValid() { return !is_null(mem); }
  VkDeviceSize getAllocatedMemorySize() { return mem.size; }
  VulkanDeviceMemoryHandle getDeviceMemoryHandle() { return mem.memory; }
};

class FixedOccupancyPage : public DeviceMemoryPage
{
  Tab<VkDeviceSize> freePlaces;
  VkDeviceSize totalPlaces;

public:
  bool init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize page_size, VkDeviceSize block_size);
  void shutdown();

  bool occupy(ResourceMemory &target, const AllocationDesc &desc, VkDeviceSize mix_gran);
  void free(ResourceMemory &target);

  VkDeviceSize getUnusedMemorySize();
  uint32_t getOccupiedCount();

  String printMemoryMap(int32_t page_index);
};

class FreeListPage : public DeviceMemoryPage
{
  FreeList<VkDeviceSize> freeList;
  uint32_t refs;

public:
  bool init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize page_size);
  void shutdown();

  bool occupy(ResourceMemory &target, const AllocationDesc &desc, VkDeviceSize mix_gran);
  void free(ResourceMemory &target);

  VkDeviceSize getUnusedMemorySize() { return freeList.getFreeSize(); }
  uint32_t getOccupiedCount() { return refs; }

  String printMemoryMap(uint32_t page_index);
};

class LinearOccupancyPage : public DeviceMemoryPage
{
  uint32_t refs;
  VkDeviceSize pushOffset;
  VkDeviceSize usableSize;

public:
  bool init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize page_size);
  void shutdown();

  bool occupy(ResourceMemory &target, const AllocationDesc &desc, VkDeviceSize mix_gran);
  void free(ResourceMemory &target);

  VkDeviceSize getUnusedMemorySize();
  uint32_t getOccupiedCount() { return refs; }

  String printMemoryMap(uint32_t page_index);
};

} // namespace drv3d_vulkan
