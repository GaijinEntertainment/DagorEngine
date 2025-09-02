// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_adjpow2.h>
#include "globals.h"
#include "device_memory.h"
#include "device_memory_pages.h"
#include "buffer_resource.h"
#include "resource_manager.h"
#include "vulkan_allocation_callbacks.h"

using namespace drv3d_vulkan;

class PageMemMap
{
  static const int maxDencity = 8;
  static const int mapSize = 32;
  static const int totalIndexes = maxDencity * mapSize;
  static const char *symbols[maxDencity + 1];


  VkDeviceSize pageSize;
  VkDeviceSize step;
  uint32_t pageIndex;

  StaticTab<uint8_t, mapSize> dencityArray;

public:
  PageMemMap(VkDeviceSize pg_size, uint32_t page_index) : pageSize(pg_size), pageIndex(page_index)
  {
    dencityArray.assign(mapSize, (1 << maxDencity) - 1);
    step = pageSize / totalIndexes;
    // make sure page size good to use
    G_ASSERT((pageSize & step) == 0);
  }

  void markFreeBlock(VkDeviceSize offset, VkDeviceSize size)
  {
    //  █    ░    <- resulting map
    //|....|....| <- dencity
    //  ^
    //  index in sub block of map block
    VkDeviceSize index = offset / step;
    VkDeviceSize endIndex = (offset + size) / step;

    G_ASSERT(index < totalIndexes);
    while (index < endIndex)
    {
      int charIndex = index / maxDencity;
      int bitIndex = index % maxDencity;

      dencityArray[charIndex] &= ~(1 << bitIndex);
      ++index;
    }
  }

  String print()
  {
    String map("");
    for (int i = 0; i < mapSize; ++i)
      map += symbols[__popcount(dencityArray[i])];
    return String(128, "vulkan: RMS| page {%08lX} %03uMb map [%s]", pageIndex, pageSize >> 20, map);
  }
};

// utf8 encoded multi byte symbols
const char *PageMemMap::symbols[PageMemMap::maxDencity + 1] = {" ", ".", ":", "|", "║", "░", "▒", "▓", "█"};

//////////////////////////////

uint32_t DeviceMemoryPage::toAllocatorIndex(uint32_t cat, uint32_t page)
{
  G_ASSERT(cat < 0xFFFF);
  G_ASSERT(page < 0xFFFF);
  return (cat << 16) | page;
}

uint32_t DeviceMemoryPage::getPageIdx(uint32_t allocator_idx) { return allocator_idx & 0xFFFF; }

uint32_t DeviceMemoryPage::getCatIdx(uint32_t allocator_idx) { return allocator_idx >> 16; }

bool DeviceMemoryPage::init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize in_size)
{
  VulkanDevice &vkDev = Globals::VK::dev;
  subType = suballoc_method;
  VkDeviceSize correctedSize = in_size;

  if (suballoc_method == SubAllocationMethod::BUF_OFFSET)
  {
    // Create buffer
    VkBufferCreateInfo bci;
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.pNext = NULL;
    bci.flags = 0;
    bci.size = in_size;
    bci.usage = Buffer::getUsage(Globals::Mem::pool.isDeviceLocalMemoryType(allocator->getMemType())
                                   ? DeviceMemoryClass::DEVICE_RESIDENT_BUFFER
                                   : DeviceMemoryClass::HOST_RESIDENT_HOST_READ_WRITE_BUFFER);
    bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bci.queueFamilyIndexCount = 0;
    bci.pQueueFamilyIndices = NULL;
    const VkResult resCode = VULKAN_CHECK_RESULT(vkDev.vkCreateBuffer(vkDev.get(), &bci, VKALLOC(buffer), ptr(buffer)));
    if (resCode != VK_SUCCESS)
      return false;

    MemoryRequirementInfo ret = get_memory_requirements(buffer);
    correctedSize = ret.requirements.size;
    if (!correctedSize)
    {
      vkDev.vkDestroyBuffer(vkDev.get(), buffer, VKALLOC(buffer));
      return false;
    }
  }

  DeviceMemoryTypeAllocationInfo devMemDsc;
  devMemDsc.typeIndex = allocator->getMemType();
  devMemDsc.size = correctedSize;

  mem = Globals::Mem::pool.allocate(devMemDsc);
  if (is_null(mem))
    return false;

  if (suballoc_method == SubAllocationMethod::BUF_OFFSET)
  {
    const VkResult resCode = VULKAN_CHECK_RESULT(vkDev.vkBindBufferMemory(vkDev.get(), buffer, mem.memory, 0));
    if (resCode != VK_SUCCESS)
    {
      vkDev.vkDestroyBuffer(vkDev.get(), buffer, VKALLOC(buffer));
      return false;
    }
  }

  return true;
}

void DeviceMemoryPage::shutdown()
{
  if (buffer)
  {
    VulkanDevice &vkDev = Globals::VK::dev;
    vkDev.vkDestroyBuffer(vkDev.get(), buffer, VKALLOC(buffer));
    buffer = VulkanNullHandle();
  }
  Globals::Mem::pool.free(mem);
  mem.memory = VulkanNullHandle();
}

bool DeviceMemoryPage::isAllocAllowedFor(const AllocationDesc &desc)
{
  if ((subType == SubAllocationMethod::BUF_OFFSET) && !desc.obj.isBuffer())
    return false;

  return true;
}

void DeviceMemoryPage::useOffset(ResourceMemory &target, VkDeviceSize offset)
{
  G_ASSERT(mem.size > offset);
  target.handle = subType == SubAllocationMethod::BUF_OFFSET ? generalize(buffer) : generalize(mem.memory);
  target.offset = offset;
  target.mappedPointer = mem.pointer;
}

//////////////////////////////

bool FixedOccupancyPage::init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize page_size,
  VkDeviceSize block_size)
{
  if (!DeviceMemoryPage::init(allocator, suballoc_method, page_size))
    return false;

  VkDeviceSize cofs = 0;
  totalPlaces = 0;
  while ((cofs + block_size) < page_size)
  {
    freePlaces.push_back(cofs);
    cofs += block_size;
    ++totalPlaces;
  }

  return true;
}

void FixedOccupancyPage::shutdown()
{
  G_ASSERT(freePlaces.size() == totalPlaces);
  clear_and_shrink(freePlaces);
  DeviceMemoryPage::shutdown();
}

bool FixedOccupancyPage::occupy(ResourceMemory &target, const AllocationDesc &desc, VkDeviceSize)
{
  G_ASSERT(isValid());

  if (freePlaces.empty() || !isAllocAllowedFor(desc))
    return false;

  useOffset(target, freePlaces.back());
  freePlaces.pop_back();

  return true;
}

void FixedOccupancyPage::free(ResourceMemory &target)
{
  freePlaces.push_back(target.offset);
  // do not keep hysteresis things, relay on HOT deletion delayment
  // for realloc spike avoidance
  if (freePlaces.size() == totalPlaces)
    shutdown();
}

VkDeviceSize FixedOccupancyPage::getUnusedMemorySize() { return freePlaces.size() * getAllocatedMemorySize() / totalPlaces; }

uint32_t FixedOccupancyPage::getOccupiedCount() { return totalPlaces - freePlaces.size(); }

String FixedOccupancyPage::printMemoryMap(int32_t page_index)
{
  VkDeviceSize blockSize = getAllocatedMemorySize() / totalPlaces;

  PageMemMap map(getAllocatedMemorySize(), page_index);
  for (VkDeviceSize i : freePlaces)
    map.markFreeBlock(i, blockSize);
  return map.print();
}

//////////////////////////////

bool FreeListPage::init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize page_size)
{
  if (!DeviceMemoryPage::init(allocator, suballoc_method, page_size))
    return false;
  freeList.init(page_size);
  refs = 0;

  return true;
}

void FreeListPage::shutdown()
{
  G_ASSERT(freeList.checkEmpty(getAllocatedMemorySize()));
  G_ASSERT(refs == 0);
  freeList.reset();
  DeviceMemoryPage::shutdown();
}

bool FreeListPage::occupy(ResourceMemory &target, const AllocationDesc &desc, VkDeviceSize mix_gran)
{
  G_ASSERT(isValid());

  VkDeviceSize size = desc.reqs.requirements.size;
  VkDeviceSize align = max<VkDeviceSize>(desc.reqs.requirements.alignment, mix_gran);

  if ((freeList.getFreeSize() <= size) || !isAllocAllowedFor(desc))
    return false;

  int block = freeList.findBest(size, align);

  if (block == -1)
    return false;

  target.size = size;
  useOffset(target, freeList.allocate(block, size, align));
  ++refs;

  return true;
}

void FreeListPage::free(ResourceMemory &target)
{
  freeList.free(target.offset, target.size);
  --refs;

  if (!refs)
    shutdown();
}

String FreeListPage::printMemoryMap(uint32_t page_index)
{
  PageMemMap map(getAllocatedMemorySize(), page_index);
  freeList.iterate([&map](VkDeviceSize offset, VkDeviceSize size) { map.markFreeBlock(offset, size); });
  return map.print();
}

bool LinearOccupancyPage::init(AbstractAllocator *allocator, SubAllocationMethod suballoc_method, VkDeviceSize page_size)
{
  if (!DeviceMemoryPage::init(allocator, suballoc_method, page_size))
    return false;

  usableSize = page_size;
  refs = 0;
  pushOffset = 0;
  return true;
}

void LinearOccupancyPage::shutdown()
{
  G_ASSERT(refs == 0);
  pushOffset = 0;
  DeviceMemoryPage::shutdown();
}

bool LinearOccupancyPage::occupy(ResourceMemory &target, const AllocationDesc &desc, VkDeviceSize mix_gran)
{
  VkDeviceSize size = desc.reqs.requirements.size;
  VkDeviceSize invAlign = max<VkDeviceSize>(desc.reqs.requirements.alignment, mix_gran) - 1;

  VkDeviceSize targetOffset = (pushOffset + invAlign) & ~invAlign;
  VkDeviceSize newOffset = targetOffset + size;

  if (newOffset < usableSize)
  {
    useOffset(target, targetOffset);
    pushOffset = newOffset;
    ++refs;
    return true;
  }
  else
    return false;
}

void LinearOccupancyPage::free(ResourceMemory &)
{
  --refs;
  // do not free memory, as we can reuse it right after this cleanup
  if (!refs)
    pushOffset = 0;
}

VkDeviceSize LinearOccupancyPage::getUnusedMemorySize() { return getAllocatedMemorySize() - pushOffset; }

String LinearOccupancyPage::printMemoryMap(uint32_t page_index)
{
  PageMemMap map(getAllocatedMemorySize(), page_index);
  map.markFreeBlock(pushOffset, getUnusedMemorySize());
  return map.print();
}