#pragma once

#include <string.h>

void* VirtualMemoryAlloc(size_t size);
void VirtualMemoryFree(void* data, size_t size);

inline void* VirtualMemoryRealloc(void* old_data, size_t old_size, size_t new_size)
{
  size_t copy_length = (old_size < new_size) ? old_size : new_size;
  
  void* new_data = VirtualMemoryAlloc(new_size);
  memcpy(new_data, old_data, copy_length);

  VirtualMemoryFree(old_data, old_size);
  return new_data;
}