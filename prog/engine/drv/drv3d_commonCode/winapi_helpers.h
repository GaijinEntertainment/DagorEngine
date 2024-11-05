// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <windows.h>
#include <EASTL/span.h>
#include <EASTL/string_view.h>
#include <EASTL/tuple.h>
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


struct RegistryKeyHandler
{
  typedef HKEY pointer;
  void operator()(HKEY lib)
  {
    if (lib)
    {
      RegCloseKey(lib);
    }
  }
};
using RegistryKeyPtr = eastl::unique_ptr<HKEY, RegistryKeyHandler>;

struct RegistryKey
{
  RegistryKeyPtr ptr;

  RegistryKey() = default;
  RegistryKey(RegistryKey &&) = default;
  RegistryKey &operator=(RegistryKey &&) = default;
  ~RegistryKey() = default;

  explicit RegistryKey(HKEY key) : ptr{key} {}

  RegistryKey(HKEY key, LPCSTR sub_key, DWORD options, REGSAM sam) { open(key, sub_key, options, sam); }

  RegistryKey(const RegistryKey &key, LPCSTR sub_key, DWORD options, REGSAM sam) { open(key.ptr.get(), sub_key, options, sam); }

  LSTATUS open(HKEY key, LPCSTR sub_key, DWORD options, REGSAM sam);

  RegistryKey openSubKey(LPCSTR sub_key, DWORD options, REGSAM sam) const
  {
    RegistryKey newKey;
    newKey.open(ptr.get(), sub_key, options, sam);
    return newKey;
  }

  eastl::tuple<DWORD, DWORD> querySubKeyCountAndMaxLength() const;

  eastl::tuple<DWORD, DWORD, DWORD> queryValueCountMaxLengthAndSize() const;

  eastl::string_view enumKeyName(DWORD index, eastl::span<char> buffer);

  eastl::string_view enumValueName(DWORD index, eastl::span<char> buffer);

  eastl::tuple<eastl::string_view, eastl::span<char>> enumValueNameAndValue(DWORD index, eastl::span<char> name_buffer,
    eastl::span<char> data_buffer);

  explicit operator bool() const { return static_cast<bool>(ptr); }
};
