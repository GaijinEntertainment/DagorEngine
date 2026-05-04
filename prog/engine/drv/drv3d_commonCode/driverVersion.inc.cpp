// Copyright (C) Gaijin Games KFT.  All rights reserved.

#if _TARGET_PC_WIN
#define INITGUID
#include <windows.h>
#include <winternl.h>
#include <cfgmgr32.h>
#include <Ntddvdeo.h>
#include <d3dkmthk.h>
#include <devpkey.h>
#endif
#include <cwchar>

#include <drv/3d/dag_consts.h>
#include <osApiWrappers/dag_miscApi.h>
#include <generic/dag_carray.h>
#include <generic/dag_span.h>
#include <EASTL/string_view.h>
#include <dag/dag_vector.h>

static const carray<const char *, 7> nvidia_dll_names = {
  "nvapi.dll",     //
  "nvapi64.dll",   //
  "nvd3dum.dll",   //
  "nvd3dumx.dll",  //
  "nvwgf2um.dll",  //
  "nvwgf2umx.dll", //
  "nv4_disp.dll"   //
};

// There are other dlls but they have different version numbering we have no use for.
static const carray<const char *, 4> ati_dll_names = {
  "aticfx32.dll", //
  "aticfx64.dll", //
  "atidxx32.dll", //
  "atidxx64.dll"  //
};

static const carray<const char *, 8> intel_dll_names = {
  "igd10iumd32.dll", //
  "igd10iumd64.dll", //
  "igd10umd32.dll",  //
  "igd10umd64.dll",  //
  "igdumd32.dll",    //
  "igdumd64.dll",    //
  "igdumdim32.dll",  //
  "igdumdim64.dll"   //
};

static const carray<dag::ConstSpan<const char *>, GPU_VENDOR_COUNT> vendor_dll_names{
  dag::ConstSpan<const char *>(), // GpuVendor::NONE
  dag::ConstSpan<const char *>(), // GpuVendor::MESA
  dag::ConstSpan<const char *>(), // GpuVendor::IMGTEC
  ati_dll_names,                  // GpuVendor::ATI
  nvidia_dll_names,               // GpuVendor::NVIDIA
  intel_dll_names,                // GpuVendor::INTEL
  dag::ConstSpan<const char *>(), // GpuVendor::APPLE
  dag::ConstSpan<const char *>(), // GpuVendor::SHIM_DRIVER
  dag::ConstSpan<const char *>(), // GpuVendor::ARM
  dag::ConstSpan<const char *>()  // GpuVendor::QUALCOMM
};

#if _TARGET_PC_WIN

#pragma comment(lib, "version.lib")
#pragma comment(lib, "Cfgmgr32.lib")

/**
 * @brief Iterates through all adapters from system, and if then current adapter's LUID matches to luid paramter then returns with this
 * adapters driver version and date.
 * @param luid Locally unique identifier of the requested adapter.
 * @param drv_ver Output parameter of found adapter's driver version.
 * @param drv_date Output parameter of found adapter's driver date.
 */
static bool get_driver_info_from_luid(const LUID &luid, uint32_t drv_ver[4], DagorDateTime &drv_date)
{
  const LPGUID guid = (LPGUID)&GUID_DISPLAY_DEVICE_ARRIVAL;
  const ULONG flag = CM_GET_DEVICE_INTERFACE_LIST_PRESENT;

  CONFIGRET cr;
  ULONG length = 0;
  if ((cr = CM_Get_Device_Interface_List_SizeW(&length, guid, nullptr, flag)) != CR_SUCCESS)
    return false;

  dag::Vector<wchar_t> deviceInterfaceBuffer(length);
  if ((cr = CM_Get_Device_Interface_ListW(guid, nullptr, deviceInterfaceBuffer.data(), length, flag)) != CR_SUCCESS)
    return false;

  for (auto it = deviceInterfaceBuffer.cbegin(); it != deviceInterfaceBuffer.cend();)
  {
    eastl::wstring_view deviceInterface{it};
    it = deviceInterface.end() + 1;
    if (deviceInterface.empty())
      continue;

    D3DKMT_OPENADAPTERFROMDEVICENAME adapter{
      .pDeviceName = deviceInterface.data(),
    };

    if (auto d3dResult = D3DKMTOpenAdapterFromDeviceName(&adapter); NT_SUCCESS(d3dResult))
    {
      D3DKMT_CLOSEADAPTER closeAdapter{adapter.hAdapter};
      D3DKMTCloseAdapter(&closeAdapter);
    }

    if (adapter.AdapterLuid.HighPart != luid.HighPart || adapter.AdapterLuid.LowPart != luid.LowPart)
      continue;

    DEVPROPTYPE devicePropertyType{};
    ULONG deviceInstanceIdLength = MAX_DEVICE_ID_LEN;
    wchar_t deviceInstanceId[MAX_DEVICE_ID_LEN] = {};

    if ((cr = CM_Get_Device_Interface_PropertyW(deviceInterface.data(), &DEVPKEY_Device_InstanceId, &devicePropertyType,
           (PBYTE)deviceInstanceId, &deviceInstanceIdLength, flag)) != CR_SUCCESS)
      return false;

    DEVINST deviceInstanceHandle{};
    if ((cr = CM_Locate_DevNodeW(&deviceInstanceHandle, deviceInstanceId, CM_LOCATE_DEVNODE_NORMAL)) != CR_SUCCESS)
      return false;

    dag::Vector<wchar_t> deviceIdBuffer(64);
    auto getProperty = [&](const DEVPROPKEY *property_key) {
      ULONG bufferSize = deviceIdBuffer.size();
      CONFIGRET ret = CM_Get_DevNode_PropertyW(deviceInstanceHandle, property_key, &devicePropertyType, (PBYTE)deviceIdBuffer.data(),
        &bufferSize, flag);
      if (ret != CR_BUFFER_SMALL)
        return ret;
      deviceIdBuffer.resize(bufferSize);
      return CM_Get_DevNode_PropertyW(deviceInstanceHandle, property_key, &devicePropertyType, (PBYTE)deviceIdBuffer.data(),
        &bufferSize, flag);
    };

    if ((cr = getProperty(&DEVPKEY_Device_DriverVersion)) != CR_SUCCESS)
      return false;

    swscanf(deviceIdBuffer.data(), L"%d %*c %d %*c %d %*c %d", &drv_ver[0], &drv_ver[1], &drv_ver[2], &drv_ver[3]);

    if ((cr = getProperty(&DEVPKEY_Device_DriverDate)) != CR_SUCCESS)
      return false;

    FILETIME driverDate{};
    memcpy(&driverDate, deviceIdBuffer.data(), sizeof(driverDate));

    SYSTEMTIME systime;
    FileTimeToSystemTime(&driverDate, &systime);
    drv_date.year = systime.wYear;
    drv_date.month = systime.wMonth;
    drv_date.day = systime.wDay;
    drv_date.hour = systime.wHour;
    drv_date.minute = systime.wMinute;
    drv_date.second = systime.wSecond;
    drv_date.microsecond = systime.wMilliseconds * 1000;

    return true;
  }

  return false;
}

static bool get_driver_info_from_dll(const char *dllName, uint32_t drv_ver[4], DagorDateTime &drv_date)
{
  memset(drv_ver, 0, sizeof(drv_ver[0]) * 4);
  memset(&drv_date, 0, sizeof(drv_date));

  DWORD versionHandle = 0;
  DWORD size = GetFileVersionInfoSize(dllName, &versionHandle);
  if (size > 0)
  {
    BYTE *versionInfo = new BYTE[size];
    VS_FIXEDFILEINFO *vsfi = NULL;
    UINT len = 0;
    if (GetFileVersionInfo(dllName, versionHandle, size, versionInfo) && VerQueryValueW(versionInfo, L"\\", (void **)&vsfi, &len) &&
        vsfi)
    {
      drv_ver[0] = HIWORD(vsfi->dwProductVersionMS);
      drv_ver[1] = LOWORD(vsfi->dwProductVersionMS);
      drv_ver[2] = HIWORD(vsfi->dwProductVersionLS);
      drv_ver[3] = LOWORD(vsfi->dwProductVersionLS);
    }
    delete[] versionInfo;
  }

  HMODULE driverModuleHandle = LoadLibrary(dllName);
  if (driverModuleHandle)
  {
    char driverFileName[MAX_PATH];
    DWORD res = GetModuleFileName(driverModuleHandle, driverFileName, sizeof(driverFileName));
    if (res > 0)
    {
      driverFileName[MAX_PATH - 1] = 0;
      HANDLE dllFileHandle = CreateFile(driverFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
      if (dllFileHandle != INVALID_HANDLE_VALUE)
      {
        FILETIME fileTimeWrite;
        if (GetFileTime(dllFileHandle, NULL, NULL, &fileTimeWrite))
        {
          SYSTEMTIME systime;
          FileTimeToSystemTime(&fileTimeWrite, &systime);
          drv_date.year = systime.wYear;
          drv_date.month = systime.wMonth;
          drv_date.day = systime.wDay;
          drv_date.hour = systime.wHour;
          drv_date.minute = systime.wMinute;
          drv_date.second = systime.wSecond;
          drv_date.microsecond = systime.wMilliseconds * 1000;
        }
        CloseHandle(dllFileHandle);
      }
    }
    FreeLibrary(driverModuleHandle);
  }

  return (drv_ver[0] != 0 && drv_date.year != 0);
}

static bool get_driver_info_from_dlls(dag::ConstSpan<const char *> dlls, uint32_t drv_ver[4], DagorDateTime &drv_date)
{
  for (int dllNo = 0; dllNo < dlls.size(); ++dllNo)
    if (get_driver_info_from_dll(dlls[dllNo], drv_ver, drv_date))
      return true;
  return false;
}

#endif