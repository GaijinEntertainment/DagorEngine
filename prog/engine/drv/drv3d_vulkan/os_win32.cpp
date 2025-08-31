// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <debug/dag_debug.h>
#include <osApiWrappers/dag_unicode.h>
#include <math/integer/dag_IPoint2.h>
#include <debug/dag_logSys.h>

#include <windows.h>
#include <stdlib.h>
#include <WinTrust.h>
#include <Softpub.h>
#include "os.h"
#include <startup/dag_globalSettings.h>
#include "globals.h"

namespace
{
bool displayModeModified = false;
}

using namespace drv3d_vulkan;
// code borrowed from opengl driver
void drv3d_vulkan::get_video_modes_list(Tab<String> &list)
{
  clear_and_shrink(list);

  DEVMODE devMode;
  devMode.dmSize = sizeof(devMode);
  Tab<DEVMODE> uniqueModes;
  for (int modeNo = 0;; modeNo++)
  {
    if (!EnumDisplaySettings(NULL, modeNo, &devMode))
      break;

    if (devMode.dmPelsWidth >= MIN_LISTED_MODE_WIDTH && devMode.dmPelsHeight >= MIN_LISTED_MODE_HEIGHT)
    {
      int testModeNo = 0;
      for (; testModeNo < uniqueModes.size(); testModeNo++)
      {
        if (uniqueModes[testModeNo].dmPelsWidth == devMode.dmPelsWidth && uniqueModes[testModeNo].dmPelsHeight == devMode.dmPelsHeight)
        {
          break;
        }
      }

      if (testModeNo == uniqueModes.size())
      {
        uniqueModes.push_back(devMode);
        list.push_back(String(64, "%d x %d", devMode.dmPelsWidth, devMode.dmPelsHeight));
      }
    }
  }
}

bool drv3d_vulkan::validate_vulkan_signature(void *file)
{
  WCHAR filePath[MAX_PATH];
  GetModuleFileNameW(static_cast<HMODULE>(file), filePath, MAX_PATH);
  WINTRUST_FILE_INFO fileInfo = {sizeof(WINTRUST_FILE_INFO), filePath, 0};

  GUID WVTPolicyGUID = WINTRUST_ACTION_GENERIC_VERIFY_V2;

  WINTRUST_DATA trustData = {sizeof(WINTRUST_DATA)};
  trustData.dwUIChoice = WTD_UI_NOGOOD;
  trustData.fdwRevocationChecks = WTD_REVOKE_WHOLECHAIN;
  trustData.dwUnionChoice = WTD_CHOICE_FILE;
  trustData.dwStateAction = WTD_STATEACTION_VERIFY;
  trustData.dwProvFlags = WTD_REVOCATION_CHECK_CHAIN | WTD_LIFETIME_SIGNING_FLAG
#ifdef WTD_DISABLE_MD2_MD4
                          | WTD_DISABLE_MD2_MD4
#endif
    ;
  trustData.pFile = &fileInfo;
  auto result = WinVerifyTrust(NULL, &WVTPolicyGUID, &trustData);

  trustData.dwStateAction = WTD_STATEACTION_CLOSE;
  WinVerifyTrust(NULL, &WVTPolicyGUID, &trustData);

  switch (result)
  {
    case ERROR_SUCCESS: return true;
    case TRUST_E_NOSIGNATURE:
      MessageBoxA(NULL, "Your Vulkan driver is not digitaly signed, disabling Vulkan", "Vulkan Driver", 0);
      return false;
    case TRUST_E_EXPLICIT_DISTRUST:
    case TRUST_E_SUBJECT_NOT_TRUSTED:
      MessageBoxA(NULL, "The signature of your Vulkan driver is not been trusted, disabling Vulkan", "Vulkan Driver", 0);
      return false;
    case CRYPT_E_SECURITY_SETTINGS:
      MessageBoxA(NULL, "The signature of your Vulkan driver is not been trusted, because of your security settings, disabling Vulkan",
        "Vulkan Driver", 0);
      return false;
    case CERT_E_REVOKED:
      MessageBoxA(NULL, "The signature of your Vulkan driver has been revoked, disabling Vulkan", "Vulkan Driver", 0);
      return false;
    case CERT_E_EXPIRED:
      MessageBoxA(NULL, "The signature of your Vulkan driver has been expired, disabling Vulkan", "Vulkan Driver", 0);
      return false;
    case TRUST_E_BAD_DIGEST:
      MessageBoxA(NULL, "The signature of your Vulkan driver could not be verified, disabling Vulkan", "Vulkan Driver", 0);
      return false;
    default:
      MessageBoxA(NULL, "The signature of your Vulkan driver is not been trusted, disabling Vulkan", "Vulkan Driver", 0);
      return false;
  }
}

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance)
{
  VulkanSurfaceKHRHandle result;
#if VK_KHR_win32_surface
  // no check needed, only this extension is supported to create
  // surfaces and we abort before ending up here if this is missing
  // if (instance.hasExtension<SurfaceWin32KHR>())
  {
    VkWin32SurfaceCreateInfoKHR w32sci;
    w32sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    w32sci.pNext = NULL;
    w32sci.flags = 0;
    w32sci.hinstance = reinterpret_cast<HINSTANCE>(Globals::window.params.hinst);
    w32sci.hwnd = reinterpret_cast<HWND>(Globals::window.getMainWindow());
    if (VULKAN_CHECK_FAIL(instance.vkCreateWin32SurfaceKHR(instance.get(), &w32sci, NULL, ptr(result))))
    {
      result = VulkanNullHandle();
    }
  }
#else
  G_UNUSED(instance);
  G_UNUSED(parms);
#endif
  return result;
}

void drv3d_vulkan::os_restore_display_mode()
{
  if (displayModeModified)
  {
    ChangeDisplaySettings(nullptr, 0);
    displayModeModified = false;
  }
  Globals::window.updateRefreshRateFromCurrentDisplayMode();
}

void drv3d_vulkan::os_set_display_mode(int res_x, int res_y)
{
  DEVMODE dm;
  dm.dmSize = sizeof(dm);
  if (!EnumDisplaySettings(nullptr, 0, &dm))
    return;
  int mode_w = dm.dmPelsWidth;
  int mode_h = dm.dmPelsHeight;
  int mode_b = dm.dmBitsPerPel;
  int mode_rd = (mode_w - res_x) * (mode_h - res_x) + (mode_h - res_y) * (mode_h - res_y);
  DEVMODE devm = dm;
  for (int mi = 1; mode_rd || mode_b != 32; ++mi)
  {
    if (!EnumDisplaySettings(nullptr, mi, &dm))
      break;

    if (dm.dmBitsPerPel != 32)
      continue;

    int w = dm.dmPelsWidth;
    int h = dm.dmPelsHeight;
    int d = (w - res_x) * (w - res_x) + (h - res_y) * (h - res_y);
    if (d < mode_rd)
    {
      mode_w = w;
      mode_h = h;
      mode_b = dm.dmBitsPerPel;
      mode_rd = d;
      devm = dm;
    }
  }

  devm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

  LONG res = ChangeDisplaySettings(&devm, CDS_FULLSCREEN);
  if (res != DISP_CHANGE_SUCCESSFUL)
  {
    if (res == DISP_CHANGE_RESTART)
      D3D_ERROR("vulkan: you should set display mode %dx%dx%d manually", mode_w, mode_h, mode_b);
    else
      D3D_ERROR("vulkan: error %u setting display mode %dx%dx%d", res, mode_w, mode_h, mode_b);
  }
  else
  {
    Globals::window.refreshRate = devm.dmDisplayFrequency;
    displayModeModified = true;
    if ((mode_w != res_x) || (mode_h != res_y))
      debug("vulkan: can't set display mode %dx%d, falling back to %dx%d", res_x, res_y, mode_w, mode_h);
  }
}

eastl::string drv3d_vulkan::os_get_additional_ext_requirements(VulkanPhysicalDeviceHandle, const dag::Vector<VkExtensionProperties> &)
{
  return "";
}

DAGOR_NOINLINE static void toggle_fullscreen(HWND hWnd, UINT message, WPARAM wParam)
{
  if (dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE)
    return;

  if (has_focus(hWnd, message, wParam))
  {
    RenderWindowSettings &wcfg = Globals::window.settings;
    os_set_display_mode(wcfg.resolutionX, wcfg.resolutionY);
    ShowWindow(hWnd, SW_RESTORE);
    SetWindowPos(hWnd, HWND_TOP, wcfg.winRectLeft, wcfg.winRectTop, wcfg.winRectRight - wcfg.winRectLeft,
      wcfg.winRectBottom - wcfg.winRectTop, 0);
  }
  else
  {
    os_restore_display_mode();
    ShowWindow(hWnd, SW_MINIMIZE);
  }
}

LRESULT CALLBACK drv3d_vulkan::WindowState::windowProcProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: paint_window(hWnd, message, wParam, lParam, mainCallback); return 1;
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    {
      toggle_fullscreen(hWnd, message, wParam);
      break;
    }
  }

  if (originWndProc)
    return CallWindowProcW(originWndProc, hWnd, message, wParam, lParam);
  if (mainCallback)
    return mainCallback(hWnd, message, wParam, lParam);
  return DefWindowProcW(hWnd, message, wParam, lParam);
}

void drv3d_vulkan::WindowState::updateRefreshRateFromCurrentDisplayMode()
{
  refreshRate = 0;

  DEVMODE dm;
  dm.dmSize = sizeof(dm);
  if (!EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &dm))
    return;
  refreshRate = dm.dmDisplayFrequency;
}
