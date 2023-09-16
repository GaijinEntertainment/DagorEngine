#include "Precompiled.h"
#include "Platform.h"

void* VirtualMemoryAlloc(size_t size)
{
  return VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void VirtualMemoryFree(void* data, size_t size)
{
  (void)size;
  VirtualFree(data, 0, MEM_RELEASE);
}
