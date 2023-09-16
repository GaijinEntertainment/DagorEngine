#include "device.h"

using namespace drv3d_dx12;

bool drv3d_dx12::is_hdr_available(const ComPtr<IDXGIOutput> &output)
{
  if (!output)
    return false;

  ComPtr<IDXGIOutput6> output6;
  if (FAILED(output.As(&output6)))
    return false;

  DXGI_OUTPUT_DESC1 desc1;
  if (FAILED(output6->GetDesc1(&desc1)))
    return false;

  return desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
}

bool drv3d_dx12::change_hdr(bool force_enable, const ComPtr<IDXGIOutput> &output)
{
  static_assert(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020 != 0, "The aggreagete initialized "
                                                                 "desc1's ColorSpace check will "
                                                                 "give false result!");
  DXGI_OUTPUT_DESC1 desc1{};

  if (output)
  {
    ComPtr<IDXGIOutput6> output6;
    if (SUCCEEDED(output.As(&output6)))
    {
      if (FAILED(output6->GetDesc1(&desc1)))
        desc1 = {};
    }
  }

  bool is_hdr_enabled = force_enable ? true : desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
  hdr_changed(is_hdr_enabled, desc1.MinLuminance, desc1.MaxLuminance, desc1.MaxFullFrameLuminance);
  return is_hdr_enabled;
}

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

namespace
{
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
} // namespace

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
    eastl::vector<char> nameBuf;
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

  LSTATUS open(HKEY key, LPCSTR sub_key, DWORD options, REGSAM sam)
  {
    DEBUG_REG("DX12: Opening Registry key %p <%s> %u %u", key, sub_key, options, sam);
    HKEY result;
    auto status = RegOpenKeyExA(key, sub_key, options, sam, &result);
    if (ERROR_SUCCESS == status)
    {
      DEBUG_REG("DX12: ...success %p", result);
      ptr.reset(result);
    }
    else
    {
      DEBUG_REG("DX12: ...failed");
    }
    return status;
  }

  RegistryKey openSubKey(LPCSTR sub_key, DWORD options, REGSAM sam) const
  {
    RegistryKey newKey;
    newKey.open(ptr.get(), sub_key, options, sam);
    return newKey;
  }

  eastl::tuple<DWORD, DWORD> querySubKeyCountAndMaxLength() const
  {
    DWORD count = 0;
    DWORD length = 0;

    RegQueryInfoKeyA(ptr.get(), nullptr, nullptr, nullptr, &count, &length, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    DEBUG_REG("DX12: querySubKeyCountAndMaxLength of %p -> %u, %u", ptr.get(), count, length);

    return eastl::make_tuple(count, length);
  }

  eastl::tuple<DWORD, DWORD, DWORD> queryValueCountMaxLengthAndSize() const
  {
    DWORD count = 0;
    DWORD length = 0;
    DWORD value = 0;

    RegQueryInfoKeyA(ptr.get(), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &count, &length, &value, nullptr, nullptr);
    DEBUG_REG("DX12: queryValueCountMaxLengthAndSize of %p -> %u, %u, %u", ptr.get(), count, length, value);

    return eastl::make_tuple(count, length, value);
  }

  eastl::string_view enumKeyName(DWORD index, eastl::span<char> buffer)
  {
    auto maxLength = static_cast<DWORD>(buffer.size());
    if (ERROR_SUCCESS == RegEnumKeyExA(ptr.get(), index, buffer.data(), &maxLength, nullptr, nullptr, nullptr, nullptr))
    {
      DEBUG_REG("DX12: enumKeyName of %p at %u -> <%s>", ptr.get(), index, buffer.data());
      return {buffer.data(), maxLength};
    }
    else
    {
      DEBUG_REG("DX12: enumKeyName of %p at %u failed", ptr.get(), index);
      return {};
    }
  }

  eastl::string_view enumValueName(DWORD index, eastl::span<char> buffer)
  {
    auto maxNameLength = static_cast<DWORD>(buffer.size());
    if (ERROR_SUCCESS == RegEnumValueA(ptr.get(), index, buffer.data(), &maxNameLength, nullptr, nullptr, nullptr, nullptr))
    {
      DEBUG_REG("DX12: enumValueName of %p at %u -> <%s>", ptr.get(), index, buffer.data());
      return {buffer.data(), maxNameLength};
    }
    else
    {
      DEBUG_REG("DX12: enumValueName of %p at %u failed", ptr.get(), index);
      return {};
    }
  }

  eastl::tuple<eastl::string_view, eastl::span<char>> enumValueNameAndValue(DWORD index, eastl::span<char> name_buffer,
    eastl::span<char> data_buffer)
  {
    auto maxNameLength = static_cast<DWORD>(name_buffer.size());
    auto maxDataLength = static_cast<DWORD>(data_buffer.size());
    if (ERROR_SUCCESS == RegEnumValueA(ptr.get(), index, name_buffer.data(), &maxNameLength, nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(data_buffer.data()), &maxDataLength))
    {
      DEBUG_REG("DX12: enumValueNameAndValue of %p at %u -> <%s> %u", ptr.get(), index, name_buffer.data(), maxDataLength);
      return eastl::make_tuple(eastl::string_view{name_buffer.data(), maxNameLength}, data_buffer.subspan(0, maxDataLength));
    }
    else
    {
      DEBUG_REG("DX12: enumValueNameAndValue of %p at %u failed", ptr.get(), index);
      return {};
    }
  }

  explicit operator bool() const { return static_cast<bool>(ptr); }
};

bool is_this_application_config_name(eastl::string_view app_path, eastl::span<char> config)
{
  eastl::string_view configPath{config.data(), strlen(config.data())};
  debug("DX12: Checking if <%s> is covering <%s>...", configPath.data(), app_path.data());
  if (configPath.length() > app_path.length())
  {
    debug("DX12: Config path is too long...");
    return false;
  }
  if (configPath == app_path)
  {
    debug("DX12: Exact match...");
    return true;
  }
  if (configPath.back() == '\\')
  {
    debug("DX12: Config path is a path, looking if its a base of application path...");
    if (configPath == app_path.substr(0, configPath.length()))
    {
      // TODO see if this is accurate or not
      debug("DX12: Application path did match...");
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
    debug("DX12: Unable to open current user registry path <%s>", debug_settings_path);
    return result;
  }

  auto [subKeyCount, maxSubKeyLength] = baseKey.querySubKeyCountAndMaxLength();

  if (!subKeyCount)
  {
    return result;
  }

  eastl::vector<char> nameBuf;
  nameBuf.resize(maxSubKeyLength + 1, '\0');

  eastl::vector<char> valueBuf;

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