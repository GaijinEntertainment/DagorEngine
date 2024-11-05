// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "winapi_helpers.h"


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

eastl::string_view RegistryKey::enumKeyName(DWORD index, eastl::span<char> buffer)
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

eastl::string_view RegistryKey::enumValueName(DWORD index, eastl::span<char> buffer)
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
  eastl::span<char> data_buffer)
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
