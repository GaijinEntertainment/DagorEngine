#include <3d/dag_drv3d.h>
#include <3d/dag_drv3dCmd.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_string.h>
#include <generic/dag_carray.h>
#if _TARGET_PC_WIN
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d11.h>
#endif
#include <stdio.h>
#include "gpuVendor.h"

static const carray<const char *, 7> nvidia_dll_names = {
  "nvapi.dll", "nvapi64.dll", "nvd3dum.dll", "nvd3dumx.dll", "nvwgf2um.dll", "nvwgf2umx.dll", "nv4_disp.dll"};

// There are other dlls but they have different version numbering we have no use for.
static const carray<const char *, 4> ati_dll_names = {"aticfx32.dll", "aticfx64.dll", "atidxx32.dll", "atidxx64.dll"};

static const carray<const char *, 8> intel_dll_names = {"igd10iumd32.dll", "igd10iumd64.dll", "igd10umd32.dll", "igd10umd64.dll",
  "igdumd32.dll", "igdumd64.dll", "igdumdim32.dll", "igdumdim64.dll"};

static const carray<dag::ConstSpan<const char *>, D3D_VENDOR_COUNT> vendor_dll_names{
  dag::ConstSpan<const char *>(), // D3D_VENDOR_NONE
  dag::ConstSpan<const char *>(), // D3D_VENDOR_MESA
  dag::ConstSpan<const char *>(), // D3D_VENDOR_IMGTEC
  ati_dll_names,                  // D3D_VENDOR_ATI
  nvidia_dll_names,               // D3D_VENDOR_NVIDIA
  intel_dll_names,                // D3D_VENDOR_INTEL
  dag::ConstSpan<const char *>(), // D3D_VENDOR_APPLE
  dag::ConstSpan<const char *>(), // D3D_VENDOR_SHIM_DRIVER
  dag::ConstSpan<const char *>(), // D3D_VENDOR_ARM
  dag::ConstSpan<const char *>()  // D3D_VENDOR_QUALCOMM
};

using namespace gpu;

static int cached_gpu_vendor = -1;
static uint32_t cached_gpu_device_id = -1;
static char cached_gpu_desc[2048] = "";
static uint32_t cached_gpu_drv_ver[4] = {0, 0, 0, 0};
static DagorDateTime cached_gpu_drv_date = {0};
static uint32_t cached_gpu_video_memory_mb = 0;
static uint32_t cached_gpu_free_video_memory_mb = 0;
static int cached_feature_level_supported = 0;

#if _TARGET_PC_WIN

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "version.lib")

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
      FreeLibrary(driverModuleHandle);
    }
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


static int get_dx11_feature_level_supported()
{
  int featureLevelSupported = 0;

  HMODULE dxgi = LoadLibrary("dxgi.dll"); // this is workaround. Optimus is working correctly only after loading this library!
  if (dxgi)
    FreeLibrary(dxgi);

  HMODULE d3d11dll = LoadLibraryA("d3d11.dll");
  if (d3d11dll)
  {
    PFN_D3D11_CREATE_DEVICE createDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11dll, "D3D11CreateDevice");
    if (createDevice)
    {
      D3D_FEATURE_LEVEL featureLevelsRequested[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
      ID3D11Device *device;
      ID3D11DeviceContext *context;
      HRESULT hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelsRequested, countof(featureLevelsRequested),
        D3D11_SDK_VERSION, &device, (D3D_FEATURE_LEVEL *)&featureLevelSupported, &context);
      if (SUCCEEDED(hr))
      {
        context->Release();
        device->Release();
      }
      else
        debug("D3D11CreateDevice failed.");
    }
    else
      debug("D3D11CreateDevice not found.");
    FreeLibrary(d3d11dll);
  }
  else
    debug("Can't load d3d11.dll");

  debug("get_dx11_feature_level_supported=%d", featureLevelSupported);

  return featureLevelSupported;
}

static int get_active_gpu_vendor_pc(String &out_active_gpu_description, uint32_t &out_video_memory_mb,
  uint32_t &out_free_video_memory_mb, int &feature_level_supported, uint32_t drv_ver[4], DagorDateTime &drv_date, uint32_t &device_id)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191) // 'type cast' : unsafe conversion from 'FARPROC'
#endif

  debug("get_active_gpu_vendor...");

  int vendor = D3D_VENDOR_NONE;
  out_active_gpu_description = "";
  out_video_memory_mb = 2048; // Fallback value to avoid quality adjustment on descrete GPU with unknown memory.
  out_free_video_memory_mb = 0;
  feature_level_supported = 0;
  int sharedMemoryMb = 0;

  HMODULE dxgi = LoadLibrary("dxgi.dll"); // this is workaround. Optimus is working correctly only after loading this library!
  if (dxgi)
    FreeLibrary(dxgi);

  HMODULE d3d11dll = LoadLibraryA("d3d11.dll");
  if (d3d11dll)
  {
    PFN_D3D11_CREATE_DEVICE createDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(d3d11dll, "D3D11CreateDevice");
    if (createDevice)
    {
      D3D_FEATURE_LEVEL featureLevelsRequested[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
      ID3D11Device *device;
      ID3D11DeviceContext *context;
      HRESULT hr = createDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, featureLevelsRequested, countof(featureLevelsRequested),
        D3D11_SDK_VERSION, &device, (D3D_FEATURE_LEVEL *)&feature_level_supported, &context);
      if (SUCCEEDED(hr))
      {
        IDXGIDevice *pDXGIDevice = NULL;
        device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
        IDXGIAdapter *adapter;
        if (pDXGIDevice)
        {
          if (SUCCEEDED(pDXGIDevice->GetAdapter(&adapter)))
          {
            DXGI_ADAPTER_DESC adapterDesc;
            adapter->GetDesc(&adapterDesc);
            char deviceName[512];
            wcstombs(deviceName, adapterDesc.Description, sizeof(deviceName));
            debug("adapter=<%s>, vendor=0x%X, dev_id=0x%X, subsys=0x%X, vid_mem=%dMB, sys_mem=%dMB, shared_mem=%dMB", deviceName,
              adapterDesc.VendorId, adapterDesc.DeviceId, adapterDesc.SubSysId, adapterDesc.DedicatedVideoMemory >> 20,
              adapterDesc.DedicatedSystemMemory >> 20, adapterDesc.SharedSystemMemory >> 20);
            out_active_gpu_description = deviceName;
            out_video_memory_mb = adapterDesc.DedicatedVideoMemory >> 20;
            vendor = d3d_get_vendor(adapterDesc.VendorId, deviceName);
            sharedMemoryMb = adapterDesc.SharedSystemMemory >> 20;
            device_id = adapterDesc.DeviceId;

            IDXGIAdapter3 *adapter3 = nullptr;
            if (SUCCEEDED(adapter->QueryInterface(&adapter3)))
            {
              DXGI_QUERY_VIDEO_MEMORY_INFO info;
              if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)) && info.Budget > 0)
              {
                debug("Local budget=%dMB", info.Budget >> 20);
                out_free_video_memory_mb = info.Budget >> 20;
              }
              else if (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info)) && info.Budget > 0)
              {
                debug("Non-local budget=%dMB", info.Budget >> 20);
                out_free_video_memory_mb = info.Budget >> 20;
              }

              adapter3->Release();
            }
            adapter->Release();
          }
          pDXGIDevice->Release();
        }

        HMODULE hAtiDLL = NULL;
#if HAS_NVAPI
        if (init_nvapi()                                                       // Always initializes if NV GPU is present.
            && NvAPI_D3D11_SetDepthBoundsTest(device, 1, 0.f, 1.f) == NVAPI_OK // Succeeds only if NV GPU is active.
            && NvAPI_D3D11_SetDepthBoundsTest(device, 0, 0.f, 1.f) == NVAPI_OK)
        {
          vendor = D3D_VENDOR_NVIDIA;
        }
        else
#endif
        {
#if _TARGET_64BIT
          hAtiDLL = GetModuleHandleW(L"atidxx64.dll");
#else
          hAtiDLL = GetModuleHandleW(L"atidxx32.dll");
#endif
          if (hAtiDLL)
          {
            PFNAmdDxExtCreate11 AmdDxExtCreate = (PFNAmdDxExtCreate11)GetProcAddress(hAtiDLL, "AmdDxExtCreate11");
            if (AmdDxExtCreate)
            {
              IAmdDxExt *amdExtension;
              HRESULT hr = AmdDxExtCreate(device, &amdExtension); // Succeeds only if ATI GPU is active.
              if (SUCCEEDED(hr) && amdExtension)
              {
                vendor = D3D_VENDOR_ATI;
                amdExtension->Release();
              }
            }
          }
        }

        get_driver_info_from_dlls(vendor_dll_names[vendor], drv_ver, drv_date); // Do it while the device is created and the dlls are
                                                                                // loaded.

        context->Release();
        device->Release();
        if (hAtiDLL)
          FreeLibrary(hAtiDLL);
      }
      else
        debug("D3D11CreateDevice failed.");
    }
    else
      debug("D3D11CreateDevice not found.");

    FreeLibrary(d3d11dll);
  }
  else
    debug("Can't load d3d11.dll");

  if (vendor == D3D_VENDOR_NVIDIA)
  {
    uint32_t nvDedicatedKb, nvSharedKb, nvDedicatedFreeKb;
    get_nv_gpu_memory(nvDedicatedKb, nvSharedKb, nvDedicatedFreeKb);
    if (nvDedicatedKb > 0)
    {
      debug("Nvidia GPU memory dedicated: %dMB, free dedicated: %dMB, shared system: %dMB", nvDedicatedKb >> 10,
        nvDedicatedFreeKb >> 10, nvSharedKb >> 10);

      out_video_memory_mb = nvDedicatedKb >> 10;
      sharedMemoryMb = nvSharedKb >> 10;
      if (!out_free_video_memory_mb) // Prefer budget returned by Win10.
        out_free_video_memory_mb = nvDedicatedFreeKb >> 10;
    }
  }

  if (out_video_memory_mb <= 1024) // GPU with low memory, rely on shared memory.
  {
    debug("GPU with %dMB of dedicated memory. Use %dMB of dedicated and shared memory.", out_video_memory_mb,
      out_video_memory_mb + sharedMemoryMb / 2);
    out_video_memory_mb = out_free_video_memory_mb = out_video_memory_mb + sharedMemoryMb / 2;
  }

  if (!out_free_video_memory_mb)
  {
    debug("Free GPU memory is unknown, fallback to total memory");
    out_free_video_memory_mb = out_video_memory_mb;
  }

  debug("get_active_gpu_vendor=%s, '%s', vid_mem_size=%dMB, free=%dMB", d3d_get_vendor_name(vendor), out_active_gpu_description,
    out_video_memory_mb, out_free_video_memory_mb);

  return vendor;

#ifdef _MSC_VER
#pragma warning(pop)
#endif
}

#elif _TARGET_PC_MACOSX
extern void mac_get_vdevice_info(int &vram, int &vendor, String &gpu_desc, bool &web_driver);
#endif


static int guess_gpu_vendor(String *out_gpu_desc, uint32_t *out_drv_ver, DagorDateTime *out_drv_date, uint32_t *device_id,
  uint32_t *out_gpu_video_memory_mb, int *out_feature_level_supported)
{
  if (cached_gpu_vendor == -1)
  {
    memset(cached_gpu_desc, 0, sizeof(cached_gpu_desc));
    memset(cached_gpu_drv_ver, 0, sizeof(cached_gpu_drv_ver));
    memset(&cached_gpu_drv_date, 0, sizeof(cached_gpu_drv_date));

#if _TARGET_PC_WIN
    // Detect dynamically switchable graphics.
    String activeGpuDescription;
    cached_gpu_vendor = get_active_gpu_vendor_pc(activeGpuDescription, cached_gpu_video_memory_mb, cached_gpu_free_video_memory_mb,
      cached_feature_level_supported, cached_gpu_drv_ver, cached_gpu_drv_date, cached_gpu_device_id);
    SNPRINTF(cached_gpu_desc, sizeof(cached_gpu_desc), "%s", activeGpuDescription.str());

#elif _TARGET_C1


#elif _TARGET_C2


#elif _TARGET_XBOX
    cached_gpu_vendor = D3D_VENDOR_ATI;
    TargetPlatform tp = get_platform_id();
    if (tp == TP_XBOXONE)
      strcpy(cached_gpu_desc, "XBOX1");
    else if (tp == TP_XBOX_SCARLETT)
      strcpy(cached_gpu_desc, "XBOXSCARLETT");
#elif _TARGET_PC_LINUX | _TARGET_ANDROID | _TARGET_IOS | _TARGET_TVOS
    if (d3d::is_inited())
    {
      const char *devName = NULL;
      uint32_t *drvVer = NULL;
      const uint32_t unknownDrvVer[4] = {0, 0, 0, 0};
      const char *ctxInfo = NULL;
      cached_gpu_vendor = d3d::driver_command(DRV3D_COMMAND_GET_VENDOR, &devName, &drvVer, &ctxInfo);
      SNPRINTF(cached_gpu_desc, sizeof(cached_gpu_desc), "%s [%s]", devName ? devName : "Unknown", ctxInfo ? ctxInfo : "None");
      memcpy(cached_gpu_drv_ver, drvVer ? drvVer : unknownDrvVer, sizeof(cached_gpu_drv_ver));
    }
    else
    {
      cached_gpu_vendor = -1;
    }
#elif _TARGET_PC_MACOSX
    int vram = 0;
    bool webDriver;
    String gpuDesc;
    mac_get_vdevice_info(vram, cached_gpu_vendor, gpuDesc, webDriver);
    SNPRINTF(cached_gpu_desc, sizeof(cached_gpu_desc), "%s", gpuDesc.str());
#else
    cached_gpu_vendor = D3D_VENDOR_NONE;
#endif
  }

  if (out_gpu_desc)
    *out_gpu_desc = strlen(cached_gpu_desc) > 0 ? cached_gpu_desc : "Unknown";
  if (out_drv_ver)
    memcpy(out_drv_ver, cached_gpu_drv_ver, sizeof(cached_gpu_drv_ver));
  if (out_drv_date)
    *out_drv_date = cached_gpu_drv_date;
  if (out_gpu_video_memory_mb)
    *out_gpu_video_memory_mb = cached_gpu_video_memory_mb;
  if (out_feature_level_supported)
    *out_feature_level_supported = cached_feature_level_supported;
  if (device_id)
    *device_id = cached_gpu_device_id;
  return cached_gpu_vendor != -1 ? cached_gpu_vendor : D3D_VENDOR_NONE;
}

int d3d::guess_gpu_vendor(String *out_gpu_desc, uint32_t *out_drv_ver, DagorDateTime *out_drv_date, uint32_t *device_id)
{
  return guess_gpu_vendor(out_gpu_desc, out_drv_ver, out_drv_date, device_id, nullptr, nullptr);
}

DagorDateTime d3d::get_gpu_driver_date(int vendor)
{
  DagorDateTime drv_date{};
#if _TARGET_PC_WIN
  uint32_t dummy[4];
  get_driver_info_from_dlls(vendor_dll_names[vendor], dummy, drv_date); // Do it while the device is created and the dlls are loaded.
#endif
  return drv_date;
}

#if _TARGET_PC_WIN

static void get_dedicated_gpu_memory_total_kb_dxgi(unsigned &out_dedicated, unsigned &out_dedicated_free)
{
  if (cached_gpu_vendor == -1)
    guess_gpu_vendor(nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);

  out_dedicated = cached_gpu_video_memory_mb << 10;
  out_dedicated_free = cached_gpu_free_video_memory_mb << 10;
}

void d3d::get_current_gpu_memory_kb(uint32_t *dedicated_total, uint32_t *dedicated_free)
{
  uint32_t dedicatedKb, sharedKb, dedicatedFreeKb;
  get_nv_gpu_memory(dedicatedKb, sharedKb, dedicatedFreeKb);

  if (dedicated_total)
    *dedicated_total = dedicatedKb;

  if (dedicated_free)
    *dedicated_free = dedicatedFreeKb;
}

#endif

static bool get_dedicated_gpu_memory_total_kb_driver(unsigned &out_dedicated, unsigned &out_dedicated_free)
{
  if (!d3d::is_inited())
    return false;

  return d3d::driver_command(DRV3D_COMMAND_GET_VIDEO_MEMORY_BUDGET, (void *)&out_dedicated, (void *)&out_dedicated_free, nullptr);
}

#if _TARGET_PC_WIN | _TARGET_PC_LINUX

static void get_dedicated_gpu_memory_total_kb(unsigned &out_dedicated, unsigned &out_dedicated_free)
{
  if (!get_dedicated_gpu_memory_total_kb_driver(out_dedicated, out_dedicated_free))
  {
#if _TARGET_PC_WIN
    get_dedicated_gpu_memory_total_kb_dxgi(out_dedicated, out_dedicated_free);
#else
    debug("d3d: GPU memory size before driver init is not available on platform");
#endif
  }
}

unsigned d3d::get_dedicated_gpu_memory_size_kb()
{
  unsigned dedicated = 0, dedicatedFree = 0;
  get_dedicated_gpu_memory_total_kb(dedicated, dedicatedFree);
  return dedicated;
}

unsigned d3d::get_free_dedicated_gpu_memory_size_kb()
{
  unsigned dedicated = 0, dedicatedFree = 0;
  get_dedicated_gpu_memory_total_kb(dedicated, dedicatedFree);
  return dedicatedFree;
}

#elif _TARGET_C1 | _TARGET_C2

#elif _TARGET_APPLE
unsigned d3d::get_free_dedicated_gpu_memory_size_kb() { return 0; }
#elif _TARGET_XBOX
unsigned d3d::get_free_dedicated_gpu_memory_size_kb() { return 0; }
#if _TARGET_XBOXONE
unsigned d3d::get_dedicated_gpu_memory_size_kb() { return 2 << 20; } // report GPU mem 2Gb for XBOX1 by default
#elif _TARGET_SCARLETT
unsigned d3d::get_dedicated_gpu_memory_size_kb()
{
  ConsoleModel model = get_console_model();
  if (model == ConsoleModel::XBOX_ANACONDA)
    return 6 << 20; // 6GB for Anaconda
  return 3 << 20;   // 3GB for Lockhart
}
#endif
#elif _TARGET_C3



#elif _TARGET_ANDROID
namespace
{
// reserve 1Gb for system and 1.5Gb for game needs
constexpr unsigned ANDROID_SYS_MEM_RESERVE_MB = (1 << 10);
constexpr unsigned ANDROID_GAME_MEM_RESERVE_MB = 1534;
// fallback to 1Gb if something is not right
constexpr unsigned ANDROID_GPU_MEM_FALLBACK_MB = (1 << 10);
// fallback to 512Mb if reported budgets is absolutely broken
constexpr unsigned ANDROID_GPU_MEM_FALLBACK_MIN_MB = 512;
// keep some memory free if we using whole amount of memory reported by driver
constexpr unsigned ANDROID_GPU_MEM_SAFETY_MARGIN_MB = 32;

static unsigned estimated_android_gpu_memory_kb = 0;
} // namespace

unsigned d3d::get_dedicated_gpu_memory_size_kb()
{
  unsigned dedicated = 0, dedicatedFree = 0;
  get_dedicated_gpu_memory_total_kb_driver(dedicated, dedicatedFree);

  // android GPU mem detection can give actual available memory or whole system memory size
  // in order to avoid reporting too much, use system and game reservation values
  // and if reservation causes too low values, use clamped fallback value
  const DataBlock *memCfg = dgs_get_settings()->getBlockByNameEx("android")->getBlockByNameEx("gpuMem");

  // usual amount of memory used up by system
  unsigned sysReserveMB = memCfg->getInt("sysReserveMB", ANDROID_SYS_MEM_RESERVE_MB);
  // usual amount of memory used up by game
  unsigned gameReserveMB = memCfg->getInt("gameReserveMB", ANDROID_GAME_MEM_RESERVE_MB);
  // usual minimal that our game needs to work properly
  unsigned fallbackMB = memCfg->getInt("fallbackMB", ANDROID_GPU_MEM_FALLBACK_MB);
  // cricital minimal that our game needs to at least run on lowest settings
  unsigned fallbackMinMB = memCfg->getInt("fallbackMinMB", ANDROID_GPU_MEM_FALLBACK_MIN_MB);
  // safety margin to avoid allocating whole reported memory if we try to use reported value
  unsigned safetyMarginMB = memCfg->getInt("safetyMarginMB", ANDROID_GPU_MEM_SAFETY_MARGIN_MB);

  unsigned driverReportedMB = dedicatedFree ? (dedicatedFree >> 10) : (dedicated >> 10);

  // estimated - estimation by whole device RAM assume + reserves
  // surely{Available} - estimation by driver report and safety margin
  // otherwise fallback minimal is used

  // convert game + system to total reserved
  unsigned totalReserved = sysReserveMB + gameReserveMB;
  // estimate how much we have for case when whole device RAM is reported by driver
  unsigned estimatedFreeMB = driverReportedMB - totalReserved;
  // cals how much memory we can surely use by driver reported values
  unsigned surelyAvailableMB = driverReportedMB - safetyMarginMB;

  // assume that if total reserved fits in driver reported, last one is equal to total RAM on device
  bool wholeRAMreported = totalReserved < driverReportedMB;
  // check that estimated memory is enough to run game
  bool estimatedIsEnough = estimatedFreeMB > fallbackMB;
  // check that surely available memory is enough to run game
  bool surelyIsEnough = surelyAvailableMB > fallbackMinMB;
  // check that surely available memory is valid at all
  bool surelyIsValid = driverReportedMB > safetyMarginMB;
  // check driver reported memory is somewhat good
  bool reportedIsValid = driverReportedMB > fallbackMinMB;

  // start by assuming that everything is broken
  unsigned retMB = fallbackMinMB;
  if (!reportedIsValid)
    ;
  else if (wholeRAMreported && estimatedIsEnough)
    retMB = estimatedFreeMB;
  else if (surelyIsValid && surelyIsEnough)
    retMB = surelyAvailableMB;

  unsigned retKB = retMB << 10;
  if (estimated_android_gpu_memory_kb != retKB)
  {
    estimated_android_gpu_memory_kb = retKB;

    debug("d3d: gpuMem: estimated %u Mb [%s%s%s%s%s%s] \n"
          // report both control flags and config/reported values
          "(%u Mb reported, %u+%u Mb reserved, %u Mb fallback, %u Mb minimal, %u Mb safety margin)",
      retMB,
      // valid/invalid
      reportedIsValid ? "V" : "I",
      // device/gpu
      wholeRAMreported ? "D" : "G",
      // reserve/margin
      estimatedIsEnough ? "R" : "M",
      // fitting/unsuited
      surelyIsValid ? "F" : "U",
      // enough/not enough
      surelyIsEnough ? "E" : "N",
      // budget/whole
      dedicatedFree ? "B" : "W",
      // config/reported values
      driverReportedMB, sysReserveMB, gameReserveMB, fallbackMB, fallbackMinMB, safetyMarginMB);
  }

  return estimated_android_gpu_memory_kb;
}

unsigned d3d::get_free_dedicated_gpu_memory_size_kb()
{
  unsigned dedicated = 0, dedicatedFree = 0;
  get_dedicated_gpu_memory_total_kb_driver(dedicated, dedicatedFree);

  return dedicatedFree;
}
#else
unsigned d3d::get_dedicated_gpu_memory_size_kb() { return 0; }
unsigned d3d::get_free_dedicated_gpu_memory_size_kb() { return 0; }
#endif

#if _TARGET_PC_WIN
static String video_vendor_info_str;

void d3d::get_video_vendor_str(String &out_str)
{
  if (!video_vendor_info_str.empty())
  {
    out_str = video_vendor_info_str;
    return;
  }

  get_video_nvidia_str(video_vendor_info_str);
  if (video_vendor_info_str.empty())
    get_video_ati_str(video_vendor_info_str);
  if (video_vendor_info_str.empty())
    video_vendor_info_str = "No info about graphics from vendor";

  debug("%s", video_vendor_info_str.str());
  out_str = video_vendor_info_str;
}

bool d3d::get_gpu_freq(String &out_freq)
{
#if HAS_NVAPI
  if (get_nv_physical_gpu())
  {
    carray<const char *, NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM> clockTypeStr = {"current", "base", "boost"};
    for (int clockType = 0; clockType < NV_GPU_CLOCK_FREQUENCIES_CLOCK_TYPE_NUM; clockType++)
    {
      NV_GPU_CLOCK_FREQUENCIES_V2 freq = {0};
      freq.version = NV_GPU_CLOCK_FREQUENCIES_VER_2;
      freq.ClockType = clockType;
      NvAPI_GPU_GetAllClockFrequencies(get_nv_physical_gpu(), &freq);
      out_freq += String(0, "%s%s: ", clockType == 0 ? "" : ", ", clockTypeStr[clockType]);
      bool first = true;
      for (int domainNo = 0; domainNo < NVAPI_MAX_GPU_PUBLIC_CLOCKS; domainNo++)
      {
        if (freq.domain[domainNo].bIsPresent)
        {
          out_freq += String(0, "%s%d MHz", first ? "" : " / ", freq.domain[domainNo].frequency / 1000);
          first = false;
        }
      }
    }
    return true;
  }
#endif
  return false;
}

int d3d::get_gpu_temperature()
{
#if HAS_NVAPI
  if (get_nv_physical_gpu())
  {
    d3d::driver_command(DRV3D_COMMAND_ACQUIRE_OWNERSHIP, NULL, NULL, NULL);

    NV_GPU_THERMAL_SETTINGS thermalSettings = {0};
    thermalSettings.version = NV_GPU_THERMAL_SETTINGS_VER;
    NvAPI_Status status = NvAPI_GPU_GetThermalSettings(get_nv_physical_gpu(), NVAPI_THERMAL_TARGET_ALL, &thermalSettings);

    d3d::driver_command(DRV3D_COMMAND_RELEASE_OWNERSHIP, NULL, NULL, NULL);

    if (status == NVAPI_OK)
    {
      int temperature = 0;
      for (unsigned int sensorNo = 0; sensorNo < NVAPI_MAX_THERMAL_SENSORS_PER_GPU; sensorNo++)
      {
        if (thermalSettings.sensor[sensorNo].controller > NVAPI_THERMAL_CONTROLLER_NONE &&
            thermalSettings.sensor[sensorNo].currentTemp > temperature)
        {
          temperature = thermalSettings.sensor[sensorNo].currentTemp;
        }
      }

      return temperature;
    }
  }
#endif
  return 0;
}
#else
void d3d::get_video_vendor_str(String &) {}
bool d3d::get_gpu_freq(String &) { return false; }
int d3d::get_gpu_temperature() { return 0; }
#endif
