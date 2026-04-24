// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#include <Windows.h>

#include <EASTL/optional.h>

#include <debug/dag_logSys.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_versionQuery.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>


namespace systeminfo
{
namespace
{
#if _TARGET_ARCH_X86_64
static constexpr auto vcRedistInstallerName = "redist/vc_redist.x64.exe";
static constexpr auto vcRedistRegKey = L"SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x64\\";
#elif _TARGET_ARCH_ARM64
static constexpr auto vcRedistInstallerName = "redist/vc_redist.arm64.exe";
static constexpr auto vcRedistRegKey = L"SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\ARM64\\";
#elif _TARGET_ARCH_X86
static constexpr auto vcRedistInstallerName = "redist/vc_redist.x86.exe";
static constexpr auto vcRedistRegKey = L"SOFTWARE\\Microsoft\\VisualStudio\\14.0\\VC\\Runtimes\\x86\\";
#else
static constexpr auto vcRedistInstallerName = "";
static constexpr auto vcRedistRegKey = L"";
#endif

static constexpr auto vcRedistMainDll = L"MSVCP140.dll";

static constexpr auto vcRedistUpdateArticleUrl = "support.gaijin.net/hc/articles/30371868515858";
static constexpr auto vcRedistInstallerUrl =
  "learn.microsoft.com/cpp/windows/latest-supported-vc-redist#latest-supported-redistributable-version";


eastl::optional<LibraryVersion> get_current_vc_redist_ver()
{
  // There are cases when the latest VC++ 2015-2025 redistributable is installed and in the registry the version is correct,
  // but the actual DLLs are older. So instead of relying on registry, we check the version of the actual DLL.
  // Fixing EEX-10390
#if 0
  HKEY hKey;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, vcRedistRegKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
  {
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, vcRedistRegKey, 0, KEY_READ | KEY_WOW64_32KEY, &hKey) != ERROR_SUCCESS)
      return eastl::nullopt;
  }

  DWORD major = 0, minor = 0, bld = 0, rev = 0;
  DWORD size = sizeof(DWORD);

  RegQueryValueExW(hKey, L"Major", nullptr, nullptr, reinterpret_cast<LPBYTE>(&major), &size);
  RegQueryValueExW(hKey, L"Minor", nullptr, nullptr, reinterpret_cast<LPBYTE>(&minor), &size);
  RegQueryValueExW(hKey, L"Bld", nullptr, nullptr, reinterpret_cast<LPBYTE>(&bld), &size);
  RegQueryValueExW(hKey, L"Revision", nullptr, nullptr, reinterpret_cast<LPBYTE>(&rev), &size);

  RegCloseKey(hKey);

  return LibraryVersion{
    .major = static_cast<uint16_t>(major),
    .minor = static_cast<uint16_t>(minor),
    .build = static_cast<uint16_t>(bld),
    .revision = static_cast<uint16_t>(rev),
  };
#else
  return get_library_version(vcRedistMainDll);
#endif
}
} // namespace

void check_vc_redist(const DataBlock *cfg)
{
  if (!cfg || ::dgs_execute_quiet)
    return;

  LibraryVersion vcRedistVer{};
  sscanf(cfg->getStr("minVcRedistVer", "0.0.0.0"), " %hu . %hu . %hu . %hu", &vcRedistVer.major, &vcRedistVer.minor,
    &vcRedistVer.build, &vcRedistVer.revision);

  if (vcRedistVer == LibraryVersion{})
    return;

  auto currentVcRedistVer = get_current_vc_redist_ver();
  if (currentVcRedistVer && *currentVcRedistVer >= vcRedistVer)
    return;

  const bool hasVcRedistInstaller = dd_file_exists(vcRedistInstallerName);

  int result = [&] {
    ScopeSetWatchdogTimeout _wd(WATCHDOG_DISABLE);

    const String msg1(1024,                                                                              //
      get_localized_text(hasVcRedistInstaller ? "engine/vcredist_install" : "engine/outdated_vcredist"), //
      currentVcRedistVer->toString().c_str(),                                                            //
      vcRedistVer.toString().c_str());                                                                   //
    const String msg(1024, "%s\n<a href=\"https://%s\">%s</a>", msg1.c_str(), vcRedistUpdateArticleUrl, vcRedistUpdateArticleUrl);

    return os_message_box(msg.c_str(), get_localized_text("engine/outdated_vcredist_hdr"),
      (hasVcRedistInstaller ? GUI_MB_YES_NO_CANCEL : GUI_MB_OK_CANCEL) | GUI_MB_ICON_WARNING);
  }();

  if (hasVcRedistInstaller)
  {
    if (result == GUI_MB_BUTTON_2) // No
      return;

    if (result == GUI_MB_BUTTON_1) // Yes
    {
      STARTUPINFO si{.cb = sizeof(si)};
      PROCESS_INFORMATION pi{};
      CreateProcessA(vcRedistInstallerName, nullptr, nullptr, nullptr, TRUE, 0, nullptr, nullptr, &si, &pi);
    }

    flush_debug_file();
    terminate_process(1);
  }
  else if (result == GUI_MB_BUTTON_2) // Cancel
  {
    flush_debug_file();
    terminate_process(1);
  }
}
} // namespace systeminfo
#else
class DataBlock;
namespace systeminfo
{
void check_vc_redist(const DataBlock *) {}
} // namespace systeminfo
#endif
