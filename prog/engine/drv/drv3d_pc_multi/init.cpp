// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_info.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_res.h>
#include <3d/dag_ICrashFallback.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_winVersionQuery.h>
#include <drv_utils.h>
#include <gpuVendor.h>
#include <gpuVendorIntel.h>
#include <EASTL/optional.h>
#include <EASTL/string_view.h>
#include <EASTL/unordered_map.h>

#if _TARGET_PC_WIN
#include <winapi_helpers.h>
#include <Windows.h>
#endif

#if USE_MULTI_D3D_DX11
namespace d3d_multi_dx11
{
#include "d3d_api.inc.h"
}
#endif

#if USE_MULTI_D3D_DX12
namespace d3d_multi_dx12
{
#include "d3d_api.inc.h"
}
APISupport get_dx12_support_status();
#endif

#if USE_MULTI_D3D_vulkan
namespace d3d_multi_vulkan
{
#include "d3d_api.inc.h"
}
bool is_vulkan_supported(bool only_when_preferred);
#endif

#if USE_MULTI_D3D_Metal
namespace d3d_multi_metal
{
#include "d3d_api.inc.h"
}
bool is_metal_available();
#endif

#if USE_MULTI_D3D_stub
namespace d3d_multi_stub
{
#include "d3d_api.inc.h"
}
#endif


[[maybe_unused]]
static constexpr auto release = d3d::drivercode::matcher::Const<DAGOR_DBGLEVEL == 0>{};

#if !_TARGET_PC_WIN
static constexpr eastl::array<eastl::pair<uint16_t, uint16_t>, 0> getGDIs() { return {}; }
#endif

#if USE_MULTI_D3D_DX11
static bool is_dx11_supported_by_os()
{
  const char *arch = [] {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    switch (si.wProcessorArchitecture)
    {
      case PROCESSOR_ARCHITECTURE_AMD64: return "x64";
      case PROCESSOR_ARCHITECTURE_ARM64: return "arm64";
      default: return "x86";
    }
  }();

  OSVERSIONINFOEXW osvi{.dwOSVersionInfoSize = sizeof(osvi)};
  if (get_version_ex(&osvi))
    debug("detected Windows %s v%d.%d, %s", osvi.wProductType == VER_NT_WORKSTATION ? "Workstation" : "Server", osvi.dwMajorVersion,
      osvi.dwMinorVersion, arch);
  else
  {
    debug("Unable to detect Windows version, default to XP, %s", arch);
    osvi.dwMajorVersion = 5;
  }

  if (osvi.dwMajorVersion == 6) // Windows Vista - Windows 8
  {
    if (osvi.dwMinorVersion >= 1 ||                                // Windows 7, Windows 8
        (osvi.dwMinorVersion == 0 && osvi.wServicePackMajor >= 2)) // Vista with SP2 may have DX11 //-V560
    {
      return true;
    }
  }

  if (osvi.dwMajorVersion >= 10) // Windows 10 and next
    return true;

  return false;
}

static void message_box_os_compatibility_mode()
{
  static bool shown = false;
  if (!shown)
    drv_message_box(get_localized_text("msgbox/os_compatibility",
                      "The game is running in Windows compatibility mode and can have stability and performance issues."),
      get_localized_text("video/settings_adjusted_hdr"), GUI_MB_ICON_INFORMATION);
  shown = true;
}
#endif

static bool test_preferred_driver(const DataBlock *gpu_cfg, auto &gdis)
{
  if (gpu_cfg)
  {
    for (auto [vendorId, deviceId] : gdis)
    {
      if (gpu::is_preferred_device(*gpu_cfg, vendorId, deviceId, {}))
        return true;
    }
  }
  return false;
}

/**
 * If only Intel GPUs are present - with no NVIDIA, AMD, or other vendors - and
 * none of the Intel devices are Arc-class or newer, we assume the GPUs are
 * integrated UHD/HD or older models that typically perform poorly with DX12.
 * In this case, the preferred driver becomes DX11.
 * This is applied only on release builds. It is assumed on dev build the users know what they are doing
 * and this function won't override their decision.
 */
static eastl::optional<DriverCode> override_driver_preference([[maybe_unused]] const auto candidate_driver, const auto &gdis)
{
  if (gdis.empty())
    return {};

  [[maybe_unused]] bool anyModernIntel = false;
  for (auto [vendorId, deviceId] : gdis)
  {
    if (d3d_get_vendor(vendorId) != GpuVendor::INTEL)
      return {};
    anyModernIntel = anyModernIntel || !gpu::IsPreXe(deviceId);
  }

#if USE_MULTI_D3D_DX12
  if (anyModernIntel && candidate_driver.is((release && d3d::dx11) || d3d::undefined))
    return DriverCode::make(d3d::dx12);
#endif

#if USE_MULTI_D3D_DX11
  if (!anyModernIntel && candidate_driver.is(release && d3d::dx12))
    return DriverCode::make(d3d::dx11);
#endif

  return {};
}

static DriverCode detect_driver()
{
  const eastl::unordered_map<eastl::string_view, DriverCode> codes{
#if USE_MULTI_D3D_DX11
    {"dx11", DriverCode::make(d3d::dx11)},
#endif
#if USE_MULTI_D3D_DX12
    {"dx12", DriverCode::make(d3d::dx12)},
#endif
#if USE_MULTI_D3D_vulkan
    {"vulkan", DriverCode::make(d3d::vulkan)},
#endif
#if USE_MULTI_D3D_Metal
    {"metal", DriverCode::make(d3d::metal)},
#endif
#if USE_MULTI_D3D_stub
    {"stub", DriverCode::make(d3d::stub)},
#endif
  };

  const auto gDIs = getGDIs();

  auto &video = *::dgs_get_settings()->getBlockByNameEx("video");
  const char *driver;
#if DAGOR_DBGLEVEL > 0
  if (auto driverName = ::dgs_get_argv("driver"); driverName && *driverName)
    driver = driverName;
  else
#endif
    driver = video.getStr("driver", "auto");

  auto autoDriverMode = strcmp(driver, "auto") == 0;
  if (autoDriverMode)
  {
    if (video.paramExists("autoDriver"))
      driver = video.getStr("autoDriver");
    else
    {
      if (test_preferred_driver(::dgs_get_settings()->getBlockByNameEx("dx12")->getBlockByNameEx("gpuPreferences"), gDIs))
        driver = "dx12";
    }
  }

  auto it = codes.find(driver);
  auto candidateDriver = it == codes.end() ? DriverCode::make(d3d::undefined) : it->second;

  if (video.getBool("overrideDriver", false))
  {
    if (auto overrideDriver = override_driver_preference(candidateDriver, gDIs); overrideDriver)
      candidateDriver = *overrideDriver;
  }

  logdbg("Mapping driver:t=%s to DriverCode:0x%04X", driver, candidateDriver.asFourCC());

  for (int i = 0; i < 2; i++)
  {
#if USE_MULTI_D3D_DX12
    if (candidateDriver == d3d::dx12)
    {
#if _TARGET_64BIT
      if (!crash_fallback_helper || !crash_fallback_helper->previousStartupFailed())
      {
        if (crash_fallback_helper)
          crash_fallback_helper->beforeStartup();

        const APISupport status = get_dx12_support_status();
        switch (status)
        {
          case APISupport::FULL_SUPPORT: return candidateDriver;
          case APISupport::OUTDATED_DRIVER:
          case APISupport::BLACKLISTED_DRIVER: // TODO: add separate message for blacklisted driver.
          {
            if (::dgs_execute_quiet)
            {
              debug_flush(false);
              _exit(1);
            }

            const char *address = "https://support.gaijin.net/hc/articles/4405867465489";
            String message(1024, "%s\n<a href=\"%s\">%s</a>", get_localized_text("video/outdated_driver"), address, address);
            drv_message_box(message.data(), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK | GUI_MB_ICON_ERROR);
          }
          break;
          case APISupport::INSUFFICIENT_DEVICE:
            if (!autoDriverMode)
            {
              if (crash_fallback_helper)
                crash_fallback_helper->setSettingToAuto();
              if (!::dgs_execute_quiet)
              {
                drv_message_box(get_localized_text("video/d3d12_not_full_support_text"),
                  get_localized_text("video/d3d12_not_full_support_caption"), GUI_MB_OK);
              }
            }
            break;
          default: break;
        }

#if USE_MULTI_D3D_vulkan
        if (is_vulkan_supported(true /*only_when_preferred*/))
          return DriverCode::make(d3d::vulkan);
#endif
      }
      else if (crash_fallback_helper)
      {
        crash_fallback_helper->setSettingToAuto();
        if (!::dgs_execute_quiet)
        {
          drv_message_box(get_localized_text("video/previous_run_failed_text"),
            get_localized_text("video/previous_run_failed_caption"), GUI_MB_OK);
        }
      }
#endif
      // if we fail, behave like auto was selected
      autoDriverMode = true;
    }
#endif

#if USE_MULTI_D3D_vulkan
    if (candidateDriver == d3d::vulkan)
      return candidateDriver;
#endif

#if USE_MULTI_D3D_Metal
    if (candidateDriver == d3d::metal)
    {
      if (is_metal_available())
      {
        d3d_multi_metal::fill_interface_table(d3di);
        return candidateDriver;
      }

      drv_message_box(get_localized_text("video/incompatible_mac"), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK);
      exit(1);
    }
#endif

#if USE_MULTI_D3D_DX11
    if (candidateDriver == d3d::dx11)
      return candidateDriver;
#endif

#if USE_MULTI_D3D_stub
    if (candidateDriver == d3d::stub)
      return candidateDriver;
#endif

    if (!autoDriverMode)
      logwarn("ignore unsupported driver:t=%s and switching to auto", video.getStr("driver", ""));

#if USE_MULTI_D3D_DX11
    if (is_dx11_supported_by_os())
      return DriverCode::make(d3d::dx11);
    else if (detect_os_compatibility_mode())
    {
      message_box_os_compatibility_mode();
      return DriverCode::make(d3d::dx11);
    }
#endif

#if USE_MULTI_D3D_Metal
    if (is_metal_available())
    {
      d3d_multi_metal::fill_interface_table(d3di);
      return DriverCode::make(d3d::metal);
    }
    else
    {
      drv_message_box(get_localized_text("video/incompatible_mac"), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK);
      exit(1);
    }
#endif
  }

#if USE_MULTI_D3D_vulkan
  return DriverCode::make(d3d::vulkan);
#elif USE_MULTI_D3D_stub
  return DriverCode::make(d3d::stub);
#endif
  return DriverCode::make(d3d::undefined);
}

static DriverCode active_driver = DriverCode::make(d3d::undefined);

static DriverCode get_selected_driver()
{
  if (active_driver == d3d::undefined)
  {
    active_driver = detect_driver();
    if (active_driver == d3d::undefined)
      DAG_FATAL("D3D API not selected, settings.blk: video { driver:t=\"%s\" }",
        ::dgs_get_settings()->getBlockByNameEx("video")->getStr("driver", ""));
    else
      logdbg("Set active_driver to 0x%04X", active_driver.asFourCC());
  }

  return active_driver;
}

static auto get_selected_driver_map() { return get_selected_driver().map<bool>(); }

static void init_driver_desc(const DriverDesc &drv_desc) { memcpy(&d3di.drvDesc, &drv_desc, sizeof(d3di.drvDesc)); }

#if !DAGOR_HOSTED_INTERNAL_SERVER
DriverCode d3d::get_driver_code()
{
  if (is_inited())
    return d3di.driverCode;

  return get_selected_driver();
}
#endif

bool d3d::is_inited()
{
  return get_selected_driver_map()
#if USE_MULTI_D3D_DX11
    .map(d3d::dx11, [] { return d3d_multi_dx11::is_inited(); })
#endif
#if USE_MULTI_D3D_DX12
    .map(d3d::dx12, [] { return d3d_multi_dx12::is_inited(); })
#endif
#if USE_MULTI_D3D_vulkan
    .map(d3d::vulkan, [] { return d3d_multi_vulkan::is_inited(); })
#endif
#if USE_MULTI_D3D_Metal
    .map(d3d::metal, [] { return d3d_multi_metal::is_inited(); })
#endif
#if USE_MULTI_D3D_stub
    .map(d3d::stub, [] { return d3d_multi_stub::is_inited(); })
#endif
    .map(d3d::any, false);
}

bool d3d::init_video(void *hinst, main_wnd_f *f, const char *wcname, int ncmdshow, void *&mainwnd, void *hicon, const char *title,
  Driver3dInitCallback *cb)
{
  return get_selected_driver_map()
#if USE_MULTI_D3D_DX11
    .map(d3d::dx11,
      [&] {
        if (d3d_multi_dx11::init_video(hinst, f, wcname, ncmdshow, mainwnd, hicon, title, cb))
        {
          init_driver_desc(d3d_multi_dx11::get_driver_desc());
          return true;
        }

        if (detect_os_compatibility_mode())
          message_box_os_compatibility_mode();

        drv_message_box(
          get_localized_text("msgbox/dx9_gpu", "The game requires videocard with DirectX 10 or higher hardware support. "),
          "Initialization error", GUI_MB_OK | GUI_MB_ICON_ERROR);

        return false;
      })
#endif
#if USE_MULTI_D3D_DX12
    .map(d3d::dx12,
      [&] {
        if (d3d_multi_dx12::init_video(hinst, f, wcname, ncmdshow, mainwnd, hicon, title, cb))
        {
          init_driver_desc(d3d_multi_dx12::get_driver_desc());
          return true;
        }
        return false;
      })
#endif
#if USE_MULTI_D3D_vulkan
    .map(d3d::vulkan,
      [&] {
        if (d3d_multi_vulkan::init_video(hinst, f, wcname, ncmdshow, mainwnd, hicon, title, cb))
        {
          init_driver_desc(d3d_multi_vulkan::get_driver_desc());
          return true;
        }
        return false;
      })
#endif
#if USE_MULTI_D3D_Metal
    .map(d3d::metal,
      [&] {
        if (d3d_multi_metal::init_video(hinst, f, wcname, ncmdshow, mainwnd, hicon, title, cb))
        {
          init_driver_desc(d3d_multi_metal::get_driver_desc());
          return true;
        }
        return false;
      })
#endif
#if USE_MULTI_D3D_stub
    .map(d3d::stub,
      [&] {
        if (d3d_multi_stub::init_video(hinst, f, wcname, ncmdshow, mainwnd, hicon, title, cb))
        {
          init_driver_desc(d3d_multi_stub::get_driver_desc());
          return true;
        }
        return false;
      })
#endif
    .map(d3d::any, false);
}

bool d3d::init_driver()
{
  return get_selected_driver_map()
#if USE_MULTI_D3D_DX11
    .map(d3d::dx11,
      [] {
        d3d_multi_dx11::fill_interface_table(d3di);
        return d3d_multi_dx11::init_driver();
      })
#endif
#if USE_MULTI_D3D_DX12
    .map(d3d::dx12,
      [] {
        d3d_multi_dx12::fill_interface_table(d3di);
        return d3d_multi_dx12::init_driver();
      })
#endif
#if USE_MULTI_D3D_vulkan
    .map(d3d::vulkan,
      [] {
        d3d_multi_vulkan::fill_interface_table(d3di);
        return d3d_multi_vulkan::init_driver();
      })
#endif
#if USE_MULTI_D3D_Metal
    .map(d3d::metal,
      [] {
        d3d_multi_metal::fill_interface_table(d3di);
        return d3d_multi_metal::init_driver();
      })
#endif
#if USE_MULTI_D3D_stub
    .map(d3d::stub,
      [] {
        d3d_multi_stub::fill_interface_table(d3di);
        return d3d_multi_stub::init_driver();
      })
#endif
    .map(d3d::any, false);
}

void d3d::release_driver()
{
  if (crash_fallback_helper)
  {
    crash_fallback_helper->successfullyLoaded();
    crash_fallback_helper.reset();
  }

  get_selected_driver()
#if USE_MULTI_D3D_DX11
    .match(d3d::dx11, [] { d3d_multi_dx11::release_driver(); })
#endif
#if USE_MULTI_D3D_DX12
    .match(d3d::dx12, [] { d3d_multi_dx12::release_driver(); })
#endif
#if USE_MULTI_D3D_vulkan
    .match(d3d::vulkan, [] { d3d_multi_vulkan::release_driver(); })
#endif
#if USE_MULTI_D3D_Metal
    .match(d3d::metal, [] { d3d_multi_metal::release_driver(); })
#endif
#if USE_MULTI_D3D_stub
    .match(d3d::stub, [] { d3d_multi_stub::release_driver(); })
#endif
    ;
}

bool d3d::fill_interface_table(D3dInterfaceTable &d3dit)
{
  return get_selected_driver_map()
#if USE_MULTI_D3D_DX11
    .map(d3d::dx11, [&] { return d3d_multi_dx11::fill_interface_table(d3dit); })
#endif
#if USE_MULTI_D3D_DX12
    .map(d3d::dx12, [&] { return d3d_multi_dx12::fill_interface_table(d3dit); })
#endif
#if USE_MULTI_D3D_vulkan
    .map(d3d::vulkan, [&] { return d3d_multi_vulkan::fill_interface_table(d3dit); })
#endif
#if USE_MULTI_D3D_Metal
    .map(d3d::metal, [&] { return d3d_multi_metal::fill_interface_table(d3dit); })
#endif
#if USE_MULTI_D3D_stub
    .map(d3d::stub, [&] { return d3d_multi_stub::fill_interface_table(d3dit); })
#endif
    .map(d3d::any, false);
}

void d3d::prepare_for_destroy()
{
  get_selected_driver()
#if USE_MULTI_D3D_DX11
    .match(d3d::dx11, [] { d3d_multi_dx11::prepare_for_destroy(); })
#endif
#if USE_MULTI_D3D_DX12
    .match(d3d::dx12, [] { d3d_multi_dx12::prepare_for_destroy(); })
#endif
#if USE_MULTI_D3D_vulkan
    .match(d3d::vulkan, [] { d3d_multi_vulkan::prepare_for_destroy(); })
#endif
    ;
}

void d3d::window_destroyed(void *hwnd)
{
  get_selected_driver()
#if USE_MULTI_D3D_DX11
    .match(d3d::dx11, [=] { d3d_multi_dx11::window_destroyed(hwnd); })
#endif
#if USE_MULTI_D3D_DX12
    .match(d3d::dx12, [=] { d3d_multi_dx12::window_destroyed(hwnd); })
#endif
#if USE_MULTI_D3D_vulkan
    .match(d3d::vulkan, [=] { d3d_multi_vulkan::window_destroyed(hwnd); })
#endif
#if USE_MULTI_D3D_Metal
    .match(d3d::metal, [=] { d3d_multi_metal::window_destroyed(hwnd); })
#endif
#if USE_MULTI_D3D_stub
    .match(d3d::stub, [=] { d3d_multi_stub::window_destroyed(hwnd); })
#endif
    ;
}

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib, int m)
{
  get_selected_driver()
#if USE_MULTI_D3D_DX11
    .match(d3d::dx11, [&] { d3d_multi_dx11::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m); })
#endif
#if USE_MULTI_D3D_DX12
    .match(d3d::dx12, [&] { d3d_multi_dx12::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m); })
#endif
#if USE_MULTI_D3D_vulkan
    .match(d3d::vulkan,
      [&] { d3d_multi_vulkan::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m); })
#endif
#if USE_MULTI_D3D_Metal
    .match(d3d::metal,
      [&] { d3d_multi_metal::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m); })
#endif
#if USE_MULTI_D3D_stub
    .match(d3d::stub, [&] { d3d_multi_stub::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m); })
#endif
    ;
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  get_selected_driver()
#if USE_MULTI_D3D_DX11
    .match(d3d::dx11, [&] { d3d_multi_dx11::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_DX12
    .match(d3d::dx12, [&] { d3d_multi_dx12::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_vulkan
    .match(d3d::vulkan,
      [&] { d3d_multi_vulkan::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_Metal
    .match(d3d::metal,
      [&] { d3d_multi_metal::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_stub
    .match(d3d::stub, [&] { d3d_multi_stub::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
    ;

  max_tex = max_vs = max_ps = max_vdecl = max_vb = max_ib = max_stblk = 0;
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  get_selected_driver()
#if USE_MULTI_D3D_DX11
    .match(d3d::dx11, [&] { d3d_multi_dx11::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_DX12
    .match(d3d::dx12, [&] { d3d_multi_dx12::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_vulkan
    .match(d3d::vulkan,
      [&] { d3d_multi_vulkan::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_Metal
    .match(d3d::metal,
      [&] { d3d_multi_metal::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
#if USE_MULTI_D3D_stub
    .match(d3d::stub, [&] { d3d_multi_stub::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk); })
#endif
    ;

  max_tex = max_vs = max_ps = max_vdecl = max_vb = max_ib = max_stblk = 0;
}

D3dInterfaceTable d3di;

#ifdef _MSC_VER
#pragma warning(disable : 4074)
#endif
#pragma init_seg(compiler)
static class __Drv3dVtableInit
{
public:
  __Drv3dVtableInit()
  {
    // fill table with guard/fatal functions first
    void (**funcTbl)() = (void (**)()) & d3di; // -V580
    for (int i = 0; i < sizeof(d3di) / sizeof(*funcTbl); i++)
      funcTbl[i] = &na_func;
    d3di.driverCode = DriverCode::make(d3d::undefined);
    d3di.driverName = "n/a";
    d3di.driverVer = "n/a";
    d3di.deviceName = "n/a";
    memset(&d3di.drvDesc, 0, sizeof(d3di.drvDesc));
  }
  static void na_func() { DAG_FATAL("D3DI function not implemented"); }
} __drv3d_vtable_init;

#if _TARGET_PC_MACOSX && !USE_MULTI_D3D_Metal
void destroy_cached_window_data(void *) {}
#if !DAGOR_HOSTED_INTERNAL_SERVER
GpuVendor d3d::guess_gpu_vendor(String *, uint32_t *, DagorDateTime *, uint32_t *) { return {}; }
unsigned d3d::get_dedicated_gpu_memory_size_kb() { return 0; }
#endif
#endif
