// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "device.h"
#include <startup/dag_globalSettings.h>

using namespace drv3d_dx12;

eastl::optional<HDRCapabilities> drv3d_dx12::get_hdr_caps(const ComPtr<IDXGIOutput> &output)
{
  ComPtr<IDXGIOutput6> output6;
  DXGI_OUTPUT_DESC1 desc1;
  if (output && SUCCEEDED(output.As(&output6)) && SUCCEEDED(output6->GetDesc1(&desc1)) &&
      desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
  {
    return eastl::make_optional<HDRCapabilities>(desc1.MinLuminance, desc1.MaxLuminance, desc1.MaxFullFrameLuminance);
  }
  return eastl::nullopt;
}

bool drv3d_dx12::is_hdr_available(const ComPtr<IDXGIOutput> &output) { return get_hdr_caps(output).has_value(); }

#if !_TARGET_64BIT
bool drv3d_dx12::is_wow64()
{
  static BOOL (*is_wow64_process)(HANDLE, PBOOL) = nullptr;
  BOOL isWow64 = FALSE;
  if (!is_wow64_process)
  {
    reinterpret_cast<FARPROC &>(is_wow64_process) = GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process");
  }

  if (is_wow64_process)
  {
    isWow64 = FALSE;
    is_wow64_process(GetCurrentProcess(), &isWow64);
  }
  return FALSE != isWow64;
}
#endif

#if DEBUG_REPORT_REGISTRY_INSPECTION
#define DEBUG_REG debug
#else
#define DEBUG_REG(...)
#endif

#if !_TARGET_64BIT
// Optimizer damages the stack on RegQueryInfoKeyA calls when optimized in 32 bit builds
#pragma optimize("", off)
#endif
DriverVersion drv3d_dx12::get_driver_version_from_registry(LUID adapter_luid)
{
  // LUID of 0 is invalid
  G_ASSERTF_RETURN((0 != adapter_luid.HighPart) || (0 != adapter_luid.LowPart), {}, "Device LUID can not be 0");

  DEBUG_REG("DX12: Looking for DirectX driver version for adapter with LUID of %08X%08X in "
            "registry...",
    adapter_luid.HighPart, adapter_luid.LowPart);
  REGSAM options = KEY_READ;
#if !_TARGET_64BIT
  if (is_wow64())
  {
    options |= KEY_WOW64_64KEY;
    DEBUG_REG("DX12: 32 Bit on WoW64 detected, requesting 64 Bit keys");
  }
#endif
  DEBUG_REG("DX12: Trying to open DirectX root key...");
  RegistryKeyPtr directXKey;
  HKEY key;
  if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\DirectX", 0, options, &key))
  {
    DEBUG_REG("DX12: Failed...");
    logwarn("DX12: Unable to open DirectX driver info in registry");
    return {};
  }
  directXKey.reset(key);

  DWORD subKeyCount = 0;
  DWORD maxSubKeyLength = 0;
  RegQueryInfoKeyA(directXKey.get(), nullptr, nullptr, nullptr, &subKeyCount, &maxSubKeyLength, nullptr, nullptr, nullptr, nullptr,
    nullptr, nullptr);
  DEBUG_REG("DX12: Found %u subkeys", subKeyCount);
  if (subKeyCount)
  {
    dag::Vector<char> nameBuf;
    nameBuf.resize(maxSubKeyLength + 1, '\0');

    for (DWORD k = 0; k < subKeyCount; ++k)
    {
      DEBUG_REG("DX12: Inspecting subkey %u...", k);

      DWORD maxNameLength = static_cast<DWORD>(nameBuf.size());
      if (ERROR_SUCCESS == RegEnumKeyEx(directXKey.get(), k, nameBuf.data(), &maxNameLength, nullptr, nullptr, nullptr, nullptr))
      {
        RegistryKeyPtr driverKey;
        DEBUG_REG("DX12: Subkey name is <%s>", nameBuf.data());
        if (ERROR_SUCCESS == RegOpenKeyExA(directXKey.get(), nameBuf.data(), 0, options, &key))
        {
          driverKey.reset(key);
          DWORD valueCount = 0;
          DWORD maxValueLength = 0;
          RegQueryInfoKeyA(driverKey.get(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &valueCount, &maxValueLength,
            nullptr, nullptr, nullptr);
          nameBuf.resize(max<size_t>(nameBuf.size(), maxValueLength + 1));
          LARGE_INTEGER adapterLUID{};
          uint64_t driverVersion = 0;
          DEBUG_REG("DX12: Found %u values", valueCount);
          for (DWORD v = 0; v < valueCount && (0 == adapterLUID.QuadPart || 0 == driverVersion); ++v)
          {
            DEBUG_REG("DX12: Inspecting value %u...", v);
            maxNameLength = static_cast<DWORD>(nameBuf.size());
            if (ERROR_SUCCESS == RegEnumValueA(driverKey.get(), v, nameBuf.data(), &maxNameLength, nullptr, nullptr, nullptr, nullptr))
            {
              DEBUG_REG("DX12: value name is <%s>", nameBuf.data());
              LPBYTE target = nullptr;
              DWORD targetSize = sizeof(uint64_t);
              if (0 == strcmp(nameBuf.data(), "AdapterLuid"))
              {
                target = reinterpret_cast<LPBYTE>(&adapterLUID.QuadPart);
              }
              else if (0 == strcmp(nameBuf.data(), "DriverVersion"))
              {
                target = reinterpret_cast<LPBYTE>(&driverVersion);
              }
              else
              {
                continue;
              }
              RegEnumValueA(driverKey.get(), v, nameBuf.data(), &maxNameLength, nullptr, nullptr, target, &targetSize);
              DEBUG_REG("DX12: value was %I64u", *reinterpret_cast<uint64_t *>(target));
              // if we just read LUID and it was 0, then the driver is installed but is not used for
              // any device, so we can stop
              if (target == reinterpret_cast<LPBYTE>(&adapterLUID.QuadPart) && 0 == adapterLUID.QuadPart)
              {
                break;
              }
            }
          }

          if (adapterLUID.LowPart == adapter_luid.LowPart && adapterLUID.HighPart == adapter_luid.HighPart)
          {
            DEBUG_REG("DX12: Found entry with matching LUID, stopping");
            return DriverVersion::fromRegistryValue(driverVersion);
          }
        }
      }
    }
  }
  logwarn("DX12: Unable to locate DirectX driver entry in registry for LUID %08X%08X", adapter_luid.HighPart, adapter_luid.LowPart);
  return {};
}

namespace
{
bool is_this_application_config_name(eastl::string_view app_path, eastl::span<char> config)
{
  eastl::string_view configPath{config.data(), strlen(config.data())};
  logdbg("DX12: Checking if <%s> is covering <%s>...", configPath.data(), app_path.data());
  if (configPath.length() > app_path.length())
  {
    logdbg("DX12: Config path is too long...");
    return false;
  }
  if (configPath == app_path)
  {
    logdbg("DX12: Exact match...");
    return true;
  }
  if (configPath.back() == '\\')
  {
    logdbg("DX12: Config path is a path, looking if its a base of application path...");
    if (configPath == app_path.substr(0, configPath.length()))
    {
      // TODO see if this is accurate or not
      logdbg("DX12: Application path did match...");
      return true;
    }
  }
  return false;
}

// key values have the following format
// <key>=<value>
// pairs are separated by ;
eastl::string_view find_key_value_in_behaviors(eastl::string_view config, eastl::string_view key)
{
  auto pos = config.find(key);
  if (eastl::string_view::npos == pos)
  {
    return {};
  }
  auto valueStart = config.substr(pos + key.length() + 1);
  return {begin(valueStart), static_cast<size_t>(eastl::find(begin(valueStart), end(valueStart), ';') - begin(valueStart))};
}

eastl::tuple<eastl::optional<D3D12_DRED_ENABLEMENT>, eastl::optional<D3D12_DRED_ENABLEMENT>> parse_behavior(eastl::span<char> config)
{
  eastl::string_view configView{config.data(), config.size()};
  DEBUG_REG("DX12: parse_behavior <%s>...", config.data());
  eastl::optional<D3D12_DRED_ENABLEMENT> breacrumbEnablement;
  eastl::optional<D3D12_DRED_ENABLEMENT> pagefaultEnablement;
  auto breadcumbsValue = find_key_value_in_behaviors(configView, "DredAutoBreadcrumbs");
  if (!breadcumbsValue.empty())
  {
    // Values stored in config match enum indices
    breacrumbEnablement = static_cast<D3D12_DRED_ENABLEMENT>(breadcumbsValue[0] - '0');
    DEBUG_REG("DX12: ...found key <%s> value is %u...", "DredAutoBreadcrumbs", *breacrumbEnablement);
  }
  else
  {
    DEBUG_REG("DX12: ...missing key <%s>...", "DredAutoBreadcrumbs");
  }
  auto pagefaultValue = find_key_value_in_behaviors(configView, "DredPageFault");
  if (!pagefaultValue.empty())
  {
    // Values stored in config match enum indices
    pagefaultEnablement = static_cast<D3D12_DRED_ENABLEMENT>(pagefaultValue[0] - '0');
    DEBUG_REG("DX12: ...found key <%s> value is %u...", "DredPageFault", *pagefaultEnablement);
  }
  else
  {
    DEBUG_REG("DX12: ...missing key <%s>...", "DredPageFault");
  }
  return eastl::make_tuple(breacrumbEnablement, pagefaultEnablement);
}
} // namespace

D3D12_DRED_ENABLEMENT drv3d_dx12::get_application_DRED_enablement_from_registry()
{
  static const char debug_settings_path[] = "Software\\Microsoft\\Direct3D\\Direct3D12";
  static const char application_base_path[] = "Application";

  // TODO fails when app is located in a path with a very long name
  char thisAppPath[MAX_PATH]{};
  GetModuleFileNameA(nullptr, thisAppPath, sizeof(thisAppPath));
  eastl::string_view thisAppPathView{thisAppPath, strlen(thisAppPath)};

  D3D12_DRED_ENABLEMENT result = D3D12_DRED_ENABLEMENT_SYSTEM_CONTROLLED;

  REGSAM options = KEY_READ;
  // Open Computer\HKEY_CURRENT_USER\Software\Microsoft\Direct3D\Direct3D12
  RegistryKey baseKey{HKEY_CURRENT_USER, debug_settings_path, 0, options};
  if (!baseKey)
  {
    DEBUG_REG("DX12: Failed...");
    logdbg("DX12: Unable to open current user registry path <%s>", debug_settings_path);
    return result;
  }

  auto [subKeyCount, maxSubKeyLength] = baseKey.querySubKeyCountAndMaxLength();

  if (!subKeyCount)
  {
    return result;
  }

  dag::Vector<char> nameBuf;
  nameBuf.resize(maxSubKeyLength + 1, '\0');

  dag::Vector<char> valueBuf;

  for (DWORD k = 0; k < subKeyCount; ++k)
  {
    auto keyName = baseKey.enumKeyName(k, {nameBuf.data(), nameBuf.size()});
    if (keyName.empty())
    {
      continue;
    }

    // We only want to inspect Computer\HKEY_CURRENT_USER\Software\Microsoft\Direct3D\Direct3D12\Application<N>
    if (0 != strncmp(keyName.data(), application_base_path, sizeof(application_base_path) - 1))
    {
      continue;
    }
    // TODO do we want to inspect Computer\HKEY_CURRENT_USER\Software\Microsoft\Direct3D\Direct3D12\ControlPannel to see global config?

    auto appKey = baseKey.openSubKey(keyName.data(), 0, options);
    if (!appKey)
    {
      continue;
    }

    auto [valueCount, maxValueLenth, valueSize] = appKey.queryValueCountMaxLengthAndSize();

    nameBuf.resize(max(maxSubKeyLength + 1, maxValueLenth + 1), '\0');
    valueBuf.resize(max<size_t>(valueSize + 1, valueBuf.size()), '\0');

    bool nameWasFound = false;
    bool configuresApplication = false;
    bool behaviorsFound = false;
    D3D12_DRED_ENABLEMENT pagefaultConfig = D3D12_DRED_ENABLEMENT_SYSTEM_CONTROLLED;
    D3D12_DRED_ENABLEMENT breadcrumbConfig = D3D12_DRED_ENABLEMENT_SYSTEM_CONTROLLED;
    for (DWORD v = 0; v < valueCount && (!nameWasFound || !behaviorsFound); ++v)
    {
      auto [valueName, value] = appKey.enumValueNameAndValue(v, {nameBuf.data(), nameBuf.size()}, {valueBuf.data(), valueBuf.size()});

      if (0 == valueName.compare("Name"))
      {
        // Name contains the application name or base path for a application that is registered to be debugged
        configuresApplication = is_this_application_config_name(thisAppPathView, value);
        nameWasFound = true;
      }
      else if (0 == valueName.compare("D3DBehaviors"))
      {
        // Behaviors is a string with key value pairs, configuring the behavior of the debug layer and other dx runtime systems
        auto [breadcumbBehavior, pagefaultBehavior] = parse_behavior(value);
        breadcrumbConfig = breadcumbBehavior.value_or(D3D12_DRED_ENABLEMENT_SYSTEM_CONTROLLED);
        pagefaultConfig = pagefaultBehavior.value_or(D3D12_DRED_ENABLEMENT_SYSTEM_CONTROLLED);
        behaviorsFound = true;
      }
    }

    if (configuresApplication)
    {
      result = breadcrumbConfig;
      break;
    }
  }
  return result;
}
#if !_TARGET_64BIT
#pragma optimize("", on)
#endif

DXGI_GPU_PREFERENCE drv3d_dx12::get_gpu_preference_from_registry()
{
  static const char directx_path[] = "Software\\Microsoft\\DirectX\\UserGpuPreferences";
  static const char gpu_preference[] = "GpuPreference";

  eastl::string_view exeName(dgs_argv[0]);

  REGSAM options = KEY_READ;
  RegistryKey baseKey{HKEY_CURRENT_USER, directx_path, 0, options};
  if (!baseKey)
  {
    DEBUG_REG("DX12: Failed...");
    logdbg("DX12: Unable to open current user registry path <%s>", directx_path);
    return DXGI_GPU_PREFERENCE_UNSPECIFIED;
  }

  auto [valueCount, maxValueLenth, valueSize] = baseKey.queryValueCountMaxLengthAndSize();
  if (!valueCount)
  {
    return DXGI_GPU_PREFERENCE_UNSPECIFIED;
  }

  dag::Vector<char> nameBuf(maxValueLenth + 1, '\0'), valueBuf(valueSize + 1, '\0');
  for (DWORD v = 0; v < valueCount; ++v)
  {
    auto [valueName, value] = baseKey.enumValueNameAndValue(v, nameBuf, valueBuf);
    if (valueName == exeName)
    {
      eastl::string_view stringValue(value.data(), value.size());
      if (stringValue.starts_with(gpu_preference))
      {
        switch (value[sizeof(gpu_preference)])
        {
          case '1': return DXGI_GPU_PREFERENCE_MINIMUM_POWER;
          case '2': return DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE;
        }
      }
    }
  }

  return DXGI_GPU_PREFERENCE_UNSPECIFIED;
}
