// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_versionQuery.h>

#include <EASTL/unique_ptr.h>

#if _TARGET_PC_WIN
#include <Windows.h>


eastl::optional<LibraryVersion> get_library_version(eastl::string_view library)
{
  DWORD versionHandle;
  auto versionSize = GetFileVersionInfoSizeA(library.data(), &versionHandle);
  if (!versionSize)
  {
    return eastl::nullopt;
  }

  auto versionData = eastl::make_unique<uint8_t[]>(versionSize);
  if (!GetFileVersionInfoA(library.data(), versionHandle, versionSize, versionData.get()))
  {
    return eastl::nullopt;
  }

  VS_FIXEDFILEINFO *fileInfo;
  UINT fileVersionInfoSize;
  if (!VerQueryValueA(versionData.get(), "\\", reinterpret_cast<LPVOID *>(&fileInfo), &fileVersionInfoSize))
  {
    return eastl::nullopt;
  }

  if (!fileInfo)
  {
    return eastl::nullopt;
  }

  return LibraryVersion{
    .major = HIWORD(fileInfo->dwFileVersionMS),
    .minor = LOWORD(fileInfo->dwFileVersionMS),
    .build = HIWORD(fileInfo->dwFileVersionLS),
    .revision = LOWORD(fileInfo->dwFileVersionLS),
  };
}

eastl::optional<LibraryVersion> get_library_version(eastl::wstring_view library)
{
  DWORD versionHandle;
  auto versionSize = GetFileVersionInfoSizeW(library.data(), &versionHandle);
  if (!versionSize)
  {
    return eastl::nullopt;
  }

  auto versionData = eastl::make_unique<uint8_t[]>(versionSize);
  if (!GetFileVersionInfoW(library.data(), versionHandle, versionSize, versionData.get()))
  {
    return eastl::nullopt;
  }

  VS_FIXEDFILEINFO *fileInfo;
  UINT fileVersionInfoSize;
  if (!VerQueryValueW(versionData.get(), L"\\", reinterpret_cast<LPVOID *>(&fileInfo), &fileVersionInfoSize))
  {
    return eastl::nullopt;
  }

  if (!fileInfo)
  {
    return eastl::nullopt;
  }

  return LibraryVersion{
    .major = HIWORD(fileInfo->dwFileVersionMS),
    .minor = LOWORD(fileInfo->dwFileVersionMS),
    .build = HIWORD(fileInfo->dwFileVersionLS),
    .revision = LOWORD(fileInfo->dwFileVersionLS),
  };
}

eastl::optional<LibraryVersion> get_library_version(HMODULE module)
{
  wchar_t dllName[MAX_PATH];
  if (GetModuleFileNameW(module, dllName, MAX_PATH))
  {
    return get_library_version(dllName);
  }

  return eastl::nullopt;
}

#else

eastl::optional<LibraryVersion> get_library_version(eastl::string_view) { return eastl::nullopt; }

eastl::optional<LibraryVersion> get_library_version(eastl::wstring_view) { return eastl::nullopt; }

eastl::optional<LibraryVersion> get_library_version(HMODULE) { return eastl::nullopt; }

#endif

// When can't decide runtime the used driver(vulkan or dx11/dx12)
// - on windows the follow the
// https://www.nvidia.com/en-gb/drivers/drivers-faq/
// because more likely the driver ins't vulkan.
// - on non windows system follow the vulkan nvidia driver encoding scheme
// https://www.reddit.com/r/vulkan/comments/fmift4/how_to_decode_driverversion_field_of
static TwoComponentVersion default_to_nvidia_version(DriverVersion version)
{
#if _TARGET_PC_WIN
  return {
    .major = uint16_t((version.minor % 10) * 100 + version.build / 100),
    .minor = uint16_t(version.build % 100),
  };
#else
  return {
    .major = version.product,
    .minor = version.major,
  };
#endif
}

TwoComponentVersion (*to_nvidia_version)(DriverVersion version) = &default_to_nvidia_version;

#define EXPORT_PULL dll_pull_osapiwrappers_versionQuery
#include <supp/exportPull.h>
