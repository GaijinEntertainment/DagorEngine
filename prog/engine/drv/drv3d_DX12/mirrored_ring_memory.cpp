// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "mirrored_ring_memory.h"

#include <EASTL/optional.h>
#include <Windows.h>

// Placeholder flags are not defined by every SDK the driver builds against.
// Hoisted above the platform split so the Xbox translation unit sees them too.
#ifndef MEM_RESERVE_PLACEHOLDER
#define MEM_RESERVE_PLACEHOLDER 0x00040000
#endif
#ifndef MEM_REPLACE_PLACEHOLDER
#define MEM_REPLACE_PLACEHOLDER 0x00004000
#endif
#ifndef MEM_PRESERVE_PLACEHOLDER
#define MEM_PRESERVE_PLACEHOLDER 0x00000002
#endif

#if _TARGET_XBOX

#include "mirrored_ring_memory_xbox.inl.h"

#else

namespace
{
// The MEM_EXTENDED_PARAMETER* argument is always null here, so it is typed as
// void* to avoid depending on the SDK's extended-parameter definitions.
using PFN_VirtualAlloc2 = PVOID(WINAPI *)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, void *, ULONG);
using PFN_MapViewOfFile3 = PVOID(WINAPI *)(HANDLE, HANDLE, PVOID, ULONG64, SIZE_T, ULONG, ULONG, void *, ULONG);
using PFN_UnmapViewOfFile2 = BOOL(WINAPI *)(HANDLE, PVOID, ULONG);

struct PlaceholderApi
{
  PFN_VirtualAlloc2 virtualAlloc2 = nullptr;
  PFN_MapViewOfFile3 mapViewOfFile3 = nullptr;
  PFN_UnmapViewOfFile2 unmapViewOfFile2 = nullptr;
};

// Resolved once on first use; engaged only when all three functions are present,
// so callers can branch with the optional monadic operations.
const eastl::optional<PlaceholderApi> &get_placeholder_api()
{
  static eastl::optional<PlaceholderApi> api = []() -> eastl::optional<PlaceholderApi> {
    HMODULE lib = GetModuleHandleW(L"kernelbase.dll");
    if (!lib)
      return eastl::nullopt;

    PlaceholderApi a;
    // Assign through a FARPROC lvalue so this is a plain assignment rather than
    // a function-pointer conversion, which would otherwise trip MSVC C4191.
    reinterpret_cast<FARPROC &>(a.virtualAlloc2) = GetProcAddress(lib, "VirtualAlloc2");
    reinterpret_cast<FARPROC &>(a.mapViewOfFile3) = GetProcAddress(lib, "MapViewOfFile3");
    reinterpret_cast<FARPROC &>(a.unmapViewOfFile2) = GetProcAddress(lib, "UnmapViewOfFile2");
    if (a.virtualAlloc2 && a.mapViewOfFile3 && a.unmapViewOfFile2)
      return a;
    return eastl::nullopt;
  }();
  return api;
}

size_t allocation_granularity()
{
  SYSTEM_INFO si{};
  GetSystemInfo(&si);
  return si.dwAllocationGranularity;
}

size_t round_up(size_t value, size_t multiple) { return ((value + multiple - 1) / multiple) * multiple; }

using drv3d_dx12::MirrorError;

// Race-free path available on Windows 10 1803+.
// Note that PVS can not cope with split memory ranges and that memory mappings increase the ref counter of mappings, that
// is why all the 586 and 774 warnings are toggled off in so many places.
dag::Expected<uint8_t *, MirrorError> create_placeholder(const PlaceholderApi &api, size_t capacity)
{
  uint8_t *placeholder = static_cast<uint8_t *>(
    api.virtualAlloc2(nullptr, nullptr, 2 * capacity, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0));
  if (!placeholder)
    return dag::Unexpected{MirrorError::Reserve};

  // Split the reservation into two adjacent placeholders of "capacity" bytes.
  if (!VirtualFree(placeholder, capacity, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER))
  {
    VirtualFree(placeholder, 0, MEM_RELEASE);
    return dag::Unexpected{MirrorError::Split};
  }

  HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
    static_cast<DWORD>(static_cast<uint64_t>(capacity) >> 32), static_cast<DWORD>(capacity & 0xFFFFFFFFu), nullptr);
  if (!section)
  {
    VirtualFree(placeholder, 0, MEM_RELEASE);            //-V586
    VirtualFree(placeholder + capacity, 0, MEM_RELEASE); //-V586 //-V774
    return dag::Unexpected{MirrorError::Section};
  }

  void *v0 =
    api.mapViewOfFile3(section, GetCurrentProcess(), placeholder, 0, capacity, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
  void *v1 = api.mapViewOfFile3(section, GetCurrentProcess(), placeholder + capacity, 0, capacity, MEM_REPLACE_PLACEHOLDER,
    PAGE_READWRITE, nullptr, 0);
  CloseHandle(section); // mapped views hold their own reference

  if (!v0 || !v1)
  {
    if (v0)
      api.unmapViewOfFile2(GetCurrentProcess(), placeholder, MEM_PRESERVE_PLACEHOLDER);
    if (v1)
      api.unmapViewOfFile2(GetCurrentProcess(), placeholder + capacity, MEM_PRESERVE_PLACEHOLDER);
    VirtualFree(placeholder, 0, MEM_RELEASE);
    VirtualFree(placeholder + capacity, 0, MEM_RELEASE); //-V586 //-V774
    return dag::Unexpected{MirrorError::MapView};
  }

  return placeholder;
}

// Windows 7+ fallback. Racy between the probe free and the maps, so it retries.
dag::Expected<uint8_t *, MirrorError> create_classic(size_t capacity)
{
  HANDLE section = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
    static_cast<DWORD>(static_cast<uint64_t>(capacity) >> 32), static_cast<DWORD>(capacity & 0xFFFFFFFFu), nullptr);
  if (!section)
    return dag::Unexpected{MirrorError::Section};

  constexpr int max_attempts = 16;
  for (int attempt = 0; attempt < max_attempts; ++attempt)
  {
    // Probe for a free 2*capacity range, then release it and race to map it.
    void *probe = VirtualAlloc(nullptr, 2 * capacity, MEM_RESERVE, PAGE_NOACCESS);
    if (!probe)
      break;
    VirtualFree(probe, 0, MEM_RELEASE);

    uint8_t *base = static_cast<uint8_t *>(probe);
    void *v0 = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, capacity, base);            //-V774
    void *v1 = MapViewOfFileEx(section, FILE_MAP_ALL_ACCESS, 0, 0, capacity, base + capacity); //-V774
    if (v0 && v1)
    {
      CloseHandle(section);
      return base; //-V774
    }
    if (v0)
      UnmapViewOfFile(v0);
    if (v1)
      UnmapViewOfFile(v1);
    // lost the race, try another address
  }

  CloseHandle(section);
  return dag::Unexpected{MirrorError::ProbeExhausted};
}
} // namespace

namespace drv3d_dx12
{
dag::Expected<MirroredRingMemory, MirrorError> MirroredRingMemory::create(size_t requested_bytes)
{
  if (requested_bytes == 0)
    return dag::Unexpected{MirrorError::ZeroSize};

  const size_t capacity = round_up(requested_bytes, allocation_granularity());

  // The placeholder API is resolved once and never changes, so its presence
  // alone selects the create path; the classic recipe is evaluated lazily by
  // or_else only when the placeholder API is absent. The mapped base is then
  // wrapped into a MirroredRingMemory; on failure the MirrorError propagates.
  return get_placeholder_api()
    .transform([&](const PlaceholderApi &api) { return create_placeholder(api, capacity); })
    .or_else([&] { return eastl::make_optional(create_classic(capacity)); })
    .value()
    .transform([capacity](uint8_t *base) {
      MirroredRingMemory result;
      result.base = base;
      result.capacity = capacity;
      return result;
    });
}

void MirroredRingMemory::reset()
{
  if (!base)
    return;

  // Same availability that selected the create path selects the teardown path.
  get_placeholder_api()
    .transform([&](const PlaceholderApi &api) {
      api.unmapViewOfFile2(GetCurrentProcess(), base, MEM_PRESERVE_PLACEHOLDER);
      api.unmapViewOfFile2(GetCurrentProcess(), base + capacity, MEM_PRESERVE_PLACEHOLDER);
      VirtualFree(base, 0, MEM_RELEASE);
      VirtualFree(base + capacity, 0, MEM_RELEASE); //-V774
      return true;
    })
    .or_else([&]() -> eastl::optional<bool> {
      UnmapViewOfFile(base);
      UnmapViewOfFile(base + capacity);
      return eastl::nullopt;
    });

  base = nullptr;
  capacity = 0;
}
} // namespace drv3d_dx12

#endif // _TARGET_XBOX
