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
  // Parses "PCI\VEN_XXXX&DEV_XXXX&..." — returns false if the format doesn't match.
  //         ^       ^    ^   ^
  //         0       8   12   17
  auto parseHardwareId = [](const wchar_t *hwid, uint16_t &vendor_id, uint16_t &device_id) {
    auto parseHex4 = [](const wchar_t *s, uint16_t &out) {
      uint16_t value = 0;
      for (int i = 0; i < 4; ++i)
      {
        wchar_t c = s[i];
        uint16_t digit;
        if (c >= L'0' && c <= L'9')
          digit = static_cast<uint16_t>(c - L'0');
        else if (c >= L'A' && c <= L'F')
          digit = static_cast<uint16_t>(c - L'A' + 10);
        else if (c >= L'a' && c <= L'f')
          digit = static_cast<uint16_t>(c - L'a' + 10);
        else
          return false;
        value = static_cast<uint16_t>((value << 4) | digit);
      }
      out = value;
      return true;
    };

    if (wcsncmp(hwid, L"PCI\\VEN_", 8) != 0)
      return false;
    if (!parseHex4(hwid + 8, vendor_id))
      return false;
    if (wcsncmp(hwid + 12, L"&DEV_", 5) != 0)
      return false;
    if (!parseHex4(hwid + 17, device_id))
      return false;
    return true;
  };

  HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
  if (devInfo == INVALID_HANDLE_VALUE)
    return {};

  FINALLY([=] { SetupDiDestroyDeviceInfoList(devInfo); });

  eastl::fixed_vector<eastl::pair<uint16_t, uint16_t>, 8> gdis;

  dag::Vector<wchar_t> hwId;
  SP_DEVINFO_DATA devData{.cbSize = sizeof(SP_DEVINFO_DATA)};
  for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); ++i)
  {
    // Shortest valid hardware ID is "PCI\VEN_XXXX&DEV_XXXX\0" = 22 wchars.
    constexpr DWORD minBytes = 22 * sizeof(wchar_t);

    DWORD bufSize = 0;
    SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID, nullptr, nullptr, 0, &bufSize);
    if (bufSize < minBytes)
      continue;

    hwId.resize_noinit(bufSize / sizeof(wchar_t));
    if (!SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_HARDWAREID, nullptr, reinterpret_cast<PBYTE>(hwId.data()), bufSize,
          nullptr))
      continue;

    uint16_t vendorId, deviceId;
    if (!parseHardwareId(hwId.data(), vendorId, deviceId))
      continue;

    // Skip Microsoft Basic Render Driver.
    if (vendorId == 0x1414 && deviceId == 0x008C)
      continue;

    gdis.push_back({vendorId, deviceId});
  }

  return gdis;
}
