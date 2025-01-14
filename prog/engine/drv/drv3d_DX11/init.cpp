// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdarg.h>

#include <util/dag_globDef.h>
#include <util/dag_watchdog.h>
#include <debug/dag_debug.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_threads.h>

#include "driver.h"
#if HAS_NVAPI
#include <nvapi.h>
#endif
#include "streamline_adapter.h"
#if HAS_GF_AFTERMATH
#include <include/GFSDK_Aftermath.h>
#endif
#include <AmdDxExtDepthBoundsApi.h>
#include <dxgi1_6.h>
#include <d3d11_4.h>
#include <RenderDoc/renderdoc_app.h>
#include "texture.h"
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_res.h>
#include <drv/3d/dag_platform_pc.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_resetDevice.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_shaderLibrary.h>
#include <3d/dag_nvLowLatency.h>
#include <startup/dag_globalSettings.h>
#include <supp/exportType.h>
#include <supp/dag_cpuControl.h>
#include <util/dag_string.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_unicode.h>
#include <debug/dag_logSys.h>
#include <perfMon/dag_cpuFreq.h>
#include <math/integer/dag_IPoint2.h>
#include <stdlib.h>
#include <osApiWrappers/dag_progGlobals.h>
#include <osApiWrappers/dag_miscApi.h>
#include <drv_utils.h>
#include <gpuConfig.h>
#include <util/dag_parseResolution.h>
#include <util/dag_oaHashNameMap.h>
#include <util/dag_delayedAction.h>
#include <osApiWrappers/dag_messageBox.h>
#include <util/dag_localization.h>
#include <nvLowLatency.h>
#include <3d/dag_lowLatency.h>
#include <util/dag_finally.h>
#include <validationLayer.h>
#include <destroyEvent.h>
#include "resource_size_info.h"

#include "helpers.h"
#include "../drv3d_commonCode/dxgi_utils.h"
#include "../drv3d_commonCode/stereoHelper.h"

#pragma comment(lib, "dxguid.lib")

#ifndef _FACD3D
#define _FACD3D 0x876
#endif
#ifndef MAKE_D3DHRESULT
#define MAKE_D3DHRESULT(code) MAKE_HRESULT(1, _FACD3D, code)
#endif
#ifndef D3DERR_INVALIDCALL
#define D3DERR_INVALIDCALL MAKE_D3DHRESULT(2156)
#endif

#ifndef D3DERR_WASSTILLDRAWING
#define D3DERR_WASSTILLDRAWING MAKE_D3DHRESULT(540)
#endif

#define FREE_LIBRARY 0

extern void close_perf_analysis();
extern void shutdown_hlsl_d3dcompiler_internal();
void get_aftermath_status();

static constexpr int DEVICE_RESET_FLUSH_TIMEOUT_MS = 120000;
static constexpr int DEVICE_RESET_FLUSH_LONG_WAIT_MS = 10000;

static void dump_dll_version([[maybe_unused]] const char *file_name)
{
#if _TARGET_PC_WIN
  char fullFileName[MAX_PATH];
  DWORD len = SearchPathA(NULL, file_name, NULL, sizeof(fullFileName), fullFileName, NULL);

  if (len == 0)
    debug("'%s' not found.", file_name);
  else
  {
    DWORD dwProductVersionMS = 0;
    DWORD dwProductVersionLS = 0;
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeA(fullFileName, &handle);
    if (size)
    {
      BYTE *versionInfo = new BYTE[size];
      if (GetFileVersionInfoA(fullFileName, handle, size, versionInfo))
      {
        UINT len = 0;
        VS_FIXEDFILEINFO *vsfi = NULL;
        if (VerQueryValueW(versionInfo, L"\\", (void **)&vsfi, &len))
        {
          dwProductVersionMS = vsfi->dwProductVersionMS;
          dwProductVersionLS = vsfi->dwProductVersionLS;
        }
      }
      delete[] versionInfo;
    }

    SYSTEMTIME date;
    memset(&date, 0, sizeof(date));
    HANDLE dllFileHandle = CreateFile(fullFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (dllFileHandle != INVALID_HANDLE_VALUE)
    {
      FILETIME fileTimeWrite;
      if (GetFileTime(dllFileHandle, NULL, NULL, &fileTimeWrite))
        FileTimeToSystemTime(&fileTimeWrite, &date);
      CloseHandle(dllFileHandle);
    }

    debug("%02d-%02d-%02d %d.%d.%d.%d %s", date.wYear, date.wMonth, date.wDay, HIWORD(dwProductVersionMS), LOWORD(dwProductVersionMS),
      HIWORD(dwProductVersionLS), LOWORD(dwProductVersionLS), fullFileName);
  }
#endif
}

// globals
namespace drv3d_dx11
{

#if _TARGET_PC_WIN && HAS_NVAPI
static char nv_device_name[NVAPI_SHORT_STRING_MAX] = {0};
#endif

bool init_nvapi()
{
#if _TARGET_PC_WIN && HAS_NVAPI
#if _TARGET_64BIT
  dump_dll_version("nvapi64.dll");
#else
  dump_dll_version("nvapi.dll");
#endif

  if (::dgs_get_settings()->getBlockByNameEx("directx")->getBool("disableNvapi", false))
  {
    debug("NVAPI disabled");
    is_nvapi_initialized = false;
    return false;
  }

  NvAPI_Status status = NvAPI_Initialize();

  is_nvapi_initialized = NVAPI_OK == status;

  if (is_nvapi_initialized)
  {
    debug("NVAPI: initialized");

    NV_DISPLAY_DRIVER_VERSION version = {0};
    version.version = NV_DISPLAY_DRIVER_VERSION_VER;
    status = NvAPI_GetDisplayDriverVersion(NVAPI_DEFAULT_HANDLE, &version);
    if (status == NVAPI_OK)
    {
      strncpy(nv_device_name, "NVIDIA ", sizeof(nv_device_name));
      nv_device_name[sizeof(nv_device_name) - 1] = 0;
      strncat(nv_device_name, version.szAdapterString, sizeof(nv_device_name) - strlen(nv_device_name) - 1);
    }
    else
    {
      debug("NVAPI: GetDisplayDriverVersion failed, error %d\n", status);
      is_nvapi_initialized = false;
    }
  }
  else
    debug("NVAPI: Init failed, error %d\n", status);
#endif

  return is_nvapi_initialized;
}


void set_nvapi_vsync(IUnknown *device)
{
  // TODO Update the adaptive sync handling on UI and here as well
  // The NvAPI_D3D_SetVerticalSyncMode api call is no longer supported by the nvidia drivers
  // and with the latest NvAPI it doesn't compile anymore
  (void)device;
}

IDXGI_FACTORY *dx11_DXGIFactory = NULL;

static HMODULE rdoc_dllh = NULL;
static HMODULE dx11_dllh = NULL;
RENDERDOC_API_1_5_0 *docAPI = nullptr;
D3D_FEATURE_LEVEL featureLevelsSupported = D3D_FEATURE_LEVEL_10_0;
bool d3d_inited = false;
static bool _in_win_started = false;
bool _no_vsync = false;
bool own_window = false;
bool window_occlusion_check_enabled = true;
bool flush_on_present = false;
bool flush_before_survey = false;
bool use_gpu_dt = true;
bool use_dxgi_present_test = true;
bool immutable_textures = true;
bool resetting_device_now = false;
HRESULT device_is_lost = S_OK;
unsigned texture_sysmemcopy_usage = 0, vbuffer_sysmemcopy_usage = 0, ibuffer_sysmemcopy_usage = 0;
static DXGI_ADAPTER_DESC adapterDesc;
bool ignore_resource_leaks_on_exit = false;
bool view_resizing_related_logging_enabled = true;
bool hdr_enabled = false;
bool command_list_wa = false;
bool use_wait_object = true;
eastl::optional<MemoryMetrics> memory_metrics;

HWND main_window_hwnd = NULL;
HWND render_window_hwnd = NULL;
HWND secondary_window_hwnd = NULL;
main_wnd_f *main_window_proc = NULL;
WNDPROC origin_window_proc = NULL;
intptr_t gpuThreadId = 0;
int gpuAcquireRefCount = 0;
bool use_tearing = false;
double vsync_refresh_rate = 0;
extern const char *dxgi_format_to_string(DXGI_FORMAT format);

__declspec(thread) HRESULT last_hres = S_OK;

// multi-threaded access support (required for background loading)
static CritSecStorage d3d_global_lock;

// device state
IDXGI_SWAP_CHAIN *swap_chain = NULL, *secondary_swap_chain = NULL;
ID3D11_DEV *dx_device = NULL;
ID3D11_DEV1 *dx_device1 = NULL;
ID3D11_DEV3 *dx_device3 = NULL;
ID3D11_DEVCTX *dx_context = NULL;
ID3D11_DEVCTX1 *dx_context1 = NULL;
ID3D11InfoQueue *pInfoQueue = nullptr;
os_spinlock_t dx_context_cs;
WinCritSec dx_res_cs;
SmartReadWriteFifoLock reset_rw_lock;
IDXGIOutput *target_output = NULL;
Texture *secondary_backbuffer_color_tex = NULL;
static char deviceName[128] = "unknown";
bool is_nvapi_initialized = false;
int default_display_index = 0;
HMODULE hAtiDLL = NULL;
IAmdDxExt *amdExtension = nullptr;
IAmdDxExtDepthBounds *amdDepthBoundsExtension = nullptr;
bool nv_dbt_supported = false;
bool ati_dbt_supported = false;
IPoint2 resolution(0, 0);
uint8_t override_max_anisotropy_level = 0;
DXGI_SWAP_CHAIN_DESC scd = {0};
bool used_flip_model_before = false;
bool conditional_render_enabled = false;
StreamlineAdapter::InterposerHandleType streamlineInterposer = {nullptr, nullptr};
eastl::optional<StreamlineAdapter> streamlineAdapter;
HandlePointer waitableObject;

RenderState g_render_state;
DriverState g_driver_state;

// Backbuffer backBufferRenderTarget;

float screen_aspect_ratio = 1.f;

DXGI_GAMMA_CONTROL gamma_control;
bool gamma_control_valid = false;

extern eastl::vector<DeviceResetEventHandler *> deviceResetEventHandlers;

extern void save_predicates();
extern void recreate_predicates();
extern void gather_textures_to_recreate(FramememResourceSizeInfoCollection &);
extern void gather_buffers_to_recreate(FramememResourceSizeInfoCollection &);
extern void recreate_texture(uint32_t index);
extern void recreate_buffer(uint32_t index);
extern void print_memory_stat();

float gpu_frame_time = -1.f;
LARGE_INTEGER qpc_freq = {{1, 1}};

#define MAXIMUM_FRAME_LATENCY 2 // The value is typically 3, but can range from 1 to 16.
#define GPU_THREAD_PRIORITY   3 // The thread priority, ranging from -7 to 7.

DriverDesc g_device_desc = {
  0, 0, // int zcmpfunc,acmpfunc;
  0, 0, // int sblend,dblend;

  1, 1,            // int mintexw,mintexh;
  16384, 16384,    // int maxtexw,maxtexh;
  1, 16384,        // int mincubesize,maxcubesize;
  1, 16384,        // int minvolsize,maxvolsize;
  0,               // int maxtexaspect; ///< 1 means texture should be square, 0 means no limit
  65536,           // int maxtexcoord;
  MAX_RESOURCES,   // int maxsimtex;
  MAX_VS_SAMPLERS, // int maxlights;
  0,               // int maxclipplanes;
  64,              // int maxstreams;
  64,              // int maxstreamstr;
  1024,            // int maxvpconsts;


  65536 * 4, 65536 * 4,                                                              // int maxprims,maxvertind;
  0.0f, 0.0f,                                                                        // float upixofs,vpixofs;
  min<int>(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, Driver3dRenderTarget::MAX_SIMRT), // int maxSimRT;
  true,                                                                              // bool is20ArbitrarySwizzleAvailable;
  32,                                                                                // minWarpSize
  64,                                                                                // maxWarpSize

  /*  0,
  16,
  1.0f*/
};

static void flush_driver_errors()
{
  if (!pInfoQueue)
    return;
  uint32_t message_count = pInfoQueue->GetNumStoredMessages();

  for (uint32_t i = 0; i < message_count; i++)
  {
    SIZE_T messageLength = 0;
    pInfoQueue->GetMessage(0, NULL, &messageLength);
    eastl::unique_ptr<uint8_t[]> messageData = eastl::make_unique<uint8_t[]>(messageLength);
    D3D11_MESSAGE *pMessage = (D3D11_MESSAGE *)messageData.get();
    pInfoQueue->GetMessage(0, pMessage, &messageLength);
    D3D_ERROR("ID: %d Message: %.*s", pMessage->ID, pMessage->DescriptionByteLength, pMessage->pDescription);
  }

  pInfoQueue->ClearStoredMessages();
}

static void debug_layer_report_fatal_error(const char *title, const char *msg, const char *call_stack)
{
  flush_driver_errors();
  flush_debug_file();
  fflush(stdout);
  fflush(stderr);
  _exit(13);
}

void acquireD3dOwnership()
{
  ::enter_critical_section(d3d_global_lock);
  gpuThreadId = ::GetCurrentThreadId();
  gpuAcquireRefCount++;
}
void releaseD3dOwnership()
{
  gpuAcquireRefCount--;
  if (!gpuAcquireRefCount)
  {
    d3d::setvsrc(0, NULL, 0); // Avoid accidental reuse of random buffers.
    d3d::setind(NULL);
    gpuThreadId = 0;
  }
  ::leave_critical_section(d3d_global_lock);
}

const char *dx11_reset_error(HRESULT hr)
{
  switch (hr)
  {
    case DXGI_ERROR_DEVICE_REMOVED:
      switch (dx_device->GetDeviceRemovedReason())
      {
        case DXGI_ERROR_DEVICE_HUNG: return "DEVICE_REMOVED-DXGI_ERROR_DEVICE_HUNG";
        case DXGI_ERROR_DEVICE_REMOVED: return "DEVICE_REMOVED-DXGI_ERROR_DEVICE_REMOVED";
        case DXGI_ERROR_DEVICE_RESET: return "DEVICE_REMOVED-DXGI_ERROR_DEVICE_RESET";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DEVICE_REMOVED-DXGI_ERROR_DRIVER_INTERNAL_ERROR";
        case DXGI_ERROR_INVALID_CALL: return "DEVICE_REMOVED-DXGI_ERROR_INVALID_CALL";
        case E_OUTOFMEMORY: return "DEVICE_REMOVED-E_OUTOFMEMORY";
        case S_OK: return "DEVICE_REMOVED-S_OK";
        default: return "DEVICE_REMOVED-unknown";
      }
    case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    default: return NULL;
  }
}

bool device_should_reset(HRESULT hr, const char *text)
{
  if (device_is_lost != S_OK)
  {
    dagor_d3d_force_driver_reset = true;
    logwarn("dx11 error: %s hr=0x%X called, while in driver reset", text, device_is_lost);
    return true;
  }
  if (SUCCEEDED(hr))
    return false;
  const char *errorText = dx11_reset_error(hr);
  HRESULT testHr = S_OK;
  if (!errorText)
  {
    {
      ContextAutoLock contextLock;
      testHr = dx_device->GetDeviceRemovedReason();
    }
    if (FAILED(testHr))
      errorText = dx11_reset_error(DXGI_ERROR_DEVICE_REMOVED);
    else
    {
      {
        ContextAutoLock contextLock;
        testHr = swap_chain->Present(_no_vsync ? 0 : 1, DXGI_PRESENT_TEST);
      }
      errorText = dx11_reset_error(testHr);
    }
    if (!errorText)
      return false;
  }
  dagor_d3d_force_driver_reset = true;
  device_is_lost = hr;
  get_aftermath_status();
  debug("Vendor: %s, device id: 0x%X, device name: %s, driver version: %d.%d.%d.%d",
    d3d_get_vendor_name(get_gpu_driver_cfg().primaryVendor), get_gpu_driver_cfg().deviceId, deviceName,
    get_gpu_driver_cfg().driverVersion[0], get_gpu_driver_cfg().driverVersion[1], get_gpu_driver_cfg().driverVersion[2],
    get_gpu_driver_cfg().driverVersion[3]);
  logwarn("dx11 error: %s hr=0x%X testHr=0x%X (%s) causes forced driver reset\n", text, hr, testHr, errorText);
  debug_dump_stack();
  return true;
}
const char *dx11_error(HRESULT hr)
{
  switch (hr)
  {
    case D3D11_ERROR_FILE_NOT_FOUND: return "D3D11_ERROR_FILE_NOT_FOUND";
    case D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_STATE_OBJECTS";
    case D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS: return "D3D11_ERROR_TOO_MANY_UNIQUE_VIEW_OBJECTS";
    case D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD: return "D3D11_ERROR_DEFERRED_CONTEXT_MAP_WITHOUT_INITIAL_DISCARD";
    case D3DERR_INVALIDCALL: return "D3DERR_INVALIDCALL";
    case D3DERR_WASSTILLDRAWING: return "D3DERR_WASSTILLDRAWING";
    case E_FAIL: return "E_FAIL";
    case E_INVALIDARG: return "E_INVALIDARG";
    case E_OUTOFMEMORY: return "E_OUTOFMEMORY";
    case S_FALSE: return "S_FALSE";
    case S_OK: return "S_OK";

    case DXGI_ERROR_DEVICE_REMOVED:
      if (!dx_device)
        return "DEVICE_REMOVED-uninitialized";
      switch (dx_device->GetDeviceRemovedReason())
      {
        case DXGI_ERROR_DEVICE_HUNG: return "DEVICE_REMOVED-DXGI_ERROR_DEVICE_HUNG";
        case DXGI_ERROR_DEVICE_REMOVED: return "DEVICE_REMOVED-DXGI_ERROR_DEVICE_REMOVED";
        case DXGI_ERROR_DEVICE_RESET: return "DEVICE_REMOVED-DXGI_ERROR_DEVICE_RESET";
        case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DEVICE_REMOVED-DXGI_ERROR_DRIVER_INTERNAL_ERROR";
        case DXGI_ERROR_INVALID_CALL: return "DEVICE_REMOVED-DXGI_ERROR_INVALID_CALL";
        case E_OUTOFMEMORY: return "DEVICE_REMOVED-E_OUTOFMEMORY";
        case S_OK: return "DEVICE_REMOVED-S_OK";
        default: return "DEVICE_REMOVED-unknown";
      }

    case DXGI_ERROR_DEVICE_HUNG: return "DXGI_ERROR_DEVICE_HUNG";
    case DXGI_ERROR_DEVICE_RESET: return "DXGI_ERROR_DEVICE_RESET";
    case DXGI_ERROR_DRIVER_INTERNAL_ERROR: return "DXGI_ERROR_DRIVER_INTERNAL_ERROR";
    case DXGI_ERROR_FRAME_STATISTICS_DISJOINT: return "DXGI_ERROR_FRAME_STATISTICS_DISJOINT";
    case DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE: return "DXGI_ERROR_GRAPHICS_VIDPN_SOURCE_IN_USE";
    case DXGI_ERROR_INVALID_CALL: return "DXGI_ERROR_INVALID_CALL";
    case DXGI_ERROR_MORE_DATA: return "DXGI_ERROR_MORE_DATA";
    case DXGI_ERROR_NONEXCLUSIVE: return "DXGI_ERROR_NONEXCLUSIVE";
    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE: return "DXGI_ERROR_NOT_CURRENTLY_AVAILABLE";
    case DXGI_ERROR_NOT_FOUND: return "DXGI_ERROR_NOT_FOUND";
    case DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED: return "DXGI_ERROR_REMOTE_CLIENT_DISCONNECTED";
    case DXGI_ERROR_REMOTE_OUTOFMEMORY: return "DXGI_ERROR_REMOTE_OUTOFMEMORY";
    case DXGI_ERROR_WAS_STILL_DRAWING: return "DXGI_ERROR_WAS_STILL_DRAWING";
    case DXGI_ERROR_UNSUPPORTED:
      return "DXGI_ERROR_UNSUPPORTED";
      // win8
      //       case DXGI_ERROR_ACCESS_LOST: return "DXGI_ERROR_ACCESS_LOST";
      //       case DXGI_ERROR_WAIT_TIMEOUT return "DXGI_ERROR_WAIT_TIMEOUT";
      //       case DXGI_ERROR_SESSION_DISCONNECTED: return "DXGI_ERROR_SESSION_DISCONNECTED";
      //       case DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE: return "DXGI_ERROR_RESTRICT_TO_OUTPUT_STALE";
      //       case DXGI_ERROR_CANNOT_PROTECT_CONTENT: return "DXGI_ERROR_CANNOT_PROTECT_CONTENT";
      //       case DXGI_ERROR_ACCESS_DENIED: return "DXGI_ERROR_ACCESS_DENIED";
  }
  return "unknown";
}


bool DriverState::createSurfaces(uint32_t screen_width, uint32_t screen_height)
{
  width = screen_width;
  height = screen_height;

  backBufferColorTex = create_backbuffer_tex(0, swap_chain);

  // could not work if not created
  G_ASSERT(backBufferColorTex);

  DEBUG_CTX("created backbuffer %p", backBufferColorTex);
  return true;
}

void disable_conditional_render_unsafe()
{
  if (conditional_render_enabled)
    dx_context->SetPredication(NULL, FALSE);
  conditional_render_enabled = false;
}
}; // namespace drv3d_dx11

using namespace drv3d_dx11;

#if HAS_GF_AFTERMATH
#include <ska_hash_map/flat_hash_map2.hpp>
#include <util/dag_hash.h>
#include <util/dag_simpleString.h>
static ska::flat_hash_map<uint32_t, SimpleString> aftermath_marker_names_map;

static HMODULE aftermath_dllh = NULL;
static PFN_GFSDK_Aftermath_SetEventMarker after_math_marker = NULL;
static PFN_GFSDK_Aftermath_GetData after_math_get_data = NULL;
static PFN_GFSDK_Aftermath_DX11_CreateContextHandle after_math_createcontexthandle = NULL;
static GFSDK_Aftermath_ContextHandle aftermath_context = NULL;
static PFN_GFSDK_Aftermath_ReleaseContextHandle aftermath_releasecontexthandle = NULL;
static PFN_GFSDK_Aftermath_GetDeviceStatus aftermath_getdevicestatus = NULL;

static void close_aftermath()
{
  aftermath_marker_names_map.clear();

  if (aftermath_dllh)
  {
    after_math_marker = NULL;
    after_math_get_data = NULL;
    after_math_createcontexthandle = NULL;
    if (aftermath_context)
      aftermath_releasecontexthandle(aftermath_context);
    aftermath_releasecontexthandle = NULL;
    aftermath_context = NULL;
    aftermath_getdevicestatus = NULL;
    FreeLibrary(aftermath_dllh);
    aftermath_dllh = NULL;
  }
}

static bool init_aftermath(ID3D11Device *device)
{
#if _TARGET_64BIT
  static const char *aftermath_fn = "GFSDK_Aftermath_Lib.x64.dll";
#else
  static const char *aftermath_fn = "GFSDK_Aftermath_Lib.x86.dll";
#endif
  aftermath_dllh = LoadLibraryA(aftermath_fn);
  if (!aftermath_dllh)
  {
    logwarn("Can't load NVIDIA AFTERMATH DLL \"%s\"", aftermath_fn);
    return false;
  }
  PFN_GFSDK_Aftermath_DX11_Initialize initAfterMath =
    (PFN_GFSDK_Aftermath_DX11_Initialize)GetProcAddress(aftermath_dllh, "GFSDK_Aftermath_DX11_Initialize");
  if (!initAfterMath)
  {
    close_aftermath();
    return false;
  }
  // Initialize Nsight Aftermath for this device.
  constexpr uint32_t aftermathFlags =                        //
    GFSDK_Aftermath_FeatureFlags_EnableMarkers |             // Enable event marker tracking.
    GFSDK_Aftermath_FeatureFlags_EnableResourceTracking |    // Enable tracking of resources.
    GFSDK_Aftermath_FeatureFlags_GenerateShaderDebugInfo |   // Generate debug information for shaders.
    GFSDK_Aftermath_FeatureFlags_EnableShaderErrorReporting; // Enable additional runtime shader error reporting.
  GFSDK_Aftermath_Result result = initAfterMath(GFSDK_Aftermath_Version_API, aftermathFlags, device);
  if (result != GFSDK_Aftermath_Result_Success)
  {
    logwarn("can't init aftermath 0x%X", result);
    close_aftermath();
    return false;
  }
  after_math_marker = (PFN_GFSDK_Aftermath_SetEventMarker)GetProcAddress(aftermath_dllh, "GFSDK_Aftermath_SetEventMarker");
  after_math_get_data = (PFN_GFSDK_Aftermath_GetData)GetProcAddress(aftermath_dllh, "GFSDK_Aftermath_GetData");
  after_math_createcontexthandle =
    (PFN_GFSDK_Aftermath_DX11_CreateContextHandle)GetProcAddress(aftermath_dllh, "GFSDK_Aftermath_DX11_CreateContextHandle");
  aftermath_releasecontexthandle =
    (PFN_GFSDK_Aftermath_ReleaseContextHandle)GetProcAddress(aftermath_dllh, "GFSDK_Aftermath_ReleaseContextHandle");
  aftermath_getdevicestatus = (PFN_GFSDK_Aftermath_GetDeviceStatus)GetProcAddress(aftermath_dllh, "GFSDK_Aftermath_GetDeviceStatus");
  if (!after_math_get_data || !after_math_marker || !after_math_createcontexthandle || !aftermath_releasecontexthandle ||
      !aftermath_getdevicestatus)
  {
    logwarn("can't init aftermath - no functions");
    close_aftermath();
    return false;
  }

  result = after_math_createcontexthandle(dx_context, &aftermath_context);
  if (result != GFSDK_Aftermath_Result_Success)
  {
    logwarn("can't get aftermath context 0x%X", result);
    close_aftermath();
    return false;
  }

  debug("aftermath enabled");
  return true;
}

bool aftermath_inited() { return aftermath_dllh != NULL; }

void set_aftermath_marker(const char *name, bool allocate_copy)
{
  if (!after_math_marker)
    return;
  if (!name || !name[0])
    return;

  // name should be a static string that will be valid after the crash.
  const char *myName = name;
  if (allocate_copy)
  {
    const uint32_t strHash = str_hash_fnv1(name);
    auto it = aftermath_marker_names_map.find(strHash);
    if (it != aftermath_marker_names_map.end())
      myName = it->second.c_str();
    else
      myName = aftermath_marker_names_map.emplace(strHash, SimpleString(myName)).first->second.c_str();
  }

  ContextAutoLock lock;
  if (after_math_marker)
  {
    after_math_marker(aftermath_context, (void *)myName, 0);
  }
}

void get_aftermath_status()
{
  if (after_math_get_data)
  {
    GFSDK_Aftermath_ContextData contextData;
    after_math_get_data(1, &aftermath_context, &contextData);
    debug("%d: gpu went down at %s (0x%08X)", contextData.status, contextData.status == 3 ? "" : (const char *)contextData.markerData,
      (contextData.status == GFSDK_Aftermath_Context_Status_Invalid) ? (GFSDK_Aftermath_Result)(uintptr_t)contextData.markerData
                                                                     : GFSDK_Aftermath_Result_Success);

    GFSDK_Aftermath_Device_Status status;
    aftermath_getdevicestatus(&status);
    debug("status= %d", status);
  }
}
#else
bool aftermath_inited() { return false; }
void set_aftermath_marker(const char *, bool) {}
void get_aftermath_status() {}
#endif

void init_memory_report()
{
#if DEBUG_MEMORY_METRICS
  G_ASSERT_RETURN(dx_device, );
  memory_metrics = MemoryMetrics(dx_device);
  memory_metrics->registerReportCallback();
#endif
}

void create_dlss_feature(IPoint2 output_resolution)
{
  streamlineAdapter->initializeDlssState();

  if (stereo_config_callback && stereo_config_callback->desiredStereoRender())
  {
    auto size = stereo_config_callback->desiredRendererSize();
    for (int viewIdx : {0, 1})
      streamlineAdapter->createDlssFeature(viewIdx, {size.width, size.height}, dx_context);
  }
  else
    streamlineAdapter->createDlssFeature(0, output_resolution, dx_context);

  if (streamlineAdapter->isDlssGSupported() == nv::SupportState::Supported)
    streamlineAdapter->createDlssGFeature(0, dx_context);
}

void release_dlss_feature()
{
  streamlineAdapter->releaseDlssFeature(0);
  if (stereo_config_callback && stereo_config_callback->desiredStereoRender())
    streamlineAdapter->releaseDlssFeature(1);

  if (streamlineAdapter->isDlssGSupported() == nv::SupportState::Supported)
    streamlineAdapter->releaseDlssGFeature(0);
}

static bool is_tearing_supported()
{
  HMODULE dxgi_dll = LoadLibraryA("dxgi.dll");
  FINALLY([=] { FreeLibrary(dxgi_dll); });
  using CreateDXGIFactory1Func = HRESULT(WINAPI *)(REFIID, void **);
  CreateDXGIFactory1Func pCreateDXGIFactory1 = (CreateDXGIFactory1Func)GetProcAddress(dxgi_dll, "CreateDXGIFactory1");
  ComPtr<IDXGIFactory5> factory;
  HRESULT hr = pCreateDXGIFactory1(COM_ARGS(&factory));
  BOOL allowTearing = FALSE;
  if (SUCCEEDED(hr))
  {
    hr = factory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
  }
  return SUCCEEDED(hr) && allowTearing;
}

static bool is_flip_model(DXGI_SWAP_EFFECT swap_effect)
{
  return swap_effect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL || swap_effect == DXGI_SWAP_EFFECT_FLIP_DISCARD;
}

static HRESULT create_waitable_object()
{
  IDXGISwapChain2 *swapChain2;
  const auto hr = swap_chain->QueryInterface(__uuidof(IDXGISwapChain2), (void **)&swapChain2);
  if (SUCCEEDED(hr) && use_wait_object)
  {
    waitableObject.reset(swapChain2->GetFrameLatencyWaitableObject());
    swapChain2->SetMaximumFrameLatency(MAXIMUM_FRAME_LATENCY);
    swapChain2->Release();
  }
  return hr;
}

static HRESULT recreate_swapchain(DXGI_SWAP_CHAIN_DESC &new_scd)
{
  waitableObject.reset();
  swap_chain->SetFullscreenState(FALSE, NULL);
  swap_chain->Release();

  DEBUG_CTX("deleted backbuffer %p", g_driver_state.backBufferColorTex);
  del_d3dres(g_driver_state.backBufferColorTex);

  HRESULT hres;
  IDXGIDevice *pDXGIDevice = NULL;
  hres = dx_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
  G_ASSERT(SUCCEEDED(hres));

  IDXGIAdapter1 *pDXGIAdapter;
  hres = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter1), (void **)&pDXGIAdapter);
  G_ASSERT(SUCCEEDED(hres));

  IDXGIFactory1 *pIDXGIFactory;
  hres = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void **)&pIDXGIFactory);
  G_ASSERT(SUCCEEDED(hres));

  hres = pIDXGIFactory->CreateSwapChain(dx_device, &new_scd, &swap_chain);
  G_ASSERT(SUCCEEDED(hres));
  if (SUCCEEDED(hres))
    scd = new_scd;

  if (scd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
    create_waitable_object();

  return hres;
}

#define FEATURE_LEVELS_LIST X(10_0) X(10_1) X(11_0) X(11_1) X(12_0) X(12_1)

D3D_FEATURE_LEVEL feature_level_from_str(const char *str)
{
#define X(level)                      \
  if (strcmp(#level, str) == 0)       \
  {                                   \
    return D3D_FEATURE_LEVEL_##level; \
  }
  FEATURE_LEVELS_LIST
#undef X
  logerr("unknown feature level %s, returning 10_0", str);
  return D3D_FEATURE_LEVEL_10_0;
}

const char *str_from_feature_level(D3D_FEATURE_LEVEL level)
{
#define X(level) \
  case D3D_FEATURE_LEVEL_##level: return #level;
  switch (level) //-V719
  {
    FEATURE_LEVELS_LIST
  }
#undef X
  logerr("unknown feature level %x, returning 10_0", level);
  return "10_0";
}

void compare_feature_levels(D3D_FEATURE_LEVEL old_level, D3D_FEATURE_LEVEL new_level)
{
  if (old_level != new_level)
  {
    logerr("DX feature level changed from %s to %s. This will likely result in a crash.", str_from_feature_level(old_level),
      str_from_feature_level(new_level));
  }
}

void find_closest_matching_mode()
{
  vsync_refresh_rate = 0;
  IDXGIOutput *dxgiOutput;
  HRESULT hr = swap_chain->GetContainingOutput(&dxgiOutput);

  if (SUCCEEDED(hr))
  {
    DXGI_MODE_DESC newModeDesc = scd.BufferDesc; // Maximum monitor refresh, not rounded, exact value requred for flip to work with
                                                 // vsync.
    hr = dxgiOutput->FindClosestMatchingMode(&scd.BufferDesc, &newModeDesc, dx_device);
    // change mode only for fullscreen
    if (SUCCEEDED(hr) && dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
    {
      scd.BufferDesc = newModeDesc;
      swap_chain->ResizeTarget(&scd.BufferDesc);
    }
    dxgiOutput->Release();

    vsync_refresh_rate = (double)newModeDesc.RefreshRate.Numerator / (double)max(newModeDesc.RefreshRate.Denominator, 1u);
  }
}

bool init_device(Driver3dInitCallback *cb, HWND window_hwnd, int screen_wdt, int screen_hgt, int screen_bpp,
  const char *opt_display_name, int64_t opt_adapter_luid, const char *window_class_name, bool after_reset = false)
{
  FloatingPointExceptionsKeeper fkeeper;

#if DAGOR_DBGLEVEL > 0
  const DataBlock &debugBlk = *::dgs_get_settings()->getBlockByNameEx("debug");
  if (debugBlk.getBool("loadRenderDoc", false))
  {
    rdoc_dllh = LoadLibraryA("renderdoc.dll");
    if (!rdoc_dllh)
    {
      D3D_ERROR("Could not load renderdoc.dll");
    }
  }

  view_resizing_related_logging_enabled = debugBlk.getBool("view_resizing_related_logging_enabled", true);
#endif

  ZeroMemory(&adapterDesc, sizeof(adapterDesc));
  if (!resetting_device_now)
  {
    static const char *dx11_fn = "d3d11.dll";
    dump_dll_version(dx11_fn);
    dump_dll_version("dxgi.dll");
    dx11_dllh = LoadLibraryA(dx11_fn);
    if (!dx11_dllh)
    {
      D3D_ERROR("Can't load DX11 DLL \"%s\"", dx11_fn);
      return false;
    }
  }

  ::QueryPerformanceFrequency(&qpc_freq);

  streamlineInterposer = StreamlineAdapter::loadInterposer();
  PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN create_dx11_device = nullptr;
  StreamlineAdapter::init(streamlineAdapter, StreamlineAdapter::RenderAPI::DX11);
  if (streamlineAdapter)
    create_dx11_device = (PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN)StreamlineAdapter::getInterposerSymbol(streamlineInterposer,
      "D3D11CreateDeviceAndSwapChain");
  else
    create_dx11_device = (PFN_D3D11_CREATE_DEVICE_AND_SWAP_CHAIN)GetProcAddress(dx11_dllh, "D3D11CreateDeviceAndSwapChain");

  if (!create_dx11_device)
  {
    FreeLibrary(dx11_dllh);
    dx11_dllh = NULL;
    D3D_ERROR("DX11 DLL do not has D3D11CreateDeviceAndSwapChain");
    return false;
  }

  const DataBlock &blk_dx = *::dgs_get_settings()->getBlockByNameEx("directx");
  const DataBlock &videoBlk = *::dgs_get_settings()->getBlockByNameEx("video");

  ignore_resource_leaks_on_exit = blk_dx.getBool("ignore_resource_leaks_on_exit", false);

  if (!resetting_device_now)
    ::create_critical_section(d3d_global_lock, "d3d global lock");

  is_nvapi_initialized = init_nvapi();

  ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

  hdr_enabled = get_enable_hdr_from_settings();

  bool inWin = dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE;

  OSVERSIONINFOEXW osvi = {sizeof(osvi)};
  get_version_ex(&osvi);

  DXGI_SWAP_EFFECT swapEffect = DXGI_SWAP_EFFECT_DISCARD; // The only flag with multisampling support.
  if ((inWin && blk_dx.getBool("flipPresent", true))      // Use the best supported presentation
                                                          // model.
      || hdr_enabled                                      // HDR requires FLIP swap effect or fullscreen
      || used_flip_model_before) // If the window was used with a flip model, it cannot be used with another model, or Present will
                                 // silently fail.
  {                              // https://devblogs.microsoft.com/directx/dxgi-flip-model/ and
    // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-id3d11devicecontext-flush#Defer_Issues_with_Flip
    if (osvi.dwMajorVersion >= 10)
    {
      used_flip_model_before = true;
      swapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    }
    else if (osvi.dwMajorVersion == 6 && osvi.dwMinorVersion >= 2)
    {
      used_flip_model_before = true;
      swapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    }
    use_tearing = _no_vsync && is_tearing_supported() && is_flip_model(swapEffect) && inWin; // The flag DXGI_PRESENT_ALLOW_TEARING
                                                                                             // is only allowed for windowed
                                                                                             // swapchains.
  }
  else
  {
    use_tearing = false;
  }

  if (hdr_enabled && inWin && !is_flip_model(swapEffect))
  {
    hdr_enabled = false;
    debug("HDR requires FLIP swap effect (vsync=1) or fullscreen");
  }

  // Supported starting with Windows 8.1
  // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag#:~:text=is%20Direct3D%2012.-,Note%C2%A0%C2%A0This%20enumeration%20value%20is%20supported%20starting%20with%20Windows%C2%A08.1.,-DXGI_SWAP_CHAIN_FLAG_FOREGROUND_LAYER%0AValue%3A
  if (!is_flip_model(swapEffect) || (osvi.dwMajorVersion <= 6 && osvi.dwMinorVersion < 3))
    use_wait_object = false;

  scd.BufferDesc.Format = PC_BACKBUFFER_DXGI_FORMAT;
  if (hdr_enabled)
    scd.BufferDesc.Format = DXGI_FORMAT_R10G10B10A2_UNORM;

  scd.BufferDesc.Width = screen_wdt;
  scd.BufferDesc.Height = screen_hgt;
  scd.BufferDesc.RefreshRate.Numerator = 0;
  scd.BufferDesc.RefreshRate.Denominator = 1; // While been correct according to MSDN, 0 crashes in some drivers.
  scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED; // DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
  scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED; // DXGI_MODE_SCALING_STRETCHED;//DXGI_MODE_SCALING_CENTERED;//
  scd.SampleDesc.Count = 1;
  scd.SampleDesc.Quality = 0;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT; // To stretch from.
  scd.BufferCount = is_flip_model(swapEffect) ? 2 : 1;
  scd.OutputWindow = window_hwnd;
  scd.Windowed = inWin;
  scd.SwapEffect = swapEffect;
  scd.Flags = (inWin ? 0 : DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH) | (use_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0) |
              (inWin && use_wait_object ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0);
  // Swapchain waitable object isn't supported in full-screen mode in dx11
  // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag#:~:text=This%20flag%20isn%27t%20supported%20in%20full%2Dscreen%20mode%2C%20unless%20the%20render%20API%20is%20Direct3D%2012.

  eastl::vector<D3D_FEATURE_LEVEL> featureLevelsRequested = {D3D_FEATURE_LEVEL_12_1, // DX 11.3 for ConservativeRasterization.
    D3D_FEATURE_LEVEL_11_1,                                                          // For UAVOnlyRenderingForcedSampleCount.
    D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
  const bool forceUseDx10 = blk_dx.getBool("forceUseDx10", false) || get_gpu_driver_cfg().forceDx10;
  const bool preferiGPU = videoBlk.getBool("preferiGPU", false);
  // Set feature level 11.0 for intel GPUs, because many intels hung on higher feature levels.
  if (get_gpu_driver_cfg().primaryVendor == D3D_VENDOR_INTEL || preferiGPU)
  {
    featureLevelsRequested = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
  }
  if (forceUseDx10)
  {
    featureLevelsRequested = {D3D_FEATURE_LEVEL_10_0};
  }

  // D3D_FEATURE_LEVEL_10_1 required for extended msaa
  // D3D_FEATURE_LEVEL_11_0 is for mandatory compute shading

  unsigned int flags = 0;
  if (blk_dx.getBool("debug", false))
    flags |= D3D11_CREATE_DEVICE_DEBUG;

  unsigned int disableTdrFlag = 0;
  if (blk_dx.getBool("disableTdr", true))
    disableTdrFlag |= 0x100; // D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT, supported from 11.1

  bool afterMathEnabled = blk_dx.getBool("afterMathEnabled", false); // warning: this option influence perfomance

  // warning!
  HMODULE dxgi_dll = LoadLibraryA("dxgi.dll"); // load library dxgi.dll has side effect - it correctly shows adapter name after that,
                                               // even after FreeLibrary!
  if (dxgi_dll)
  {
#if ENUMERATE_ADAPTERS
    typedef HRESULT(WINAPI * LPCREATEDXGIFACTORY1)(REFIID, void **);
    LPCREATEDXGIFACTORY1 s_DynamicCreateDXGIFactory = NULL;
    s_DynamicCreateDXGIFactory = (LPCREATEDXGIFACTORY1)GetProcAddress(dxgi_dll, "CreateDXGIFactory1");
    IDXGIFactory1 *pFactory = NULL;
    if (s_DynamicCreateDXGIFactory &&
        SUCCEEDED(s_DynamicCreateDXGIFactory(__uuidof(IDXGIFactory1), (void **)&s_DynamicCreateDXGIFactory)) && pFactory)
    {
      for (int index = 0;; ++index)
      {
        IDXGIAdapter1 *pAdapter = NULL;
        if (FAILED(pFactory->EnumAdapters1(index, &pAdapter)) || !pAdapter)
          break;

        DXGI_ADAPTER_DESC adapterDesc;
        if (SUCCEEDED(pAdapter->GetDesc(&adapterDesc)))
        {
          char outputName[256];
          wcstombs(outputName, adapterDesc.Description, 256);
          debug("adapter:%d: running on <%s> vendord = 0x%X, device=0x%X, SubSys=0x%X", index, outputName, adapterDesc.VendorId,
            adapterDesc.DeviceId, adapterDesc.SubSysId);
        }
      }
      pFactory->Release();
    }
#endif

    if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
      pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
      RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_5_0, (void **)&docAPI);
      if (docAPI && !docAPI->IsTargetControlConnected())
        docAPI->SetCaptureFilePathTemplate("GpuCaptures/");
    }

    if (docAPI)
      docAPI->UnloadCrashHandler();

    FreeLibrary(dxgi_dll);
  }
  // warning!


  const auto getFeatureLevelScd = [&](DXGI_SWAP_CHAIN_DESC base_scd, int level) {
    if (use_wait_object && featureLevelsRequested[level] < D3D_FEATURE_LEVEL_11_0)
      base_scd.Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    return base_scd;
  };

  // Find display by name.

  IDXGIAdapter1 *targetAdapter = NULL;
  target_output = NULL;
  if (!inWin || opt_adapter_luid || preferiGPU)
  {
    DXGI_SWAP_CHAIN_DESC tmpScd = scd;
    tmpScd.Windowed = 1;

    WNDCLASSW wc;
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = DefWindowProcW;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = (HINSTANCE)win32_get_instance();
    wc.hIcon = NULL;
    wc.hCursor = (HCURSOR)win32_init_empty_mouse_cursor();
    wc.hbrBackground = NULL;
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"DagorTmpWindowClass";
    RegisterClassW(&wc);

    tmpScd.OutputWindow = CreateWindowExW(WS_EX_APPWINDOW, L"DagorTmpWindowClass", L"", WS_POPUP, 0, 0, 100, 100, NULL, NULL,
      (HINSTANCE)win32_get_instance(), NULL);
    G_ASSERT(tmpScd.OutputWindow);

    IDXGISwapChain *dummySwapChain;
    ID3D11Device *tmpDevice;
    ID3D11DeviceContext *dummyContext;

    HRESULT hr = DXGI_ERROR_UNSUPPORTED;
    for (int featureLevelNo = 0; featureLevelNo < featureLevelsRequested.size(); featureLevelNo++)
    {
      auto featureLevelScd = getFeatureLevelScd(tmpScd, featureLevelNo);
      D3D_FEATURE_LEVEL featureLevel;
      hr = create_dx11_device(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags, featureLevelsRequested.data() + featureLevelNo, 1,
        D3D11_SDK_VERSION, &featureLevelScd, &dummySwapChain, &tmpDevice, &featureLevel, &dummyContext);
      if (SUCCEEDED(hr))
      {
        if (after_reset)
        {
          compare_feature_levels(featureLevelsSupported, featureLevel);
        }
        featureLevelsSupported = featureLevel;
        tmpScd = featureLevelScd;
        break;
      }
    }

    if (SUCCEEDED(hr))
    {
      IDXGIDevice *pDXGIDevice = NULL;
      HRESULT hr = tmpDevice->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
      G_ASSERT(SUCCEEDED(hr));

      IDXGIAdapter1 *pDXGIAdapter;
      hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter1), (void **)&pDXGIAdapter);
      G_ASSERT(SUCCEEDED(hr));

      IDXGIFactory1 *pIDXGIFactory;
      hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void **)&pIDXGIFactory);
      G_ASSERT(SUCCEEDED(hr));

      uint32_t adapterIndex = 0;
      IDXGIAdapter1 *pAdapter;
      while (pIDXGIFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
      {
        if (opt_adapter_luid)
        {
          DXGI_ADAPTER_DESC adapterDesc;
          pAdapter->GetDesc(&adapterDesc);
          if ((const int64_t &)adapterDesc.AdapterLuid == opt_adapter_luid)
            targetAdapter = pAdapter;
        }

        if (preferiGPU) // video/preferiGPU is enabled.
        {
          ID3D11Device *tmpDevice2;
          D3D_FEATURE_LEVEL featureLevel;
          hr = create_dx11_device(pAdapter, D3D_DRIVER_TYPE_UNKNOWN, nullptr, flags, &featureLevelsSupported, 1, D3D11_SDK_VERSION,
            nullptr, nullptr, &tmpDevice2, &featureLevel, nullptr);
          if (SUCCEEDED(hr))
          {
            if (after_reset)
            {
              compare_feature_levels(featureLevelsSupported, featureLevel);
            }
            featureLevelsSupported = featureLevel;
            ID3D11Device3 *tmpDevice3 = nullptr;
            if (tmpDevice2->QueryInterface(IID_PPV_ARGS(&tmpDevice3)) == S_OK && tmpDevice3)
            {
              D3D11_FEATURE_DATA_D3D11_OPTIONS2 dataOptions2;
              if (tmpDevice3->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &dataOptions2, sizeof(dataOptions2)) == S_OK)
                targetAdapter = dataOptions2.UnifiedMemoryArchitecture ? pAdapter : nullptr;
              tmpDevice3->Release();
            }
          }
          tmpDevice2->Release();
        }
        else if (!opt_adapter_luid || targetAdapter)
        {
          IDXGIOutput *output = get_output_monitor_by_name_or_default(pIDXGIFactory, opt_display_name).Detach();
          if (output)
            target_output = output;
          targetAdapter = target_output ? pAdapter : NULL;
        }

        if (targetAdapter)
          break;

        pAdapter->Release();
        adapterIndex++;
      }

      pIDXGIFactory->Release();
      pDXGIAdapter->Release();
      pDXGIDevice->Release();

      dummySwapChain->Release();
      tmpDevice->Release();
      dummyContext->Release();
    }

    DestroyWindow(tmpScd.OutputWindow);
  }


  // If you provide a D3D_FEATURE_LEVEL array that contains D3D_FEATURE_LEVEL_11_1 on a computer that doesn't have the Direct3D 11.1
  // runtime installed, this function immediately fails with E_INVALIDARG.
  // https://docs.microsoft.com/en-us/windows/desktop/api/d3d11/nf-d3d11-d3d11createdevice
  last_hres = DXGI_ERROR_UNSUPPORTED;
  for (int featureLevelNo = 0; featureLevelNo < featureLevelsRequested.size(); featureLevelNo++)
  {
    auto featureLevelScd = getFeatureLevelScd(scd, featureLevelNo);
    D3D_FEATURE_LEVEL featureLevel;
    last_hres = create_dx11_device(targetAdapter, targetAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, NULL,
      flags | (featureLevelsRequested[featureLevelNo] >= D3D_FEATURE_LEVEL_11_1 ? disableTdrFlag : 0),
      featureLevelsRequested.data() + featureLevelNo, 1, D3D11_SDK_VERSION, &featureLevelScd, &swap_chain, &dx_device, &featureLevel,
      &dx_context);
    if (SUCCEEDED(last_hres))
    {
      if (after_reset)
      {
        compare_feature_levels(featureLevelsSupported, featureLevel);
      }
      featureLevelsSupported = featureLevel;
      scd = featureLevelScd;
      break;
    }
  }

#if HAS_NVAPI
  NVAPI_DEVICE_FEATURE_LEVEL nvFeatureLevelsSupported = NVAPI_DEVICE_FEATURE_LEVEL_NULL;
  if (SUCCEEDED(last_hres) && featureLevelsSupported == D3D_FEATURE_LEVEL_10_0 // Try to get 10.0+ for
                                                                               // DeviceDriverCapabilities::DeviceDriverCapabilities
                                                                               // and
                                                                               // DeviceDriverCapabilities::hasReadMultisampledDepth.
      && is_nvapi_initialized && ::dgs_get_settings()->getBlockByNameEx("video")->getBool("nv10plus", true) && !forceUseDx10)
  {
    NV_DISPLAY_DRIVER_VERSION version = {0};
    version.version = NV_DISPLAY_DRIVER_VERSION_VER;
    NvAPI_Status status = NvAPI_GetDisplayDriverVersion(NVAPI_DEFAULT_HANDLE, &version);
    const uint32_t testedDriverVer = 33182;
    if (status == NVAPI_OK && version.drvVersion >= testedDriverVer)
    {
      swap_chain->SetFullscreenState(FALSE, NULL);
      swap_chain->Release();
      dx_context->Release();
      dx_device->Release();

      debug("attempt to create NvAPI_D3D11_CreateDeviceAndSwapChain");
      NvAPI_Status res =
        NvAPI_D3D11_CreateDeviceAndSwapChain(targetAdapter, targetAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, NULL,
          flags, featureLevelsRequested.data(), (UINT)featureLevelsRequested.size(), D3D11_SDK_VERSION, &scd, &swap_chain, &dx_device,
          &featureLevelsSupported, &dx_context, &nvFeatureLevelsSupported);
      debug("NvAPI_D3D11_CreateDeviceAndSwapChain: res=%d, nvLevel=%d", res, nvFeatureLevelsSupported);

      if (res == NVAPI_OK)
        last_hres = S_OK;
    }
  }
#endif

  if (hdr_enabled)
  {
    IDXGIOutput *dxgiOutput;
    HRESULT hr = swap_chain->GetContainingOutput(&dxgiOutput);
    if (SUCCEEDED(hr))
    {
      IDXGIOutput6 *dxgiOutput6 = nullptr;
      hr = dxgiOutput->QueryInterface(&dxgiOutput6);
      if (SUCCEEDED(hr))
      {
        DXGI_OUTPUT_DESC1 outputDesc1 = {0};
        dxgiOutput6->GetDesc1(&outputDesc1);
        if (outputDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
        {
          IDXGISwapChain3 *swapChain3 = NULL;
          hr = swap_chain->QueryInterface(__uuidof(IDXGISwapChain3), (void **)&swapChain3);
          if (SUCCEEDED(hr))
          {
            DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;

            UINT colorSpaceSupport = 0;
            hr = swapChain3->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport);
            if (SUCCEEDED(hr) && (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
            {
              hr = swapChain3->SetColorSpace1(colorSpace);
              if (SUCCEEDED(hr))
              {
                debug("HDR initialized");
              }
              else
              {
                hdr_enabled = false;
                debug("HDR failed, SetColorSpace1 hr=0x%08X", hr);
              }
            }
            else
            {
              hdr_enabled = false;
              debug("HDR failed, CheckColorSpaceSupport hr=0x%08X, colorSpaceSupport=0x%x", hr, colorSpaceSupport);
            }

            swapChain3->Release();
          }
          else
          {
            hdr_enabled = false;
            debug("HDR failed, IDXGISwapChain3 hr=0x%08X", hr);
          }
        }
        else
        {
          hdr_enabled = false;
          debug("HDR failed, outputColorSpace=%d", outputDesc1.ColorSpace);
        }

        dxgiOutput6->Release();
      }
      else
      {
        hdr_enabled = false;
        debug("HDR failed, IDXGIOutput6 hr=0x%08X", hr);
      }

      dxgiOutput->Release();
    }
    else
      hdr_enabled = false;
  }

  if (!hdr_enabled && get_enable_hdr_from_settings())
  {
    drv_message_box(get_localized_text("msgbox/hdr_failed", "HDR initialization failed. The output display is not in a HDR mode."),
      get_localized_text("msgbox/error_header"), GUI_MB_ICON_INFORMATION);

    scd.BufferDesc.Format = PC_BACKBUFFER_DXGI_FORMAT;

    swap_chain->SetFullscreenState(FALSE, NULL);
    swap_chain->Release();
    dx_context->Release();
    dx_device->Release();

    ShowWindow(scd.OutputWindow, SW_MINIMIZE); // Fix "The buffer width inferred from the output window is zero. Taking 8 as a
                                               // reasonable default instead"
    ShowWindow(scd.OutputWindow,
      SW_RESTORE); // https://docs.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgifactory-createswapchain

    D3D_FEATURE_LEVEL featureLevel;
    last_hres = create_dx11_device(targetAdapter, targetAdapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE, NULL, flags,
      &featureLevelsSupported, 1, D3D11_SDK_VERSION, &scd, &swap_chain, &dx_device, &featureLevel, &dx_context);
    if (after_reset)
    {
      compare_feature_levels(featureLevelsSupported, featureLevel);
    }
    featureLevelsSupported = featureLevel;
  }

  if (SUCCEEDED(last_hres) && (scd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT))
    create_waitable_object();

  if (blk_dx.getBool("debug", false))
  {
    dx_device->QueryInterface(__uuidof(ID3D11InfoQueue), (void **)&pInfoQueue);
    if (pInfoQueue)
    {
      if (blk_dx.getBool("throwDebugLayerExceptions", false))
      {
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        dgs_report_fatal_error = debug_layer_report_fatal_error;
      }

      D3D11_INFO_QUEUE_FILTER defaultFilter{};
      // on default we don't care about messages and info
      D3D11_MESSAGE_SEVERITY denySeverity[] = //
        {D3D11_MESSAGE_SEVERITY_MESSAGE, D3D11_MESSAGE_SEVERITY_INFO};
      auto denyId = get_ignored_validation_messages<D3D11_MESSAGE_ID>(*::dgs_get_settings()->getBlockByNameEx("directx"));
      defaultFilter.DenyList.pSeverityList = denySeverity;
      defaultFilter.DenyList.NumSeverities = static_cast<UINT>(2);
      defaultFilter.DenyList.pIDList = denyId.data();
      defaultFilter.DenyList.NumIDs = static_cast<UINT>(denyId.size());
      pInfoQueue->AddRetrievalFilterEntries(&defaultFilter);
      pInfoQueue->AddStorageFilterEntries(&defaultFilter);
    }
  }

  stereo_config_callback = cb;

  g_device_desc.caps.hasDepthReadOnly = false;
  g_device_desc.caps.hasStructuredBuffers = true;
  g_device_desc.caps.hasNoOverwriteOnShaderResourceBuffers = false;
  g_device_desc.caps.hasForcedSamplerCount = false;
  g_device_desc.caps.hasVolMipMap = true;
  g_device_desc.caps.hasAsyncCompute = false;
  g_device_desc.caps.hasOcclusionQuery = true;
  g_device_desc.caps.hasConstBufferOffset = false;
  g_device_desc.caps.hasDepthBoundsTest = false;
  g_device_desc.caps.hasConditionalRender = true;
  g_device_desc.caps.hasResourceCopyConversion = true;
  g_device_desc.caps.hasReadMultisampledDepth = true;
  g_device_desc.caps.hasInstanceID = true;
  g_device_desc.caps.hasConservativeRassterization = false;
  g_device_desc.caps.hasQuadTessellation = false;
  g_device_desc.caps.hasGather4 = true;
  g_device_desc.caps.hasWellSupportedIndirect = true;
  g_device_desc.caps.hasBindless = false;
  g_device_desc.caps.hasNVApi = false;
  g_device_desc.caps.hasATIApi = false;
  g_device_desc.caps.hasVariableRateShading = false;
  g_device_desc.caps.hasVariableRateShadingTexture = false;
  g_device_desc.caps.hasVariableRateShadingShaderOutput = false;
  g_device_desc.caps.hasVariableRateShadingCombiners = false;
  g_device_desc.caps.hasVariableRateShadingBy4 = false;
  g_device_desc.caps.hasAliasedTextures = false;
  g_device_desc.caps.hasResourceHeaps = false;
  g_device_desc.caps.hasBufferOverlapCopy = false;
  g_device_desc.caps.hasBufferOverlapRegionsCopy = false;
  g_device_desc.caps.hasUAVOnlyForcedSampleCount = false;
  g_device_desc.caps.hasShader64BitIntegerResources = false;
  g_device_desc.caps.hasNativeRenderPassSubPasses = false;
  g_device_desc.caps.hasTiled2DResources = false;
  g_device_desc.caps.hasTiled3DResources = false;
  g_device_desc.caps.hasTiledSafeResourcesAccess = false;
  g_device_desc.caps.hasTiledMemoryAliasing = false;
  g_device_desc.caps.hasDLSS = false;
  g_device_desc.caps.hasXESS = false;
  g_device_desc.caps.hasDrawID = false;
  g_device_desc.caps.hasMeshShader = false;
  g_device_desc.caps.hasBasicViewInstancing = false;
  g_device_desc.caps.hasOptimizedViewInstancing = false;
  g_device_desc.caps.hasAcceleratedViewInstancing = false;
  g_device_desc.caps.hasRenderPassDepthResolve = false;
  g_device_desc.caps.hasShaderFloat16Support = false;
  g_device_desc.caps.hasUAVOnEveryStage = false;
  g_device_desc.caps.hasRayAccelerationStructure = false;
  g_device_desc.caps.hasRayQuery = false;
  g_device_desc.caps.hasRayDispatch = false;
  g_device_desc.caps.hasIndirectRayDispatch = false;
  g_device_desc.caps.hasGeometryIndexInRayAccelerationStructure = false;
  g_device_desc.caps.hasSkipPrimitiveTypeInRayTracingShaders = false;
  g_device_desc.caps.hasNativeRayTracePipelineExpansion = false;

  if (streamlineAdapter)
  {
    streamlineAdapter->setAdapterAndDevice(targetAdapter, dx_device);

    if (streamlineAdapter->isDlssSupported() == nv::SupportState::Supported)
    {
      g_device_desc.caps.hasDLSS = true;
      create_dlss_feature(resolution);
    }

    switch (streamlineAdapter->getDlssState()) // -V785
    {
      case nv::DLSS::State::SUPPORTED:
        debug("DX11 drv: DLSS is supported on this hardware, but it was not enabled. video/dlssQuality: %d",
          videoBlk.getInt("dlssQuality", -1));
        break;
      case nv::DLSS::State::READY:
        debug("DX11 drv: DLSS is supported and is ready to go. video/dlssQuality: %d", videoBlk.getInt("dlssQuality", -1));
        break;
      case nv::DLSS::State::NOT_SUPPORTED_INCOMPATIBLE_HARDWARE: debug("DX11 drv: DLSS is not supported on this hardware."); break;
      case nv::DLSS::State::NOT_SUPPORTED_OUTDATED_VGA_DRIVER:
        debug("DX11 drv: DLSS is not supported on because of outdated VGA drivers.");
        break;
      case nv::DLSS::State::NOT_SUPPORTED_32BIT: debug("DX11 drv: DLSS is not supported in 32bit builds."); break;
      case nv::DLSS::State::DISABLED: debug("DX11 drv: DLSS is disabled for drv3d_DX11 lib."); break;
      case nv::DLSS::State::NGX_INIT_ERROR_NO_APP_ID:
        debug("DX11 drv: Couldn't initialize NGX because there is no Nvidia AppID set in game specific settings.blk");
        break;
      default: D3D_ERROR("DX11 drv: Unexpected DLSS state after initialization: %d", int(streamlineAdapter->getDlssState())); break;
    }
  }

  if (targetAdapter)
    targetAdapter->Release();
  targetAdapter = NULL;

  if (FAILED(last_hres)) // DXGI_STATUS_OCCLUDED returned if the fullscreen app is not active on startup.
  {
    D3D_ERROR("Cannot initialize DX11 device 0x%08X, %s", last_hres, dx11_error(last_hres));
    return false;
  }
  if (device_is_lost != S_OK)
  {
    debug("dx_device=%p (recreated after reset)", dx_device);
    device_is_lost = S_OK;
  }

#if HAS_NVAPI
  if (nvFeatureLevelsSupported == NVAPI_DEVICE_FEATURE_LEVEL_10_0_PLUS)
  {
    g_device_desc.shaderModel = 4.0_sm;
    g_device_desc.caps.hasGather4 = false;
    g_device_desc.caps.hasDepthBoundsTest = false;
    featureLevelsSupported = D3D_FEATURE_LEVEL_10_0; // Override 10_1 returned on 10_0 hardware to pair cube RT with cube depth. BSOD
                                                     // on 8800 otherwise.
    debug("running on NV DX10.0+ level hardware");
  }
  else
#endif
    if (featureLevelsSupported >= D3D_FEATURE_LEVEL_11_0)
  {
    g_device_desc.shaderModel = 5.0_sm;
    g_device_desc.caps.hasDepthReadOnly = true;
    g_device_desc.caps.hasQuadTessellation = true;
    g_device_desc.caps.hasForcedSamplerCount = (featureLevelsSupported >= D3D_FEATURE_LEVEL_11_1); // all Dx11.1 level hardware
                                                                                                   // supports it
    g_device_desc.caps.hasUAVOnEveryStage = (featureLevelsSupported >= D3D_FEATURE_LEVEL_11_1);
    debug("running on DX%d.%d level hardware", featureLevelsSupported >> 12, (featureLevelsSupported >> 8) & 0xF);
  }
  else if (featureLevelsSupported == D3D_FEATURE_LEVEL_10_1)
  {
    g_device_desc.shaderModel = 4.1_sm;
    g_device_desc.caps.hasDepthReadOnly = true;
    debug("running on DX10.1 level hardware");
  }
  else if (featureLevelsSupported == D3D_FEATURE_LEVEL_10_0)
  {
    g_device_desc.caps.hasDepthReadOnly = false;
    g_device_desc.caps.hasGather4 = false;
    g_device_desc.caps.hasResourceCopyConversion = false;
    g_device_desc.caps.hasReadMultisampledDepth = false;
    g_device_desc.shaderModel = 4.0_sm;
    debug("running on DX10.0 level hardware");
  }
  else
  {
    g_device_desc.shaderModel = 4.0_sm; //==?
    debug("DX11 running on invalid hardware");
    return false;
  }

  D3D11_FEATURE_DATA_THREADING dataThreading;
  if (dx_device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &dataThreading, sizeof(dataThreading)) != S_OK ||
      !dataThreading.DriverConcurrentCreates)
  {
    D3D_ERROR("dx11 driver do not support concurrent creates");
  }

  D3D11_FEATURE_DATA_D3D11_OPTIONS dataOptions;
  if (dx_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS, &dataOptions, sizeof(dataOptions)) == S_OK)
  {
    g_device_desc.caps.hasNoOverwriteOnShaderResourceBuffers = FALSE != dataOptions.MapNoOverwriteOnDynamicBufferSRV;
    g_device_desc.caps.hasForcedSamplerCount =
      g_device_desc.caps.hasForcedSamplerCount || FALSE != dataOptions.UAVOnlyRenderingForcedSampleCount;
    g_device_desc.caps.hasUAVOnlyForcedSampleCount = FALSE != dataOptions.UAVOnlyRenderingForcedSampleCount;
    g_device_desc.caps.hasConstBufferOffset = FALSE != dataOptions.ConstantBufferOffsetting;
    g_device_desc.caps.hasBufferOverlapCopy = FALSE != dataOptions.CopyWithOverlap;
  }

  D3D11_FEATURE_DATA_D3D11_OPTIONS2 dataOptions2;
  if (dx_device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS2, &dataOptions2, sizeof(dataOptions2)) == S_OK)
  {
    g_device_desc.caps.hasConservativeRassterization =
      dataOptions2.ConservativeRasterizationTier >= D3D11_CONSERVATIVE_RASTERIZATION_TIER_1;

    debug("ConservativeRasterizationTier=%d", dataOptions2.ConservativeRasterizationTier);

    if (preferiGPU && !dataOptions2.UnifiedMemoryArchitecture)
      logwarn("Despite the preferiGPU flag being enabled, the dedicated GPU is used!");
  }

  if (get_gpu_driver_cfg().primaryVendor == D3D_VENDOR_INTEL)
  {
    // We disable forced sample count for intel GPUs because it cause GPU hung on Intel 4000. Fixme: to be analyzed
    g_device_desc.caps.hasForcedSamplerCount = false;
    g_device_desc.caps.hasUAVOnlyForcedSampleCount = false;
  }

  D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS dx10HardwareOptions;
  if (dx_device->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &dx10HardwareOptions,
        sizeof(dx10HardwareOptions)) != S_OK ||
      !dx10HardwareOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x ||
      (featureLevelsSupported < D3D_FEATURE_LEVEL_11_0 && get_gpu_driver_cfg().disableSbuffers)) // Cannot disable sbuffers on DX11 -
                                                                                                 // it is a required feature.
  {
    g_device_desc.caps.hasStructuredBuffers = false;
    debug("Sbuf (D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) is not supported");
  }

  HRESULT hr = dx_device->QueryInterface(__uuidof(ID3D11Device1), (void **)&dx_device1);
  debug("ID3D11Device1 hr=0x%08X", hr);
  if (FAILED(hr))
    g_device_desc.caps.hasForcedSamplerCount = false;

  hr = dx_device->QueryInterface(__uuidof(ID3D11Device3), (void **)&dx_device3);
  debug("ID3D11Device3 hr=0x%08X", hr);
  if (FAILED(hr))
    g_device_desc.caps.hasConservativeRassterization = false;

  if (g_device_desc.caps.hasConstBufferOffset)
  {
    hr = dx_context->QueryInterface(__uuidof(ID3D11DeviceContext1), (void **)&dx_context1);
    debug("ID3D11DeviceContext1 hr=0x%08X", hr);
    if (FAILED(hr))
      g_device_desc.caps.hasConstBufferOffset = false;
    else
    {
      D3D11_DEVICE_CONTEXT_TYPE contextType = dx_context->GetType();
      if (D3D11_DEVICE_CONTEXT_DEFERRED == contextType)
      {
        D3D11_FEATURE_DATA_THREADING threadingCaps = {FALSE, FALSE};
        hr = dx_device->CheckFeatureSupport(D3D11_FEATURE_THREADING, &threadingCaps, sizeof(threadingCaps));
        if (SUCCEEDED(hr) && !threadingCaps.DriverCommandLists)
        {
          command_list_wa = true; // the runtime emulates command lists.
        }
      }
    }
  }

  debug("MapNoOverwriteOnDynamicBufferSRV=%s, UAVOnlyRenderingForcedSampleCount=%s, ConservativeRasterization=%s, "
        "ConstantBufferOffsetting=%s",
    g_device_desc.caps.hasNoOverwriteOnShaderResourceBuffers ? "TRUE" : "FALSE",
    g_device_desc.caps.hasForcedSamplerCount ? "TRUE" : "FALSE", g_device_desc.caps.hasConservativeRassterization ? "TRUE" : "FALSE",
    g_device_desc.caps.hasConstBufferOffset ? "TRUE" : "FALSE");

  // Get GPU name.
  IDXGIDevice *pDXGIDevice = NULL;
  dx_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
  IDXGIAdapter *adapter;
  if (pDXGIDevice && SUCCEEDED(pDXGIDevice->GetAdapter(&adapter)))
  {
    adapter->GetDesc(&adapterDesc);
    wcstombs(deviceName, adapterDesc.Description, 128); // Warning: this must be done BEFORE calls d3d::get_device_name()
    debug("adapter <%s>vendor=0x%X, device=0x%X, ss=0x%X : Video mem: %d MB sysmem %d MB, shared %d MB", deviceName,
      adapterDesc.VendorId, adapterDesc.DeviceId, adapterDesc.SubSysId, adapterDesc.DedicatedVideoMemory >> 20,
      adapterDesc.DedicatedSystemMemory >> 20, // Probably incorrect on Intel GPUs, 0 reproted while the memory is allocated through
                                               // the BIOS.
      adapterDesc.SharedSystemMemory >> 20);

    adapter->Release();
  }

  D3D_FEATURE_LEVEL minimumFeatureLevel = feature_level_from_str(blk_dx.getStr("minimumFeatureLevel", "10_0"));
  if (featureLevelsSupported < minimumFeatureLevel)
  {
    String message(0, "DirectX feature level %s is lesser than required %s. The driver may be outdated.",
      str_from_feature_level(featureLevelsSupported), str_from_feature_level(minimumFeatureLevel));
    drv_message_box(message.data(), "Initialization failed", GUI_MB_ICON_ERROR);
    DAG_FATAL(message.data());
    return false;
  }

  // Ban Microsoft Basic Render Driver, it is slow and unreliable.
  if (adapterDesc.VendorId == 0x1414 && adapterDesc.DeviceId == 0x8C)
  {
    debug("Error initializing video, Microsoft Basic Render Driver is not supported");
    if (get_d3d_reset_counter() > 1 || get_d3d_full_reset_counter() > 1)
    {
      drv_message_box(
        get_localized_text("msgbox/dx11_reset_failed", "Video device initialization failed. The driver may be outdated."),
        "Initialization failed", GUI_MB_ICON_ERROR);
      DAG_FATAL("Failed to restore GPU driver after device resetting");
    }
    return false;
  }

  set_nvapi_vsync(dx_device);
#if HAS_GF_AFTERMATH
  if (afterMathEnabled)
    init_aftermath(dx_device);
#endif


#if HAS_NVAPI
  if (is_nvapi_initialized && NVAPI_OK == NvAPI_D3D11_SetDepthBoundsTest(dx_device, 1, 0.f, 1.f) &&
      NVAPI_OK == NvAPI_D3D11_SetDepthBoundsTest(dx_device, 0, 0.f, 1.f))
  {
    nv_dbt_supported = true;
    g_device_desc.caps.hasDepthBoundsTest = true;
  }

  if (is_nvapi_initialized)
  {
    g_device_desc.caps.hasNVApi = true;
  }
#endif


  // Init ATISDK.
#if defined _WIN64
  dump_dll_version("atidxx64.dll");
  hAtiDLL = GetModuleHandleW(L"atidxx64.dll");
#else
  dump_dll_version("atidxx32.dll");
  hAtiDLL = GetModuleHandleW(L"atidxx32.dll");
#endif
  if (hAtiDLL)
  {
    PFNAmdDxExtCreate11 AmdDxExtCreate = reinterpret_cast<PFNAmdDxExtCreate11>(GetProcAddress(hAtiDLL, "AmdDxExtCreate11"));
    if (AmdDxExtCreate != nullptr)
    {
      HRESULT hr = AmdDxExtCreate(dx_device, &amdExtension);
      debug("AmdDxExtCreate: hr=0x%08x, p=0x%p", hr, amdExtension);
      g_device_desc.minWarpSize = 64;
      g_device_desc.maxWarpSize = 64;

      // Get the AMD Depth Bounds Extension Interface
      if (SUCCEEDED(hr) && amdExtension != nullptr)
      {
        g_device_desc.caps.hasATIApi = true;
        amdDepthBoundsExtension = static_cast<IAmdDxExtDepthBounds *>(amdExtension->GetExtInterface(AmdDxExtDepthBoundsID));
        debug("amdDepthBoundsExtension=0x%p", amdDepthBoundsExtension);
        if (amdDepthBoundsExtension != nullptr)
        {
          ati_dbt_supported = true;
          g_device_desc.caps.hasDepthBoundsTest = true;
        }
      }
    }
  }

#if HAS_NVAPI
  if (is_nvapi_initialized)
  {
    NV_DISPLAY_DRIVER_VERSION version = {0};
    version.version = NV_DISPLAY_DRIVER_VERSION_VER;
    NvAPI_Status status = NvAPI_GetDisplayDriverVersion(NVAPI_DEFAULT_HANDLE, &version);
    debug("NV driver version = %d", version.drvVersion);
    if (status == NVAPI_OK)
    {
      const uint32_t testedCopyResourceConvDriverVer = 32018;
      if (version.drvVersion < testedCopyResourceConvDriverVer)
      {
        debug("NV ver=%d, DeviceDriverCapabilities::hasResourceCopyConversion disabled", version.drvVersion);
        g_device_desc.caps.hasResourceCopyConversion = false;
      }

      const uint32_t testedReadMsaaDepthDriverVer = 33221; // Animated random squares all over the screen.
      if (version.drvVersion < testedReadMsaaDepthDriverVer)
      {
        debug("NV ver=%d, DeviceDriverCapabilities::hasReadMultisampledDepth disabled", version.drvVersion);
        g_device_desc.caps.hasReadMultisampledDepth = false;
      }
    }
    else
    {
      debug("NVAPI: GetDisplayDriverVersion failed, error %d\n", status);
      is_nvapi_initialized = false;
    }
  }
#endif

  if (get_gpu_driver_cfg().oldHardware && get_gpu_driver_cfg().primaryVendor == D3D_VENDOR_ATI || g_device_desc.shaderModel < 5.0_sm)
    g_device_desc.caps.hasWellSupportedIndirect = false;

  debug("DepthBoundsTest=%d", g_device_desc.caps.hasDepthBoundsTest);

  if (g_device_desc.caps.hasDepthBoundsTest) // All Intels and some GPUs with really old drivers.
  {
    if (featureLevelsSupported <= D3D_FEATURE_LEVEL_10_1) // Intel HD 3000 or older.
    {
      // Access violation in igd10umd64.dll on HD3000 with 2015/05/27 9.17.10.4229 driver and silent exit on 32-bit exe.
      // Not fixed in 9.17.10.4459.
      debug("DeviceDriverCapabilities::DeviceDriverCapabilities disabled (DX10 Intel GPU or outdated driver)");
      g_device_desc.caps.hasOcclusionQuery = false;
    }
  }

  if (dx_device)
  {
    IDXGIDevice *pDXGIDevice = NULL;
    HRESULT hr = dx_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
    G_ASSERT(SUCCEEDED(hr));

    IDXGIAdapter1 *pDXGIAdapter;
    hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter1), (void **)&pDXGIAdapter);
    G_ASSERT(SUCCEEDED(hr));

    hr = pDXGIAdapter->GetParent(__uuidof(IDXGI_FACTORY), (void **)&dx11_DXGIFactory);
    G_ASSERT(SUCCEEDED(hr));

    pDXGIAdapter->Release();
    pDXGIDevice->Release();
    DEBUG_CTX("dx11_DXGIFactory=%p", dx11_DXGIFactory);
  }

  // Get target_output from current adapter again (fix oculus fullscreen on Windows 8.1).

  if (dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE)
  {
    if (IDXGIOutput *output = get_output_monitor_by_name_or_default(dx11_DXGIFactory, opt_display_name).Detach(); output)
    {
      if (target_output && target_output != output)
        target_output->Release();
      target_output = output;
    }

    // http://msdn.microsoft.com/en-us/library/windows/desktop/ee417025%28v=vs.85%29.aspx
    swap_chain->ResizeTarget(&scd.BufferDesc);           // Avoid mode change in fullscreen.
    swap_chain->SetFullscreenState(TRUE, target_output); // Get actual display output.
  }

  find_closest_matching_mode();

  debug("vsync_refresh_rate %0.6f", vsync_refresh_rate);


  /*debug("in win = %d", _in_win);
  if (!_in_win)
  {
    scd.BufferDesc.RefreshRate.Numerator = 0;
    scd.BufferDesc.RefreshRate.Denominator = 1;
    swap_chain->ResizeTarget(&scd.BufferDesc);
    IDXGIOutput* pTarget = NULL; BOOL bFullscreen;
    if (SUCCEEDED(swap_chain->GetFullscreenState(&bFullscreen, &pTarget)) && pTarget)
    {
       debug("getfullscreen %d", (int)bFullscreen);
       //pTarget->Release();
    }
    else
       bFullscreen = FALSE;
    // If not full screen, enable full screen again.
    //if (!bFullscreen)
    {
       debug("minimize");
       ShowWindow(window_hwnd, SW_MINIMIZE);
       debug("restore");
       ShowWindow(window_hwnd, SW_RESTORE);
       debug("fullscreen");
       swap_chain->SetFullscreenState(TRUE, pTarget);
       debug("fullscreen-");
    }
    if (pTarget)
     pTarget->Release();
  }*/
  /*
  int maximumFrameLatency = -1;
  if (maximumFrameLatency>=0)
  {
    IDXGIDevice1 * pDXGIDevice1;
    if (dx_device->QueryInterface(__uuidof(IDXGIDevice1), (void **)&pDXGIDevice1) == S_OK && pDXGIDevice1)
    {
      pDXGIDevice1->SetMaximumFrameLatency(maximumFrameLatency);
      pDXGIDevice1->Release();
    }
  }
  */

  if (pDXGIDevice) // Disable Alt-Enter processing and PrintScreen.
  {
    IDXGIAdapter1 *pDXGIAdapter;
    HRESULT hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter1), (void **)&pDXGIAdapter);

    if (SUCCEEDED(hr))
    {
      IDXGIFactory1 *pIDXGIFactory;
      hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void **)&pIDXGIFactory);
      if (SUCCEEDED(hr))
      {
        hr = pIDXGIFactory->MakeWindowAssociation(window_hwnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN);
        G_ASSERT(SUCCEEDED(hr));
        pIDXGIFactory->Release();
      }
      pDXGIAdapter->Release();
    }
    pDXGIDevice->Release();
  }

  if (videoBlk.getBool("secondaryWindow", false))
  {
    IDXGIDevice *pDXGIDevice;
    HRESULT hr = dx_device->QueryInterface(__uuidof(IDXGIDevice), (void **)&pDXGIDevice);
    G_ASSERT(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
      IDXGIAdapter1 *pDXGIAdapter;
      hr = pDXGIDevice->GetParent(__uuidof(IDXGIAdapter1), (void **)&pDXGIAdapter);
      G_ASSERT(SUCCEEDED(hr));
      if (SUCCEEDED(hr))
      {
        IDXGIFactory1 *pIDXGIFactory;
        hr = pDXGIAdapter->GetParent(__uuidof(IDXGIFactory1), (void **)&pIDXGIFactory);
        G_ASSERT(SUCCEEDED(hr));
        if (SUCCEEDED(hr))
        {
          int width, height;
          get_resolution_from_str(videoBlk.getStr("secondaryResolution", "1920x1080"), width, height);

          RECT screenRect;
          SystemParametersInfo(SPI_GETWORKAREA, 0, &screenRect, 0);

          RECT windowRect;
          windowRect.left = (screenRect.right - screenRect.left - width) / 2 + screenRect.left;
          windowRect.top = (screenRect.bottom - screenRect.top - height) / 2 + screenRect.top;

          windowRect.right = windowRect.left + width;
          windowRect.bottom = windowRect.top + height;
          uint32_t windowStyle = WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
          AdjustWindowRectEx(&windowRect, windowStyle, FALSE, WS_EX_APPWINDOW);

          WNDCLASSW wndClass;
          wndClass.style = CS_HREDRAW | CS_VREDRAW;
          wndClass.lpfnWndProc = DefWindowProcW;
          wndClass.cbClsExtra = 0;
          wndClass.cbWndExtra = 0;
          wndClass.hInstance = (HINSTANCE)win32_get_instance();
          wndClass.hIcon = LoadIcon((HINSTANCE)win32_get_instance(), "WindowIcon");
          wndClass.hCursor = LoadCursor(NULL, IDC_NO);
          wndClass.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
          wndClass.lpszMenuName = NULL;
          wndClass.lpszClassName = L"DagorSecondaryWindowClass";
          RegisterClassW(&wndClass);

          secondary_window_hwnd = CreateWindowExW(WS_EX_APPWINDOW, L"DagorSecondaryWindowClass", L"", windowStyle, windowRect.left,
            windowRect.top, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, NULL, NULL,
            (HINSTANCE)win32_get_instance(), NULL);
          G_ASSERT(secondary_window_hwnd);

          if (secondary_window_hwnd)
          {
            ShowWindow(secondary_window_hwnd, SW_SHOWNOACTIVATE);
            UpdateWindow(secondary_window_hwnd);

            DXGI_SWAP_CHAIN_DESC secSwapChainDesc;
            ZeroMemory(&secSwapChainDesc, sizeof(secSwapChainDesc));
            secSwapChainDesc.BufferDesc.Width = width;
            secSwapChainDesc.BufferDesc.Height = height;
            secSwapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
            secSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
            secSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
            secSwapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
            secSwapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
            secSwapChainDesc.SampleDesc.Count = 1;
            secSwapChainDesc.SampleDesc.Quality = 0;
            secSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            secSwapChainDesc.BufferCount = 1;
            secSwapChainDesc.OutputWindow = secondary_window_hwnd;
            secSwapChainDesc.Windowed = TRUE;
            secSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
            secSwapChainDesc.Flags = 0;

            hr = pIDXGIFactory->CreateSwapChain(dx_device, &secSwapChainDesc, &secondary_swap_chain);
            G_ASSERT(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
              secondary_backbuffer_color_tex = create_backbuffer_tex(0, secondary_swap_chain);
          }
          pIDXGIFactory->Release();
        }
        pDXGIAdapter->Release();
      }
      pDXGIDevice->Release();
    }
  }

  if (!is_float_exceptions_enabled())
    _fpreset();

  debug("Fragment shader version: %d", g_device_desc.shaderModel);

  DEBUG_CTX("creating backbuffers %dx%d", screen_wdt, screen_hgt);

  g_driver_state.createSurfaces(screen_wdt, screen_hgt);

  nvlowlatency::init();

  init_memory_report();

  _in_win_started = inWin;

  return SUCCEEDED(last_hres);
}

static void close_all(bool is_reset)
{
  close_rendertargets();
  close_buffers();
  close_shaders();
  close_states();
  close_samplers();
  close_predicates();
  close_textures();
  close_frame_callbacks();
  reset_all_queries();

  if (is_reset)
    print_memory_stat();

  debug("%s", __FUNCTION__);
}

static void close_device(bool is_reset)
{
  nvlowlatency::close();
  close_perf_analysis();
  RenderState &rs = g_render_state;
  DriverState &ds = g_driver_state;

  DEBUG_CTX("deleted backbuffer %p", ds.backBufferColorTex);
  del_d3dres(ds.backBufferColorTex);

  debug("%s", __FUNCTION__);

  if (!dx11_dllh)
    return;

  if (dx11_DXGIFactory)
    dx11_DXGIFactory->Release();
  dx11_DXGIFactory = NULL;

  close_all(is_reset);

  if (amdDepthBoundsExtension != nullptr)
  {
    amdDepthBoundsExtension->Release();
    amdDepthBoundsExtension = nullptr;
  }
  if (amdExtension != nullptr)
  {
    amdExtension->Release();
    amdExtension = nullptr;
  }

  if (pInfoQueue)
    pInfoQueue->Release();

  if (target_output)
    target_output->Release();
  target_output = NULL;

  d3d::pcwin32::set_present_wnd(NULL);
  for (int i = 0; i < swap_chain_pairs.size(); i++)
  {
    del_d3dres(swap_chain_pairs[i].buf);
    swap_chain_pairs[i].sw->Release();
  }
  clear_and_shrink(swap_chain_pairs);

  if (swap_chain)
  {
    swap_chain->SetFullscreenState(FALSE, NULL); // switch to windowed mode
    int ts = get_time_msec();
    swap_chain->Release();
    debug("release swap_chain done in %d msec", get_time_msec() - ts);
  }
  swap_chain = NULL;

  del_d3dres(secondary_backbuffer_color_tex);

  DEBUG_CP();
  if (secondary_swap_chain)
    secondary_swap_chain->Release();

  if (dx_context)
  {
    // If we are in shutdown, we don't need this sequence. It is described at:
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-flush#deferred-destruction-issues-with-flip-presentation-swap-chains
    // and is only relevant when we will need to re-create the swapchain.
    if (is_reset)
    {
      DEBUG_CP();
      dx_context->ClearState();
      DEBUG_CP();

      {
        // The flush can last a long time, and we don't want watchdog to trigger too eagerly.
        ScopeSetWatchdogTimeout _wd(DEVICE_RESET_FLUSH_TIMEOUT_MS);
        int startTimeMs = get_time_msec();

        DEBUG_CP();
        dx_context->Flush();
        DEBUG_CP();

        int elapsedMs = get_time_msec() - startTimeMs;
        if (elapsedMs > DEVICE_RESET_FLUSH_LONG_WAIT_MS)
          D3D_ERROR("DX11 device reset flush took %d ms.", elapsedMs);
      }

      dx_context->Release();
    }
  }
  dx_context = NULL;

  if (dx_context1)
    dx_context1->Release();
  dx_context1 = NULL;

  if (dx_device1)
    dx_device1->Release();

  if (dx_device3)
    dx_device3->Release();

  if (dx_device)
  {
    DEBUG_CP();
    dx_device->Release();
    dx_device = NULL;
  }

  DEBUG_CP();
#if HAS_GF_AFTERMATH
  close_aftermath();
#endif

  DEBUG_CP();
#if HAS_NVAPI
  if (is_nvapi_initialized && NvAPI_Unload() != NVAPI_OK)
    D3D_ERROR("Failed to cleanup NVAPI Library.");
#endif
  if (streamlineAdapter)
  {
    if (streamlineAdapter->isDlssSupported() == nv::SupportState::Supported)
    {
      release_dlss_feature();
    }
    streamlineAdapter.reset();
  }
  streamlineInterposer.reset();

  is_nvapi_initialized = false;

  if (!resetting_device_now)
  {
    ::destroy_critical_section(d3d_global_lock);
#if FREE_LIBRARY
    debug("close_library");
    if (dx11_dllh)
    {
      FreeLibrary(dx11_dllh);
      dx11_dllh = NULL;
    }
#endif
  }
  DEBUG_CP();

  if (rdoc_dllh)
  {
    FreeLibrary(rdoc_dllh);
    rdoc_dllh = NULL;
  }
}

HRESULT drv3d_dx11::set_fullscreen_state(bool fullscreen)
{
  bool needToUpdateSurfaces = false;
  HRESULT hres;
  {
    ContextAutoLock contextLock;
    auto newScd = scd;
    // Swapchains using DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT cannot go fullscreen.
    // We need to recreate swapchain to remove this flag.
    if (fullscreen && (scd.Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT))
      newScd.Flags &= ~DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;

    // Need to disable present with tearing in fullscreen, or enable it back in windowed mode.
    if (scd.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING)
      use_tearing = !fullscreen && _no_vsync && is_tearing_supported() && is_flip_model(newScd.SwapEffect);

    if (newScd.Flags != scd.Flags)
    {
      hres = recreate_swapchain(newScd);
      if (SUCCEEDED(hres))
        needToUpdateSurfaces = true;
    }
    hres = fullscreen ? swap_chain->SetFullscreenState(TRUE, target_output) : swap_chain->SetFullscreenState(FALSE, NULL);
  }

  if (SUCCEEDED(hres) && fullscreen && is_flip_model(scd.SwapEffect)) //-V560
  {
    DEBUG_CTX("deleted backbuffer %p", g_driver_state.backBufferColorTex);
    // DXGI_SWAP_EFFECT_FLIP_DISCARD or DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
    // MSDN: Before you call ResizeBuffers, ensure that the application releases all references.
    // Note: Destroy via DA to reduce chances that some other render (perhaps splash) thread refs it
    add_delayed_callback_buffered([](void *t) { destroy_d3dres((Texture *)t); }, g_driver_state.backBufferColorTex);
    g_driver_state.backBufferColorTex = nullptr;

    {
      ContextAutoLock contextLock;
      swap_chain->ResizeBuffers(scd.BufferCount, scd.BufferDesc.Width, scd.BufferDesc.Height, scd.BufferDesc.Format,
        scd.Flags); // MSDN: For the flip presentation model, after you transition the display state to full screen,
                    // you must call ResizeBuffers to ensure that your call to IDXGISwapChain1::Present1 succeeds.
    }
    needToUpdateSurfaces = true;
  }

  if (needToUpdateSurfaces)
    g_driver_state.createSurfaces(scd.BufferDesc.Width, scd.BufferDesc.Height);

  return hres;
}

DAGOR_NOINLINE static void toggle_fullscreen(HWND hWnd, UINT message, WPARAM wParam)
{
  if (dgs_get_window_mode() != WindowMode::FULLSCREEN_EXCLUSIVE)
    return;

  if (has_focus(hWnd, message, wParam))
  {
    ShowWindow(hWnd, SW_RESTORE);
    ::delayed_call([]() {
      if (swap_chain)
      {
        debug("DX11: SetFullscreenState(1)");
        drv3d_dx11::set_fullscreen_state(true);

        if (gamma_control_valid)
        {
          ContextAutoLock contextLock;
          IDXGIOutput *dxgiOutput;
          HRESULT hr = swap_chain->GetContainingOutput(&dxgiOutput);
          if (SUCCEEDED(hr))
          {
            dxgiOutput->SetGammaControl(&gamma_control);
            dxgiOutput->Release();
          }
        }
        dagor_d3d_notify_fullscreen_state_restored = true;
      }
    });
  }
  else
  {
    ShowWindow(hWnd, SW_MINIMIZE);
    ::delayed_call([]() {
      if (swap_chain)
      {
        debug("DX11: SetFullscreenState(0)");
        drv3d_dx11::set_fullscreen_state(false);
      }
    });
  }
}

LRESULT CALLBACK WindowProcProxy(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  switch (message)
  {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: paint_window(hWnd, message, wParam, lParam, main_window_proc); return 1;
    case WM_ACTIVATE:
    case WM_ACTIVATEAPP:
    {
      toggle_fullscreen(hWnd, message, wParam);
      break;
    }
  }

  if (origin_window_proc)
    return CallWindowProcW(origin_window_proc, hWnd, message, wParam, lParam);
  if (main_window_proc)
    return main_window_proc(hWnd, message, wParam, lParam);
  return DefWindowProcW(hWnd, message, wParam, lParam);
}

static bool set_window_params(void *hinst, const char *wcname, int ncmdshow, void *hwnd, void *rwnd, void *icon, const char *title,
  const RenderWindowSettings &s)
{
  RenderWindowParams p = {0};
  p.hinst = hinst;
  p.hwnd = hwnd;
  p.icon = icon;
  p.mainProc = WindowProcProxy;
  p.ncmdshow = ncmdshow;
  p.rwnd = rwnd;
  p.title = title;
  p.wcname = wcname;

  if (!own_window && !origin_window_proc)
    origin_window_proc = (WNDPROC)SetWindowLongPtrW((HWND)p.rwnd, GWLP_WNDPROC, (LONG_PTR)p.mainProc);

  if (!set_render_window_params(p, s))
  {
    return false;
  }

  screen_aspect_ratio = s.aspect;
  resolution.set(s.resolutionX, s.resolutionY);
  main_window_hwnd = (HWND)p.hwnd;
  render_window_hwnd = (HWND)p.rwnd;
  return true;
};

bool d3d::init_driver()
{
  if (d3d_inited)
  {
    D3D_ERROR("Driver is already created");
    return false;
  }

  os_spinlock_init(&dx_context_cs);

  //  d3d_dump_memory_stat = &dx11_video_mem_stat;

  d3d_inited = true;

  return true;
}

void d3d::release_driver()
{
  debug("releasing driver...");
  TEXQL_SHUTDOWN_TEX();

  close_device(false);

  if (own_window)
  {
    if (main_window_hwnd)
    {
      DEBUG_CP();
      DestroyWindow(main_window_hwnd);
    }
  }
  else if (render_window_hwnd && origin_window_proc)
  {
    SetWindowLongPtrW(render_window_hwnd, GWLP_WNDPROC, (LONG_PTR)origin_window_proc);
  }

  origin_window_proc = NULL;
  main_window_hwnd = NULL;
  render_window_hwnd = NULL;

  if (secondary_window_hwnd)
  {
    DestroyWindow(secondary_window_hwnd);
    secondary_window_hwnd = NULL;
  }

#if defined(_TARGET_D3D_MULTI) || defined(_TARGET_WAS_MULTI)
  DEBUG_CP();
  shutdown_hlsl_d3dcompiler_internal();
#endif
  d3d_inited = false;
  gpuThreadId = 0;
  gpuAcquireRefCount = 0;

  os_spinlock_destroy(&dx_context_cs);

  DEBUG_CP();
}

bool d3d::device_lost(bool *can_reset_now)
{
  if (!(dagor_d3d_force_driver_reset || dagor_d3d_force_driver_mode_reset))
  {
    HRESULT presentHr = 0;
    if (use_dxgi_present_test)
    {
      int present_dest_idx = drv3d_dx11::get_present_dest_idx();
      if (present_dest_idx >= 0)
      {
        drv3d_dx11::SwapChainAndHwnd &r = swap_chain_pairs[present_dest_idx];
        RectInt rect = {0, 0, r.w, r.h};
        d3d::stretch_rect(g_driver_state.backBufferColorTex, r.buf, &rect, &rect);
        ContextAutoLock contextLock;
        presentHr = r.sw->Present(0, DXGI_PRESENT_TEST);
      }
      else
      {
        ContextAutoLock contextLock;
        presentHr = swap_chain->Present(_no_vsync ? 0 : 1, DXGI_PRESENT_TEST);
      }
    }
    else
    {
      ContextAutoLock contextLock;
      presentHr = dx_device->GetDeviceRemovedReason();
    }
    device_should_reset(presentHr, "device_lost()");
  }
  if (can_reset_now && (dagor_d3d_force_driver_reset || dagor_d3d_force_driver_mode_reset))
    *can_reset_now = true;
  return dagor_d3d_force_driver_reset || dagor_d3d_force_driver_mode_reset;
}
static bool device_is_being_reset = false;
bool d3d::is_in_device_reset_now() { return device_is_lost != S_OK || device_is_being_reset; }

static IDXGIOutput *get_output_monitor_by_name_or_default(const char *monitorName)
{
  IDXGIOutput *output = nullptr;
  IDXGIAdapter1 *dxgiAdapter = get_active_adapter(dx_device);
  if (dxgiAdapter)
  {
    output = get_output_monitor_by_name_or_default(dx11_DXGIFactory, monitorName).Detach();
    dxgiAdapter->Release();
  }
  return output;
}

static void recreate_gpu_resources()
{
  g_render_state.modified = true;
  for (int i = 0; i < g_render_state.texFetchState.resources.size(); ++i)
    g_render_state.texFetchState.resources[i].modifiedMask = 0xFFFFFFFF;
  FramememResourceSizeInfoCollection resourcesToRecreate;
  gather_textures_to_recreate(resourcesToRecreate);
  gather_buffers_to_recreate(resourcesToRecreate);
  uint32_t reloadCallbacksCount = 0;
  uint32_t tex_to_reload = 0, buf_to_reload = 0;
  for (int i = resourcesToRecreate.size() - 1; i >= 0; --i)
  {
    if (resourcesToRecreate[i].hasRecreationCallback)
    {
      (resourcesToRecreate[i].isTex ? tex_to_reload : buf_to_reload)++;
      eastl::swap(resourcesToRecreate[i], resourcesToRecreate[resourcesToRecreate.size() - 1 - reloadCallbacksCount]);
      reloadCallbacksCount++;
    }
  }
  debug("recreate_gpu_resources: %d tex + %d buf (%d recreatable of %d total)", tex_to_reload, buf_to_reload,
    tex_to_reload + buf_to_reload, resourcesToRecreate.size());
  stlsort::sort(resourcesToRecreate.begin(), resourcesToRecreate.end() - reloadCallbacksCount,
    [](const ResourceSizeInfo &a, const ResourceSizeInfo &b) { return a.sizeInBytes > b.sizeInBytes; });
  for (const ResourceSizeInfo &res : resourcesToRecreate)
  {
    if (res.isTex)
      recreate_texture(res.index);
    else
      recreate_buffer(res.index);
    watchdog_kick();
  }
}

bool d3d::reset_device()
{
  struct RaiiReset
  {
    RaiiReset() { device_is_being_reset = true; }
    ~RaiiReset() { device_is_being_reset = false; }
  } raii_reset;
  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  const DataBlock &blk_dx = *dgs_get_settings()->getBlockByNameEx("directx");
  const char *displayName = get_monitor_name_from_settings();

  if (dagor_d3d_force_driver_reset)
  {
    ScopedLockWrite lock(reset_rw_lock);
    ResAutoLock resLock; // Avoid deadlock with Tex::lock functions.
    resetting_device_now = true;
    if (device_is_lost == S_OK)
      device_is_lost = DXGI_ERROR_DEVICE_RESET;

    for (auto &handler : deviceResetEventHandlers)
      handler->preRecovery();

    save_predicates();

    if (g_device_desc.caps.hasDLSS)
    {
      release_dlss_feature();
    }

    close_device(true);
    debug("sysMem used for res copy: tex=%dK vb=%dK ib=%dK", texture_sysmemcopy_usage >> 10, vbuffer_sysmemcopy_usage >> 10,
      ibuffer_sysmemcopy_usage >> 10);

    if (!init_device(stereo_config_callback, main_window_hwnd, resolution.x, resolution.y,
          blk_video.getBool("bits16", false) ? 16 : 32, displayName, blk_dx.getInt64("adapterLuid", 0), "DagorWClass", true))
    {
      DAG_FATAL("Error initializing device(%s):\n%s", d3d::get_driver_name(), d3d::get_last_error());
      resetting_device_now = false;
      return false;
    }

    if (g_device_desc.caps.hasDLSS)
      create_dlss_feature(resolution);

    init_all(blk_dx.getInt("inline_vb_size", INLINE_VB_SIZE), blk_dx.getInt("inline_ib_size", INLINE_IB_SIZE));
    print_memory_stat();
    recreate_gpu_resources();
    recreate_all_queries();
    recreate_predicates();
    recreate_render_states();
    g_render_state.reset();

    for (auto &handler : deviceResetEventHandlers)
      handler->recovery();

    resetting_device_now = false;
  }
  else if (dagor_d3d_force_driver_mode_reset)
  {
    // don't show any window messages when we are in game
    bool savedExecuteQuiet = dgs_execute_quiet;
    dgs_execute_quiet = true;
    RenderWindowSettings wndSettings;
    get_render_window_settings(wndSettings, stereo_config_callback);
    dgs_execute_quiet = savedExecuteQuiet;

    // set window params first to avoid glitches
    // caused by changing swapchain output
    if (!set_window_params(nullptr, nullptr, 0, main_window_hwnd, render_window_hwnd, nullptr, nullptr, wndSettings))
    {
      DAG_FATAL("Error with set_window_params in reseting device");
      return false;
    }

    RenderState &rs = g_render_state;
    DriverState &ds = g_driver_state;

    DEBUG_CTX("deleted backbuffer %p", ds.backBufferColorTex);
    del_d3dres(ds.backBufferColorTex);

    close_states();

    bool fullscreen = dgs_get_window_mode() == WindowMode::FULLSCREEN_EXCLUSIVE;

    // specifies that the window's show state must be changed when we first enter to the fullscreen from windowed mode
    // otherwise some issues with clipping cursor may occur because destop and fullscreen resolutions can be different
    bool fullscreenWindowStateChanged =
      _in_win_started && fullscreen &&
      (wndSettings.resolutionX != GetSystemMetrics(SM_CXSCREEN) || wndSettings.resolutionY != GetSystemMetrics(SM_CYSCREEN));

    // minimize a window before the reset
    if (fullscreenWindowStateChanged)
      ShowWindow(render_window_hwnd, SW_MINIMIZE);

    IDXGIOutput *output = get_output_monitor_by_name_or_default(displayName);
    if (target_output && target_output != output)
      target_output->Release();
    target_output = output;


    swap_chain->SetFullscreenState(fullscreen, fullscreen ? target_output : nullptr);

    scd.BufferDesc.Width = wndSettings.resolutionX;
    scd.BufferDesc.Height = wndSettings.resolutionY;

    swap_chain->ResizeTarget(&scd.BufferDesc);

    set_nvapi_vsync(dx_device);

    // Only Nvidia modes are disabled
    const lowlatency::LatencyMode latencyMode = lowlatency::get_from_blk();
    _no_vsync = !blk_video.getBool("vsync", false) ||
                (latencyMode == lowlatency::LATENCY_MODE_NV_ON || latencyMode == lowlatency::LATENCY_MODE_NV_BOOST);

    find_closest_matching_mode();

    DXFATAL(swap_chain->ResizeBuffers(scd.BufferCount, scd.BufferDesc.Width, scd.BufferDesc.Height, scd.BufferDesc.Format, scd.Flags),
      "RSB");
    ds.createSurfaces(scd.BufferDesc.Width, scd.BufferDesc.Height);

    recreate_render_states();

    if (streamlineAdapter && streamlineAdapter->isDlssSupported() == nv::SupportState::Supported)
    {
      release_dlss_feature();
    }

    streamlineAdapter.reset();
    StreamlineAdapter::init(streamlineAdapter, StreamlineAdapter::RenderAPI::DX11);
    if (streamlineAdapter)
    {
      dx_device = streamlineAdapter->hook(dx_device);
      streamlineAdapter->setAdapterAndDevice(get_active_adapter(dx_device), dx_device);
      streamlineAdapter->initializeDlssState();
      if (streamlineAdapter->isDlssSupported() == nv::SupportState::Supported)
      {
        g_device_desc.caps.hasDLSS = true;
        create_dlss_feature(resolution);
      }
      nvlowlatency::init();
    }

    // restore a minimized window after the reset to fix client and clipping regions for the cursor
    if (fullscreenWindowStateChanged)
      ShowWindow(render_window_hwnd, SW_RESTORE);

    // SetFullscreenState can somehow interfere with window params, need to set these again
    if (!set_window_params(nullptr, nullptr, 0, main_window_hwnd, render_window_hwnd, nullptr, nullptr, wndSettings))
    {
      DAG_FATAL("Error with set_window_params in reseting device");
      return false;
    }
    ShowWindow(render_window_hwnd, SW_SHOW); // justincase
  }

  dagor_d3d_force_driver_reset = false;
  dagor_d3d_force_driver_mode_reset = false;

  return true;
}

bool d3d::is_inited() { return d3d_inited && dx_device && dx_context; }

bool d3d::init_video(void *hinst, main_wnd_f *mwf, const char *wcname, int ncmdshow, void *&hwnd, void *rwnd, void *icon,
  const char *title, Driver3dInitCallback *cb)
{
  gpuThreadId = ::GetCurrentThreadId();
  gpuAcquireRefCount = 0;

  G_ASSERT(BAD_FSHADER == BAD_HANDLE);
  G_ASSERT(BAD_VDECL == BAD_HANDLE);
  G_ASSERT(BAD_VPROG == BAD_HANDLE);

  //  G_ASSERT(is_main_thread());

  main_window_proc = mwf;

  const GpuDriverConfig &gpuCfg = get_gpu_driver_cfg();

  const DataBlock &blk_video = *dgs_get_settings()->getBlockByNameEx("video");
  const DataBlock &blk_dx = *dgs_get_settings()->getBlockByNameEx("directx");
  drv3d_dx11::immutable_textures = blk_dx.getBool("immutable_textures", true);
  drv3d_dx11::window_occlusion_check_enabled = blk_dx.getBool("winOcclusionCheckEnabled", true);
  debug("immutable textures = %d", (int)immutable_textures);

  // debug("re sc %d %d",scr_wd,scr_ht);
  int screenBpp = blk_video.getBool("bits16", false) ? 16 : 32;

  // Only Nvidia modes are disabled
  const lowlatency::LatencyMode latencyMode = lowlatency::get_from_blk();
  _no_vsync = !blk_video.getBool("vsync", false) ||
              (latencyMode == lowlatency::LATENCY_MODE_NV_ON || latencyMode == lowlatency::LATENCY_MODE_NV_BOOST);
  own_window = (rwnd == NULL);

  RenderWindowSettings wndSettings;
  get_render_window_settings(wndSettings, cb);
  if (!set_window_params(hinst, wcname, ncmdshow, hwnd, rwnd, icon, title, wndSettings))
    return false;

  hwnd = main_window_hwnd;

  int64_t cbPreferredLuid = cb ? cb->desiredAdapter() : 0;

  const char *displayName = cbPreferredLuid ? nullptr : get_monitor_name_from_settings();
  int64_t adapterLuild = cbPreferredLuid ? cbPreferredLuid : blk_dx.getInt64("adapterLuid", 0);
  flush_on_present = blk_dx.getBool("flushOnPresent", false);
  flush_before_survey = gpuCfg.flushBeforeSurvey;
  use_gpu_dt = blk_dx.getBool("useGpuDt", true);

  use_dxgi_present_test =
    blk_dx.getBool("useDxgiPresentTest", false) &&
    GetModuleHandle("d3dGearLoad.dll") == NULL /* D3DGear 2015.05.30 4.9.5.1937 confuses DXGI_PRESENT_TEST with actual present.*/;

  debug("use_dxgi_present_test = %d", (int)use_dxgi_present_test);
  debug("screen = %dx%d", resolution.x, resolution.y);
  if (!init_device(cb, main_window_hwnd, resolution.x, resolution.y, screenBpp, displayName, adapterLuild, wcname))
  {
    D3D_ERROR("can't create device");
    close_device(false);
    return false;
  }

  init_all(blk_dx.getInt("inline_vb_size", INLINE_VB_SIZE), blk_dx.getInt("inline_ib_size", INLINE_IB_SIZE));

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

  d3d::set_render_target();
  int rtw = 1, rth = 1;
  d3d::get_render_target_size(rtw, rth, NULL);
  d3d::setview(0, 0, rtw, rth, 0.0, 1.0);
  d3d::setpersp(Driver3dPerspective(1.3, 1.3 * rtw / rth, 0.1, 100.0, 0.f, 0.f));

  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  return true;
}


DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::dx11); }
const char *d3d::get_driver_name() { return "DirectX 11"; }

const char *d3d::get_device_driver_version() { return "1.0"; }

const char *d3d::get_device_name() { return deviceName; }

const char *d3d::get_last_error()
{
  return dx11_error(last_hres);
  //  sprintf ( error_buf+strlen(error_buf), " in %s,%d", error_file, error_line );
  //  return error_buf;
}

uint32_t d3d::get_last_error_code() { return last_hres; }

const Driver3dDesc &d3d::get_driver_desc() { return g_device_desc; }

void *d3d::get_device() { return drv3d_dx11::dx_device; }

void d3d::prepare_for_destroy()
{
  drv3d_dx11::set_fullscreen_state(false);
  on_before_window_destroyed();
}

void d3d::window_destroyed(void *hwnd)
{
  if (hwnd && hwnd == main_window_hwnd)
    main_window_hwnd = NULL;
}

ShaderLibrary d3d::create_shader_library(const ShaderLibraryCreateInfo &) { return InvalidShaderLibrary; }

void d3d::destroy_shader_library(ShaderLibrary) {}

#include "../drv3d_commonCode/rayTracingStub.inc.cpp"
