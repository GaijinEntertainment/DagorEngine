// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "driverVersion.inc.cpp"

#include "gpuVendorAmd.h"
#include "gpuVendorNvidia.h"

#if _TARGET_PC_WIN
#define INITGUID
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <d3d11.h>

#include "winapi_helpers.h"
#endif

#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_lock.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_miscApi.h>
#include <util/dag_string.h>
#include <util/dag_watchdog.h>
#include <generic/dag_carray.h>

#if _TARGET_ANDROID
#include <startup/dag_globalSettings.h>
#endif

#include <EASTL/optional.h>
#include <cstdio>


using namespace gpu;

static eastl::optional<GpuVendor> cached_gpu_vendor;
static uint32_t cached_gpu_device_id = -1;
static char cached_gpu_desc[2048] = "";
static uint32_t cached_gpu_drv_ver[4] = {0, 0, 0, 0};
static DagorDateTime cached_gpu_drv_date = {0};
static uint32_t cached_gpu_video_memory_mb = 0;
static uint32_t cached_gpu_free_video_memory_mb = 0;

#if _TARGET_PC_WIN

static GpuVendor get_active_gpu_vendor_pc(String &out_active_gpu_description, uint32_t &out_video_memory_mb,
  uint32_t &out_free_video_memory_mb, uint32_t drv_ver[4], DagorDateTime &drv_date, uint32_t &device_id)
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4191) // 'type cast' : unsafe conversion from 'FARPROC'
#endif

  debug("get_active_gpu_vendor...");

  GpuVendor vendor = GpuVendor::UNKNOWN;
  out_active_gpu_description = "";
  out_video_memory_mb = 2048; // Fallback value to avoid quality adjustment on descrete GPU with unknown memory.
  out_free_video_memory_mb = 0;
  int sharedMemoryMb = 0;

  // loading all the dlls, drivers and such, may take a while on slow system and cause freeze detection to kick in
  watchdog_kick();

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
        D3D11_SDK_VERSION, &device, nullptr, &context);
      if (SUCCEEDED(hr))
      {
        LUID luid{};
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
            luid = adapterDesc.AdapterLuid;

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
            for (uint32_t outputIndex = 0;; outputIndex++)
            {
              IDXGIOutput *output;
              if (FAILED(adapter->EnumOutputs(outputIndex, &output)))
              {
                if (outputIndex == 0)
                  debug("No outputs found for adapter <%s>", deviceName);
                break;
              }

              DXGI_OUTPUT_DESC outDesc;
              output->GetDesc(&outDesc);
              char name[256];
              wcstombs(name, outDesc.DeviceName, 256);
              debug("output:%d: running on <%s>", outputIndex, name);
            }
            adapter->Release();
          }
          pDXGIDevice->Release();
        }

        LibPointer amdLib;
#if HAS_NVAPI
        if (init_nvapi() &&                                                    // Always initializes if NV GPU is present.
            NVAPI_OK == NvAPI_D3D11_SetDepthBoundsTest(device, 1, 0.f, 1.f) && // Succeeds only if NV GPU is active.
            NVAPI_OK == NvAPI_D3D11_SetDepthBoundsTest(device, 0, 0.f, 1.f))
        {
          vendor = GpuVendor::NVIDIA;
        }
        else
#endif
        {
#if HAS_AMD_DX_EXT
          amdLib.reset(LoadLibraryA(amd_lib_name));
          if (amdLib)
          {
            auto amdDxExtCreate = reinterpret_cast<PFNAmdDxExtCreate11>(GetProcAddress(amdLib.get(), "AmdDxExtCreate11"));
            if (amdDxExtCreate)
            {
              IAmdDxExt *amdExtension;
              HRESULT hr = amdDxExtCreate(device, &amdExtension); // Succeeds only if ATI GPU is active.
              if (SUCCEEDED(hr) && amdExtension)

              {
                vendor = GpuVendor::AMD;
                amdExtension->Release();
              }
            }
          }
#endif
        }

        if (!get_driver_info_from_luid(luid, drv_ver, drv_date))
        {
          // Do it while the device is created and the dlls are loaded.
          get_driver_info_from_dlls(vendor_dll_names[eastl::to_underlying(vendor)], drv_ver, drv_date);
        }

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

  if (vendor == GpuVendor::NVIDIA)
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

#endif

static GpuVendor guess_gpu_vendor(String *out_gpu_desc, uint32_t *out_drv_ver, DagorDateTime *out_drv_date, uint32_t *device_id,
  uint32_t *out_gpu_video_memory_mb)
{
  if (!cached_gpu_vendor.has_value())
  {
    memset(cached_gpu_desc, 0, sizeof(cached_gpu_desc));
    memset(cached_gpu_drv_ver, 0, sizeof(cached_gpu_drv_ver));
    memset(&cached_gpu_drv_date, 0, sizeof(cached_gpu_drv_date));

#if _TARGET_PC_WIN
    // Detect dynamically switchable graphics.
    String activeGpuDescription;
    cached_gpu_vendor = get_active_gpu_vendor_pc(activeGpuDescription, cached_gpu_video_memory_mb, cached_gpu_free_video_memory_mb,
      cached_gpu_drv_ver, cached_gpu_drv_date, cached_gpu_device_id);
    SNPRINTF(cached_gpu_desc, sizeof(cached_gpu_desc), "%s", activeGpuDescription.str());

#elif _TARGET_C1


#elif _TARGET_C2


#elif _TARGET_XBOX
    cached_gpu_vendor = GpuVendor::AMD;
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
      int vendorIdx = d3d::driver_command(Drv3dCommand::GET_VENDOR, &devName, &drvVer, &ctxInfo);
      if (vendorIdx >= 0 && vendorIdx < GPU_VENDOR_COUNT)
      {
        cached_gpu_vendor = static_cast<GpuVendor>(vendorIdx);
      }
      SNPRINTF(cached_gpu_desc, sizeof(cached_gpu_desc), "%s [%s]", devName ? devName : "Unknown", ctxInfo ? ctxInfo : "None");
      memcpy(cached_gpu_drv_ver, drvVer ? drvVer : unknownDrvVer, sizeof(cached_gpu_drv_ver));
    }
    else
    {
      cached_gpu_vendor.reset();
    }
#else
    cached_gpu_vendor = GpuVendor::UNKNOWN;
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
  if (device_id)
    *device_id = cached_gpu_device_id;
  return cached_gpu_vendor.has_value() ? *cached_gpu_vendor : GpuVendor::UNKNOWN;
}

#if !_TARGET_PC_MACOSX && !_TARGET_TVOS && !_TARGET_IOS
GpuVendor d3d::guess_gpu_vendor(String *out_gpu_desc, uint32_t *out_drv_ver, DagorDateTime *out_drv_date, uint32_t *device_id)
{
  return guess_gpu_vendor(out_gpu_desc, out_drv_ver, out_drv_date, device_id, nullptr);
}
#elif DAGOR_HOSTED_INTERNAL_SERVER
GpuVendor d3d::guess_gpu_vendor(String *, uint32_t *, DagorDateTime *, uint32_t *) { return {}; }
#endif

DagorDateTime d3d::get_gpu_driver_date(GpuVendor vendor)
{
  DagorDateTime drv_date{};
#if _TARGET_PC_WIN
  uint32_t dummy[4];
  get_driver_info_from_dlls(vendor_dll_names[eastl::to_underlying(vendor)], dummy, drv_date); // Do it while the device is created and
                                                                                              // the dlls are loaded.
#endif
  return drv_date;
}

#if _TARGET_PC_WIN

static void get_dedicated_gpu_memory_total_kb_dxgi(unsigned &out_dedicated, unsigned &out_dedicated_free)
{
  if (!cached_gpu_vendor.has_value())
    guess_gpu_vendor(nullptr, nullptr, nullptr, nullptr, nullptr);

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

  return d3d::driver_command(Drv3dCommand::GET_VIDEO_MEMORY_BUDGET, (void *)&out_dedicated, (void *)&out_dedicated_free);
}

#if DAGOR_HOSTED_INTERNAL_SERVER
unsigned d3d::get_dedicated_gpu_memory_size_kb() { return 0; }
unsigned d3d::get_free_dedicated_gpu_memory_size_kb() { return 0; }

#elif _TARGET_PC_WIN | _TARGET_PC_LINUX

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
    get_video_amd_str(video_vendor_info_str);
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
    NV_GPU_THERMAL_SETTINGS thermalSettings = {0};
    const NvAPI_Status status = [&] {
      d3d::GpuAutoLock gpuLock;
      thermalSettings.version = NV_GPU_THERMAL_SETTINGS_VER;
      return NvAPI_GPU_GetThermalSettings(get_nv_physical_gpu(), NVAPI_THERMAL_TARGET_ALL, &thermalSettings);
    }();

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
