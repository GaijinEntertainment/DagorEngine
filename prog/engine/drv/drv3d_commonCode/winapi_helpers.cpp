// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "winapi_helpers.h"

#include <dag/dag_vector.h>
#include <util/dag_finally.h>

#include <setupapi.h>
#include <stdio.h>
#include <devguid.h>
#pragma comment(lib, "setupapi.lib")


#if DEBUG_REPORT_REGISTRY_INSPECTION
#define DEBUG_REG debug
#else
#define DEBUG_REG(...)
#endif

LSTATUS RegistryKey::open(HKEY key, LPCSTR sub_key, DWORD options, REGSAM sam)
{
  DEBUG_REG("Opening Registry key %p <%s> %u %u", key, sub_key, options, sam);
  HKEY result;
  auto status = RegOpenKeyExA(key, sub_key, options, sam, &result);
  if (ERROR_SUCCESS == status)
  {
    DEBUG_REG("...success %p", result);
    ptr.reset(result);
  }
  else
  {
    DEBUG_REG("...failed");
  }
  return status;
}

eastl::tuple<DWORD, DWORD> RegistryKey::querySubKeyCountAndMaxLength() const
{
  DWORD count = 0;
  DWORD length = 0;

  RegQueryInfoKeyA(ptr.get(), nullptr, nullptr, nullptr, &count, &length, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
  DEBUG_REG("querySubKeyCountAndMaxLength of %p -> %u, %u", ptr.get(), count, length);

  return eastl::make_tuple(count, length);
}

eastl::tuple<DWORD, DWORD, DWORD> RegistryKey::queryValueCountMaxLengthAndSize() const
{
  DWORD count = 0;
  DWORD length = 0;
  DWORD value = 0;

  RegQueryInfoKeyA(ptr.get(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &count, &length, &value, nullptr, nullptr);
  DEBUG_REG("queryValueCountMaxLengthAndSize of %p -> %u, %u, %u", ptr.get(), count, length, value);

  return eastl::make_tuple(count, length, value);
}

eastl::string_view RegistryKey::enumKeyName(DWORD index, eastl::span<char> buffer) const
{
  auto maxLength = static_cast<DWORD>(buffer.size());
  if (ERROR_SUCCESS == RegEnumKeyExA(ptr.get(), index, buffer.data(), &maxLength, nullptr, nullptr, nullptr, nullptr))
  {
    DEBUG_REG("enumKeyName of %p at %u -> <%s>", ptr.get(), index, buffer.data());
    return {buffer.data(), maxLength};
  }
  else
  {
    DEBUG_REG("enumKeyName of %p at %u failed", ptr.get(), index);
    return {};
  }
}

eastl::string_view RegistryKey::enumValueName(DWORD index, eastl::span<char> buffer) const
{
  auto maxNameLength = static_cast<DWORD>(buffer.size());
  if (ERROR_SUCCESS == RegEnumValueA(ptr.get(), index, buffer.data(), &maxNameLength, nullptr, nullptr, nullptr, nullptr))
  {
    DEBUG_REG("enumValueName of %p at %u -> <%s>", ptr.get(), index, buffer.data());
    return {buffer.data(), maxNameLength};
  }
  else
  {
    DEBUG_REG("enumValueName of %p at %u failed", ptr.get(), index);
    return {};
  }
}

eastl::tuple<eastl::string_view, eastl::span<char>> RegistryKey::enumValueNameAndValue(DWORD index, eastl::span<char> name_buffer,
  eastl::span<char> data_buffer) const
{
  auto maxNameLength = static_cast<DWORD>(name_buffer.size());
  auto maxDataLength = static_cast<DWORD>(data_buffer.size());
  if (ERROR_SUCCESS == RegEnumValueA(ptr.get(), index, name_buffer.data(), &maxNameLength, nullptr, nullptr,
                         reinterpret_cast<LPBYTE>(data_buffer.data()), &maxDataLength))
  {
    DEBUG_REG("enumValueNameAndValue of %p at %u -> <%s> %u", ptr.get(), index, name_buffer.data(), maxDataLength);
    return eastl::make_tuple(eastl::string_view{name_buffer.data(), maxNameLength}, data_buffer.subspan(0, maxDataLength));
  }
  else
  {
    DEBUG_REG("enumValueNameAndValue of %p at %u failed", ptr.get(), index);
    return {};
  }
}

eastl::fixed_vector<eastl::pair<uint16_t, uint16_t>, 8> getGDIs()
{
  HDEVINFO devInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
  if (devInfo == INVALID_HANDLE_VALUE)
    return {};

  FINALLY([=] { SetupDiDestroyDeviceInfoList(devInfo); });

  eastl::fixed_vector<eastl::pair<uint16_t, uint16_t>, 8> gdis;

  dag::Vector<wchar_t> hwId;
  SP_DEVINFO_DATA devData{.cbSize = sizeof(SP_DEVINFO_DATA)};
  for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); ++i)
  {
    DWORD bufSize = 0;
    SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID, nullptr, nullptr, 0, &bufSize);
    hwId.resize_noinit(bufSize / sizeof(wchar_t));
    SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID, nullptr, reinterpret_cast<PBYTE>(hwId.data()), bufSize,
      nullptr);

    uint16_t vendorId, deviceId;
    swscanf_s(hwId.data(), L"PCI\\VEN_%hX&DEV_%hX", &vendorId, &deviceId);

    gdis.push_back({vendorId, deviceId});
  }

  return gdis;
}
