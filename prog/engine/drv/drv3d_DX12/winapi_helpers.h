#pragma once

#include <windows.h>
#include <EASTL/unique_ptr.h>


struct UnloadLibHandler
{
  typedef HMODULE pointer;
  void operator()(HMODULE lib)
  {
    if (lib)
    {
      FreeLibrary(lib);
    }
  }
};

using LibPointer = eastl::unique_ptr<HMODULE, UnloadLibHandler>;

struct GenericHandleHandler
{
  typedef HANDLE pointer;
  void operator()(pointer h)
  {
    if (h != nullptr && h != INVALID_HANDLE_VALUE)
      CloseHandle(h);
  }
};

using HandlePointer = eastl::unique_ptr<HANDLE, GenericHandleHandler>;
using EventPointer = eastl::unique_ptr<HANDLE, GenericHandleHandler>;

struct VirtaulAllocMemoryHandler
{
  void operator()(void *ptr) { VirtualFree(ptr, 0, MEM_RELEASE); }
};

template <typename T>
using VirtualAllocPtr = eastl::unique_ptr<T, VirtaulAllocMemoryHandler>;
