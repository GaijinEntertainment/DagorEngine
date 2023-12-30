#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d_res.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <util/dag_localization.h>
#include <util/dag_string.h>
#include <startup/dag_globalSettings.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_messageBox.h>
#if _TARGET_PC_WIN
#include <windows.h>
#endif
#include <stdio.h>
#include "gpuConfig.h"
#include <3d/dag_ICrashFallback.h>

enum
{
  API_NONE = -1,
  API_DX11 = 0,
  API_STUB,
  API_VULKAN,
  API_METAL,
  API_DX12,
  API_COUNT
};

#if USE_MULTI_D3D_DX11
namespace d3d_multi_dx11
{
#include "d3d_api.h"
}
#endif

#if USE_MULTI_D3D_stub
namespace d3d_multi_stub
{
#include "d3d_api.h"
}
#endif

#if USE_MULTI_D3D_vulkan
namespace d3d_multi_vulkan
{
#include "d3d_api.h"
}
#endif

#if USE_MULTI_D3D_Metal
namespace d3d_multi_metal
{
#include "d3d_api.h"
}


extern bool isMetalAvailable();
#endif

#if USE_MULTI_D3D_DX12
namespace d3d_multi_dx12
{
#include "d3d_api.h"
}
APISupport get_dx12_support_status(bool use_any_device = true);
#endif


static const carray<DriverCode, API_COUNT> api_drv_codes = {
  DriverCode::make(d3d::dx11),   // API_DX11
  DriverCode::make(d3d::stub),   // API_STUB
  DriverCode::make(d3d::vulkan), // API_VULKAN
  DriverCode::make(d3d::metal),  // API_METAL
  DriverCode::make(d3d::dx12),   // API_DX12
};

D3dInterfaceTable d3di;
bool d3d::HALF_TEXEL_OFS = false;
float d3d::HALF_TEXEL_OFSFU = 0.f;

static int active_api = API_NONE;

#if USE_MULTI_D3D_DX11
static inline void select_api_dx11()
{
  d3d_multi_dx11::fill_interface_table(d3di);
  d3d::HALF_TEXEL_OFS = d3d_multi_dx11::HALF_TEXEL_OFS;
  d3d::HALF_TEXEL_OFSFU = d3d_multi_dx11::HALF_TEXEL_OFSFU;
}
#endif

#if USE_MULTI_D3D_stub
static inline void select_api_stub()
{
  d3d_multi_stub::fill_interface_table(d3di);
  d3d::HALF_TEXEL_OFS = d3d_multi_stub::HALF_TEXEL_OFS;
  d3d::HALF_TEXEL_OFSFU = d3d_multi_stub::HALF_TEXEL_OFSFU;
}
#endif

#if USE_MULTI_D3D_vulkan
static inline void select_api_vulkan()
{
  d3d_multi_vulkan::fill_interface_table(d3di);
  d3d::HALF_TEXEL_OFS = d3d_multi_vulkan::HALF_TEXEL_OFS;
  d3d::HALF_TEXEL_OFSFU = d3d_multi_vulkan::HALF_TEXEL_OFSFU;
}
#endif

#if USE_MULTI_D3D_Metal
static inline int select_api_metal()
{
  d3d_multi_metal::fill_interface_table(d3di);
  d3d::HALF_TEXEL_OFS = d3d_multi_metal::HALF_TEXEL_OFS;
  d3d::HALF_TEXEL_OFSFU = d3d_multi_metal::HALF_TEXEL_OFSFU;
  return API_METAL;
}
#endif
#if USE_MULTI_D3D_DX12
static inline void select_api_dx12()
{
  d3d_multi_dx12::fill_interface_table(d3di);
  d3d::HALF_TEXEL_OFS = d3d_multi_dx12::HALF_TEXEL_OFS;
  d3d::HALF_TEXEL_OFSFU = d3d_multi_dx12::HALF_TEXEL_OFSFU;
}
#endif

#if USE_MULTI_D3D_DX11
static bool is_dx11_supported_by_os()
{
#if _TARGET_PC_WIN
  OSVERSIONINFOEXW osvi;
  ZeroMemory(&osvi, sizeof(osvi));
  osvi.dwOSVersionInfoSize = sizeof(osvi);

  SYSTEM_INFO si;
  GetNativeSystemInfo(&si);
  const char *arch = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" : "x86";

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
#endif

  return false;
}


static void message_box_os_compatibility_mode()
{
#if _TARGET_STATIC_LIB // No os_message_box in daKernel.
  static bool show = true;
  if (show)
    os_message_box(get_localized_text("msgbox/os_compatibility",
                     "The game is running in Windows compatibility mode and can have stability and performance issues."),
      get_localized_text("video/settings_adjusted_hdr"), GUI_MB_ICON_INFORMATION);
  show = false;
#endif
}
#endif


static int detect_api(const char *drv)
{
#if USE_MULTI_D3D_DX12
  if (strcmp(drv, "dx12") == 0 || strcmp(drv, "auto") == 0)
  {
#if _TARGET_64BIT
    static bool messageShown = dgs_execute_quiet;
    if (!crash_fallback_helper || !crash_fallback_helper->previousStartupFailed())
    {
      if (crash_fallback_helper)
        crash_fallback_helper->beforeStartup();
      // this is divergent behavior compared to any other driver, but this was specifically
      // requested to test proper support even when explicitly requested.
      const APISupport status = get_dx12_support_status(strcmp(drv, "dx12") == 0);
      switch (status)
      {
        case APISupport::FULL_SUPPORT: return API_DX12;
        case APISupport::OUTDATED_DRIVER:
        case APISupport::BLACKLISTED_DRIVER: // TODO: add separate message for blacklisted driver.
          if (::dgs_execute_quiet)
          {
            debug_flush(false);
            _exit(1);
          }
          if (!messageShown)
          {
            messageShown = true;
            const char *address = "https://support.gaijin.net/hc/articles/4405867465489";
            String message(1024, "%s\n<a href=\"%s\">%s</a>", get_localized_text("video/outdated_driver"), address, address);
            os_message_box(message.data(), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK | GUI_MB_ICON_ERROR);
          }
          break;
        default: break;
      }
    }
    else if (crash_fallback_helper)
    {
      crash_fallback_helper->setSettingToAuto();
      if (!messageShown)
      {
        messageShown = true;
        os_message_box(get_localized_text("video/previous_run_failed_text"), get_localized_text("video/previous_run_failed_caption"),
          GUI_MB_OK);
      }
    }
#endif
    // if we fail, behave like auto was selected
    drv = "auto";
  }
#endif
#if USE_MULTI_D3D_vulkan
  if (strcmp(drv, "vulkan") == 0)
    return API_VULKAN;
#endif
  if (strcmp(drv, "metal") == 0)
  {
#if USE_MULTI_D3D_Metal
    if (isMetalAvailable())
      return select_api_metal();
#endif

    os_message_box(get_localized_text("video/incompatible_mac"), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK);
    exit(1);
  }
#if USE_MULTI_D3D_DX11
  if (strcmp(drv, "dx11") == 0)
    return API_DX11;
#endif
#if USE_MULTI_D3D_stub
  if (strcmp(drv, "stub") == 0)
    return API_STUB;
#endif

  if (strcmp(drv, "auto") != 0)
    logwarn("ignore unsupported driver:t=%s and switching to auto", drv);

#if USE_MULTI_D3D_DX11
  if (is_dx11_supported_by_os())
    return API_DX11;
  else if (detect_os_compatibility_mode())
  {
    message_box_os_compatibility_mode();
    return API_DX11;
  }
#endif

#if USE_MULTI_D3D_Metal
  if (isMetalAvailable())
  {
    return select_api_metal();
  }
  else
  {
    os_message_box(get_localized_text("video/incompatible_mac"), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK);
    exit(1);
  }
#endif

#if USE_MULTI_D3D_vulkan && !_TARGET_PC_WIN // Do not select Vulkan on Windows automatically, players do not have the shader binary.
  return API_VULKAN;
#elif USE_MULTI_D3D_stub
  return API_STUB;
#endif
  return API_NONE;
}


static inline int get_selected_api()
{
  if (active_api > API_NONE)
    return active_api;

  const char *drv = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("driver", "auto");
  active_api = detect_api(drv);
  if (active_api == API_NONE) //-V547
    DAG_FATAL("D3D API not selected, settings.blk: video { driver:t=\"%s\" }", drv);

  return active_api;
}

DriverCode d3d::get_driver_code()
{
  if (is_inited())
    return d3di.driverCode;
  int api = detect_api(::dgs_get_settings()->getBlockByNameEx("video")->getStr("driver", "auto"));
  return api >= 0 && api < API_COUNT ? api_drv_codes[api] : DriverCode::make(d3d::undefined); //-V560
}

bool d3d::init_driver()
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: select_api_dx11(); return d3d_multi_dx11::init_driver();
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: select_api_stub(); return d3d_multi_stub::init_driver();
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: select_api_vulkan(); return d3d_multi_vulkan::init_driver();
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: select_api_metal(); return d3d_multi_metal::init_driver();
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: select_api_dx12(); return d3d_multi_dx12::init_driver();
#endif
  }
  return false;
}
bool d3d::is_inited()
{
  if (active_api == API_NONE)
    return false;
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::is_inited();
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: return d3d_multi_stub::is_inited();
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::is_inited();
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: return d3d_multi_metal::is_inited();
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::is_inited();
#endif
  }
  return false;
}

bool d3d::init_video(void *hinst, main_wnd_f *f, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb)
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11:
      if (d3d_multi_dx11::init_video(hinst, f, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, cb))
      {
        d3d_multi_dx11::fill_interface_table(d3di);
        memcpy(&d3di.drvDesc, &d3d_multi_dx11::get_driver_desc(), sizeof(d3di.drvDesc));
        return true;
      }

#if _TARGET_STATIC_LIB // No os_message_box in daKernel.
      if (detect_os_compatibility_mode())
        message_box_os_compatibility_mode();

      if (!dgs_execute_quiet)
        os_message_box(
          get_localized_text("msgbox/dx9_gpu", "The game requires videocard with DirectX 10 or higher hardware support. "),
          "Initialization error", GUI_MB_OK | GUI_MB_ICON_ERROR);
      debug_flush(false);
      _exit(1);
#endif
      break;
#endif
#if USE_MULTI_D3D_stub
    case API_STUB:
      if (d3d_multi_stub::init_video(hinst, f, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, cb))
      {
        memcpy(&d3di.drvDesc, &d3d_multi_stub::get_driver_desc(), sizeof(d3di.drvDesc));
        return true;
      }
      break;
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN:
      if (d3d_multi_vulkan::init_video(hinst, f, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, cb))
      {
        d3d_multi_vulkan::fill_interface_table(d3di);
        memcpy(&d3di.drvDesc, &d3d_multi_vulkan::get_driver_desc(), sizeof(d3di.drvDesc));
        return true;
      }
      break;
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL:
      if (d3d_multi_metal::init_video(hinst, f, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, cb))
      {
        d3d_multi_metal::fill_interface_table(d3di);
        memcpy(&d3di.drvDesc, &d3d_multi_metal::get_driver_desc(), sizeof(d3di.drvDesc));
        return true;
      }
      break;
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12:
      if (d3d_multi_dx12::init_video(hinst, f, wcname, ncmdshow, mainwnd, renderwnd, hicon, title, cb))
      {
        d3d_multi_dx12::fill_interface_table(d3di);
        memcpy(&d3di.drvDesc, &d3d_multi_dx12::get_driver_desc(), sizeof(d3di.drvDesc));
        return true;
      }
      break;
#endif
  }
  return false;
}

void d3d::release_driver()
{
  if (crash_fallback_helper)
  {
    crash_fallback_helper->successfullyLoaded();
    crash_fallback_helper.reset();
  }

  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: d3d_multi_dx11::release_driver(); break;
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: d3d_multi_stub::release_driver(); break;
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: d3d_multi_vulkan::release_driver(); break;
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: d3d_multi_metal::release_driver(); break;
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: d3d_multi_dx12::release_driver(); break;
#endif
  }
  active_api = API_NONE;
}
bool d3d::fill_interface_table(D3dInterfaceTable &d3dit)
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::fill_interface_table(d3dit);
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: return d3d_multi_stub::fill_interface_table(d3dit);
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::fill_interface_table(d3dit);
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: return d3d_multi_metal::fill_interface_table(d3dit);
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::fill_interface_table(d3dit);
#endif
  }
  return false;
}

void d3d::prepare_for_destroy()
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::prepare_for_destroy();
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::prepare_for_destroy();
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::prepare_for_destroy();
#endif
    case API_STUB: return;
    default: return;
  }
}

void d3d::window_destroyed(void *hwnd)
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::window_destroyed(hwnd);
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: return d3d_multi_stub::window_destroyed(hwnd);
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::window_destroyed(hwnd);
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: return d3d_multi_metal::window_destroyed(hwnd);
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::window_destroyed(hwnd);
#endif
  }
}

void d3d::reserve_res_entries(bool strict_max, int max_tex, int max_vs, int max_ps, int max_vdecl, int max_vb, int max_ib, int m)
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m);
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: return d3d_multi_stub::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m);
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m);
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: return d3d_multi_metal::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m);
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::reserve_res_entries(strict_max, max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, m);
#endif
  }
}
void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: return d3d_multi_stub::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: return d3d_multi_metal::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::get_max_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
  }
  max_tex = max_vs = max_ps = max_vdecl = max_vb = max_ib = max_stblk = 0;
}
void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  switch (get_selected_api())
  {
#if USE_MULTI_D3D_DX11
    case API_DX11: return d3d_multi_dx11::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_stub
    case API_STUB: return d3d_multi_stub::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_vulkan
    case API_VULKAN: return d3d_multi_vulkan::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_Metal
    case API_METAL: return d3d_multi_metal::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
#if USE_MULTI_D3D_DX12
    case API_DX12: return d3d_multi_dx12::get_cur_used_res_entries(max_tex, max_vs, max_ps, max_vdecl, max_vb, max_ib, max_stblk);
#endif
  }
  max_tex = max_vs = max_ps = max_vdecl = max_vb = max_ib = max_stblk = 0;
}

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
    d3di.driverCode = DriverCode::make(d3d::null); // matches what was done in the past, but correct would be undefined
    d3di.driverName = "n/a";
    d3di.driverVer = "n/a";
    d3di.deviceName = "n/a";
    memset(&d3di.drvDesc, 0, sizeof(d3di.drvDesc));
  }
  static void na_func() { DAG_FATAL("D3DI function not implemented"); }
} __drv3d_vtable_init;
