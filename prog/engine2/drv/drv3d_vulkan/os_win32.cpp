#include <debug/dag_debug.h>
#include <osApiWrappers/dag_unicode.h>
#include <math/integer/dag_IPoint2.h>
#include <debug/dag_logSys.h>

#include <windows.h>
#include <stdlib.h>
#include <WinTrust.h>
#include <Softpub.h>
#include "os.h"
#include "device.h"

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

VulkanSurfaceKHRHandle drv3d_vulkan::init_window_surface(VulkanInstance &instance, SurfaceCreateParams parms)
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
    w32sci.hinstance = parms.hInstance;
    w32sci.hwnd = parms.hWnd;
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

drv3d_vulkan::ScopedGPUPowerState::ScopedGPUPowerState(bool) {}

drv3d_vulkan::ScopedGPUPowerState::~ScopedGPUPowerState() {}