// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tiledResource.h>
#include <drv/3d/dag_heap.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_query.h>
#include <drv/3d/dag_res.h>
#include <drv/3d/dag_tex3d.h>
#include <math/dag_Point2.h>
#include <generic/dag_span.h>
#include <generic/dag_tabWithLock.h>
#include <util/dag_string.h>
#include <math/dag_half.h>
#include "driver.h"
#include "texture.h"
#include "predicates.h"
#include "../drv3d_commonCode/drv_utils.h"
#include <3d/dag_nvLowLatency.h>
#include <3d/dag_lowLatency.h>
#include <drv/3d/dag_info.h>

extern void set_aftermath_marker(const char *, bool allocate_copy);

using namespace drv3d_dx11;

#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>

#include <windows.h>
#include <3d/ddsFormat.h>
#include <dxgi1_6.h>
#include <RenderDoc/renderdoc_app.h>

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_interface_table.h>
#include <drv/3d/dag_res.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_commands.h>
#include <3d/ddsxTex.h>

#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_TMatrix.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>

#include <startup/dag_globalSettings.h>
#include <perfMon/dag_cpuFreq.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>

#include <ioSys/dag_memIo.h>
#include <ioSys/dag_genIo.h>

#include <debug/dag_debug.h>
#include <debug/dag_log.h>

#include <3d/dag_lowLatency.h>

#if HAS_NVAPI
#include <nvapi.h>
#endif

#include "helpers.h"
#include "../drv3d_commonCode/dxgi_utils.h"
#include <resUpdateBufferGeneric.h>
#include <resourceActivationGeneric.h>


enum LocalTm
{
  LOCAL_TM_WORLD = 0,
  LOCAL_TM_VIEW,
  LOCAL_TM_PROJ,

  LOCAL_TM__NUM
};

static const char statistics[] = "<stat>";
Tab<drv3d_dx11::SwapChainAndHwnd> drv3d_dx11::swap_chain_pairs;
static int present_dest_idx = -1;

#define MIN_LISTED_MODE_WIDTH  800
#define MIN_LISTED_MODE_HEIGHT 600

static constexpr int SWAPCHAIN_WAIT_TIMEOUT = 1000;

namespace drv3d_dx11
{
eastl::vector<FrameEvents *> frontEndFrameCallbacks;
eastl::vector<FrameEvents *> backEndFrameCallbacks;
eastl::vector<DeviceResetEventHandler *> deviceResetEventHandlers;

int get_present_dest_idx() { return present_dest_idx; }

struct DX11Query
{
  D3D11_QUERY type;
  ID3D11Query *query;
  DX11Query() : query(NULL) {} //-V730
  DX11Query(ID3D11Query *query_ptr, D3D11_QUERY query_type) : query(query_ptr), type(query_type) {}
};

static TabWithLock<DX11Query> all_queries;

static ID3D11Query *create_raw_query(D3D11_QUERY query_type)
{
  ID3D11Query *pQuery = NULL;
  D3D11_QUERY_DESC desc;
  desc.Query = query_type;
  desc.MiscFlags = 0;
  if (dx_device->CreateQuery(&desc, &pQuery) != S_OK)
    return NULL;
  return pQuery;
}

static int create_query(D3D11_QUERY query_type)
{
  ID3D11Query *pQuery = create_raw_query(query_type);
  if (!pQuery)
    return 0;

  DX11Query query(pQuery, query_type);
  all_queries.lock();
  for (int i = 1; i < all_queries.size(); ++i)
  {
    if (all_queries[i].type == (D3D11_QUERY)0xFFFFFFFF) // -V1016 //-V1084
    {
      G_ASSERT(!all_queries[i].query);
      all_queries[i] = query;
      all_queries.unlock();
      return i;
    }
  }
  if (!all_queries.size())
    all_queries.push_back(DX11Query());
  int id = all_queries.size();
  all_queries.push_back(query);
  all_queries.unlock();
  return id;
}

static void release_query(int query)
{
  if (query <= 0)
    return;
  all_queries.lock();
  if (all_queries[query].query)
    all_queries[query].query->Release();
  all_queries[query].query = NULL;
  static_assert(sizeof(D3D11_QUERY) == 4, "D3D11_QUERY size is not 32 bit!");
  all_queries[query].type = (D3D11_QUERY)0xFFFFFFFF; // -V1016 -V1084_TURN_OFF_ON_MSVC
  all_queries.unlock();
}

void recreate_all_queries()
{
  all_queries.lock();
  for (int i = 1; i < all_queries.size(); ++i)
  {
    static_assert(sizeof(D3D11_QUERY) == 4, "D3D11_QUERY size is not 32 bit!");
    if (all_queries[i].type == (D3D11_QUERY)0xFFFFFFFF) // -V1016 //-V1084 -V1084_TURN_OFF_ON_MSVC
      continue;
    G_ASSERT(all_queries[i].query);
    D3D11_QUERY_DESC desc;
    desc.Query = all_queries[i].type;
    desc.MiscFlags = 0;
    if (dx_device->CreateQuery(&desc, &all_queries[i].query) != S_OK)
      D3D_ERROR("can not recreate query");
  }
  all_queries.unlock();
}

void reset_all_queries()
{
  all_queries.lock();
  for (int i = 1; i < all_queries.size(); ++i)
    if (all_queries[i].query)
      all_queries[i].query->Release();
  all_queries.unlock();
}

void print_memory_stat()
{
  ComPtr<IDXGIAdapter3> adapter3;
  if (auto adptr = get_active_adapter(dx_device); adptr && SUCCEEDED(adptr->QueryInterface(COM_ARGS(&adapter3))))
  {
    DXGI_QUERY_VIDEO_MEMORY_INFO info;
    if ((SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)) && info.Budget > 0) ||
        (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info)) && info.Budget > 0))
    {
      debug("GPU memory budget: %d MB", int(info.Budget >> 20));
      debug("GPU memory usage: %d MB", int(info.CurrentUsage >> 20));
      debug("GPU memory current reservation: %d MB", int(info.CurrentReservation >> 20));
      debug("GPU memory available reservation: %d MB", int(info.AvailableForReservation >> 20));
    }
  }
}

void dump_memory_if_needed(HRESULT hr)
{
  if (hr == E_OUTOFMEMORY)
  {
    static bool reportOOM = true;
    if (reportOOM)
    {
      reportOOM = false;
      String texStat;
      d3d::get_texture_statistics(nullptr, nullptr, &texStat);
      debug("GPU MEMORY STATISTICS:\n======================");
      debug(texStat.str());
      debug("======================\nEND GPU MEMORY STATISTICS");
      print_memory_stat();
    }
    else
      debug("GPU MEMORY STATISTICS: not reported as this is not the first time.");
  }
}

extern bool d3d_inited;
}; // namespace drv3d_dx11

extern bool aftermath_inited();
static String after_math_context;
static Tab<int> after_math_contexts_len;
static bool store_aftermath_context = true;
bool should_store_aftermath_context() { return store_aftermath_context; }

static void push_aftermath_context(const char *name)
{
  if (!aftermath_inited())
    return;
  if (!should_store_aftermath_context())
  {
    set_aftermath_marker(name, false);
    return;
  }
  const int len = (int)strlen(name) + 1;
  after_math_contexts_len.push_back(len);
  after_math_context.aprintf(256, "%s/", name);
  set_aftermath_marker((const char *)after_math_context, true);
}

static void pop_aftermath_context()
{
  if (after_math_contexts_len.size() == 0 || !aftermath_inited())
    return;
  const int len = after_math_contexts_len.back();
  after_math_contexts_len.pop_back();
  erase_items(after_math_context, after_math_context.size() - len - 1, len);
  set_aftermath_marker((const char *)after_math_context, true);
}

extern void get_mem_stat(String *out_str);

static Tab<String> get_resolutions_from_output(IDXGIOutput *dxgiOutput)
{
  Tab<String> resolutions;
  UINT numModes = 0;
  DXGI_MODE_DESC *displayModes = nullptr;
  DXGI_FORMAT format = DXGI_FORMAT_R32G32B32A32_FLOAT;

  HRESULT hr = dxgiOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &numModes, nullptr);

  if (SUCCEEDED(hr))
  {
    Tab<DXGI_MODE_DESC> displayModes;
    displayModes.resize(numModes);
    hr = dxgiOutput->GetDisplayModeList(DXGI_FORMAT_B8G8R8A8_UNORM, 0, &numModes, displayModes.begin());

    if (SUCCEEDED(hr))
    {
      auto recommended_resolution = get_recommended_resolution(dxgiOutput);

      for (int modeNo = 0; modeNo < numModes; modeNo++)
      {
        // first drop modes that are to small
        if (displayModes[modeNo].Width < MIN_LISTED_MODE_WIDTH || displayModes[modeNo].Height < MIN_LISTED_MODE_HEIGHT)
        {
          continue;
        }

        // drop modes with same width and height
        int testModeNo = 0;
        for (; testModeNo < modeNo; testModeNo++)
        {
          if (displayModes[testModeNo].Width == displayModes[modeNo].Width &&
              displayModes[testModeNo].Height == displayModes[modeNo].Height)
          {
            break;
          }
        }
        if (testModeNo != modeNo)
          continue;

        if (recommended_resolution)
        {
          // remove modes larger than recommended mode
          if (
            displayModes[modeNo].Width > recommended_resolution->first || displayModes[modeNo].Height > recommended_resolution->second)
            continue;

          // remove modes with differect aspect ratio than recommended
          if (!resolutions_have_same_ratio(*recommended_resolution, {displayModes[modeNo].Width, displayModes[modeNo].Height}))
            continue;
        }

        // add a suitable new mode
        resolutions.push_back(String(64, "%d x %d", displayModes[modeNo].Width, displayModes[modeNo].Height));
      }
    }
  }
  return resolutions;
}

static bool is_swapchain_window_occluded()
{
  if (!drv3d_dx11::window_occlusion_check_enabled)
    return false;

  TIME_PROFILE_NAME(checkOcclusion, "check window occlusion");
  DXGI_SWAP_CHAIN_DESC desc;
  swap_chain->GetDesc(&desc);
  return is_window_occluded(desc.OutputWindow);
}

static bool occluded_window = false;

// NOTE: our driver lockage logic is insanely complicated, no chance for thread safety analysis to help us here.
int d3d::driver_command(Drv3dCommand command, void *par1, void *par2, [[maybe_unused]] void *par3) DAG_TS_NO_THREAD_SAFETY_ANALYSIS
{
  if (!drv3d_dx11::d3d_inited)
    return 0;

  switch (command)
  {
    case Drv3dCommand::PROCESS_APP_INACTIVE_UPDATE:
    {
      if (occluded_window && !is_swapchain_window_occluded())
        occluded_window = false;
      if (par1)
      {
        bool *occ_window = reinterpret_cast<bool *>(par1);
        *occ_window = occluded_window;
      }
      return 1;
    }

    case Drv3dCommand::AFTERMATH_MARKER:
    {
      if (aftermath_inited())
      {
        if (after_math_context.size() > 1)
        {
          char buf[512];
          SNPRINTF(buf, sizeof(buf), "%s%s", (const char *)after_math_context, (const char *)par1);
          set_aftermath_marker(buf, true);
        }
        else
          set_aftermath_marker((const char *)par1, par2 != NULL);
      }
      return 1;
    }

    case Drv3dCommand::FLUSH_STATES:
    {
      drv3d_dx11::flush_all(false);
      return 1;
    }

    case Drv3dCommand::ACQUIRE_OWNERSHIP: acquireGpu(); break;

    case Drv3dCommand::RELEASE_OWNERSHIP: releaseGpu(); break;

    case Drv3dCommand::ACQUIRE_LOADING:
      if (par1)
        reset_rw_lock.lockWrite();
      else
        reset_rw_lock.lockRead();
      break;

    case Drv3dCommand::RELEASE_LOADING:
      if (par1)
        reset_rw_lock.unlockWrite();
      else
        reset_rw_lock.unlockRead();
      break;

    case Drv3dCommand::GETVISIBILITYBEGIN:
    {
      if (!dx_context)
        return 0;

      if (!g_device_desc.caps.hasOcclusionQuery)
        return 0;

      int *par11 = (int *)par1;
      int &pQuery = *par11;
      if (!pQuery)
        pQuery = create_query(D3D11_QUERY_OCCLUSION);
      if (pQuery > 0)
      {
        ContextAutoLock contextLock;
        dx_context->Begin(all_queries[pQuery].query);
      }

      return 0;
    }

    case Drv3dCommand::GETVISIBILITYEND:
    {
      if (!par1)
        return 0;
      int pQuery = (int)(intptr_t)par1;
      ContextAutoLock contextLock;
      dx_context->End(all_queries[pQuery].query);
      return 0;
    }

    case Drv3dCommand::GETVISIBILITYCOUNT:
    {
      DWORD res = 0;
      if (!par1)
        return 0;
      ID3D11Query *pQuery = all_queries[(int)(intptr_t)par1].query;
      HRESULT hr;
      uint64_t data;
      ContextAutoLock contextLock;
      if ((hr = dx_context->GetData(pQuery, &data, sizeof(data), par2 ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH)) != S_OK)
        return -1;
      else
        return (int)data;
    }

    case Drv3dCommand::RELEASE_QUERY:
    {
      // if (!dx_context)
      //   return 0;
      int *par11 = (int *)par1;
      int &pQuery = *par11;
      release_query(pQuery);
      pQuery = 0;
      return 0;
    }

    case Drv3dCommand::TIMESTAMPFREQ:
    {
      ///    returns 1 if everything is ok, otherwise returns 0
      ///    if par3 is nullptr
      ///      // sync implementation with active waiting
      ///      par1 will be frequency int64_t
      ///      disjoints are ignored
      ///    else
      ///      // async implementation with swapchain
      ///      par3 is pQuery[2] array
      ///      if par3 is array with zeroes
      ///        init queries
      ///      else
      ///        read query
      ///        par1 will be frequency int64_t
      ///        par2 will be disjoint bool
      ///        end query, start new

      if (par3 == nullptr)
      {
        int result = 0;
        ID3D11Query *pQuery = create_raw_query(D3D11_QUERY_TIMESTAMP_DISJOINT);
        if (pQuery)
        {
          ContextAutoLock contextLock;
          dx_context->Begin(pQuery);
          dx_context->End(pQuery);
          for (int count = 0; count < 120; ++count)
          {
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
            if (dx_context->GetData(pQuery, &data, sizeof(data), 0) == S_OK)
            {
              *(uint64_t *)par1 = data.Frequency;
              result = 1;
              break;
            }
            else
              Sleep(1);
          }
        }
        if (pQuery)
          pQuery->Release();
        return result;
      }
      else
      {
        int(*par33)[2] = (int(*)[2])par3;
        int(&pQueries)[2] = *par33;
        if (pQueries[0] <= 0 || pQueries[1] <= 0)
        {
          pQueries[0] = create_query(D3D11_QUERY_TIMESTAMP_DISJOINT);
          pQueries[1] = create_query(D3D11_QUERY_TIMESTAMP_DISJOINT);
          if (pQueries[0] && pQueries[1])
          {
            ContextAutoLock contextLock;
            dx_context->Begin(all_queries[pQueries[0]].query);
            dx_context->End(all_queries[pQueries[0]].query);
            dx_context->Begin(all_queries[pQueries[1]].query);
            dx_context->End(all_queries[pQueries[1]].query);
          }
        }
        else if (all_queries[pQueries[0]].query && all_queries[pQueries[1]].query)
        {
          ContextAutoLock contextLock;
          D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data;
          if (dx_context->GetData(all_queries[pQueries[0]].query, &data, sizeof(data), 0) == S_OK)
          {
            *(uint64_t *)par1 = data.Frequency;
            *(bool *)par2 = data.Disjoint;
            dx_context->Begin(all_queries[pQueries[0]].query);
            dx_context->End(all_queries[pQueries[1]].query);

            // swap
            int t = pQueries[1];
            pQueries[1] = pQueries[0];
            pQueries[0] = t;

            return 1;
          }
        }
        return 0;
      }
    }
    case Drv3dCommand::TIMESTAMPISSUE:
    {
      int *par11 = (int *)par1;
      int &pQuery = *par11;
      if (pQuery <= 0)
        pQuery = create_query(D3D11_QUERY_TIMESTAMP);
      if (pQuery)
      {
        ContextAutoLock contextLock;
        dx_context->End(all_queries[pQuery].query);
      }
      return 1;
    }
    case Drv3dCommand::TIMESTAMPGET:
    {
      ///    returns 0 if not ready, otherwise 1
      ///    par2 will be timestamp int64_t
      if (!par1 || !par2)
        return 0;
      int pQuery = (int)(intptr_t)par1;
      if (pQuery >= 0 && all_queries[pQuery].query)
      {
        ContextAutoLock contextLock;
        return dx_context->GetData(all_queries[pQuery].query, par2, sizeof(uint64_t), 0) == S_OK ? 1 : 0;
      }
      return 0;
    }
    case Drv3dCommand::TIMECLOCKCALIBRATION:
    {
      // May be it would be more precisely with DXGI_FRAME_STATISTICS / DXGIX_FRAME_STATISTICS
      ID3D11Query *pQuery = create_raw_query(D3D11_QUERY_TIMESTAMP);
      if (pQuery)
      {
        ContextAutoLock contextLock;
        dx_context->End(pQuery);
        while (dx_context->GetData(pQuery, par2, sizeof(uint64_t), 0) != S_OK)
          ; // VOID
        *(int64_t *)par1 = ref_time_ticks();
        if (par3)
          *(int *)par3 = DRV3D_CPU_FREQ_TYPE_REF;
        pQuery->Release();
        return 1;
      }
      return 0;
    }


    case Drv3dCommand::GET_GPU_FRAME_TIME:
    {
      // par1 - dt output, par2 - optional CPU dt to compare with.

      if (!par1)
        return 0;

      if (par2)
        *(float *)par1 = *(float *)par2;

      if (gpu_frame_time > 0.f)
      {
        DXGI_FRAME_STATISTICS frameStatistics;
        HRESULT hr = swap_chain->GetFrameStatistics(&frameStatistics);
        if (SUCCEEDED(hr))
        {
          if (par2) // Choose the smoothest dt.
          {
            bool useGpuDt;
            select_best_dt(*(float *)par2, gpu_frame_time, *(float *)par1, useGpuDt);
            return useGpuDt;
          }
          else // Only return GPU dt.
          {
            *(float *)par1 = gpu_frame_time;
            return 1;
          }
        }
      }
      return 0;
    }

    case Drv3dCommand::GET_VSYNC_REFRESH_RATE:
    {
      *(double *)par1 = *(double *)&vsync_refresh_rate;
      return 1;
    }

    case Drv3dCommand::GET_SECONDARY_BACKBUFFER:
    {
      if (par1)
        *(Texture **)par1 = secondary_backbuffer_color_tex;
      return secondary_backbuffer_color_tex ? 1 : 0;
    }

    case Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL:
    {
      override_max_anisotropy_level = (int)(intptr_t)par1; //-V542
      return 1;
    }

    case Drv3dCommand::MAKE_TEXTURE:
    {
      const Drv3dMakeTextureParams *makeParams = (const Drv3dMakeTextureParams *)par1;
      ID3D11Texture2D *d3dTex = static_cast<ID3D11Texture2D *>(makeParams->tex);
      d3dTex->AddRef();
      *(Texture **)par2 = create_d3d_tex(d3dTex, makeParams->name, makeParams->flg);
      return 1;
    }

    case Drv3dCommand::REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS:
    {
      FrameEvents *frameEvents = static_cast<FrameEvents *>(par1);
      const bool useFront = !!par2;
      frameEvents->beginFrameEvent();
      auto &selectedCallbackList = useFront ? frontEndFrameCallbacks : backEndFrameCallbacks;
      selectedCallbackList.emplace_back(frameEvents);
      return 1;
    }

    case Drv3dCommand::REGISTER_DEVICE_RESET_EVENT_HANDLER:
    {
      DeviceResetEventHandler *handler = static_cast<DeviceResetEventHandler *>(par1);
      auto it = eastl::find(eastl::begin(deviceResetEventHandlers), eastl::end(deviceResetEventHandlers), handler);
      if (it == eastl::end(deviceResetEventHandlers))
        deviceResetEventHandlers.emplace_back(handler);
      else
        G_ASSERTF(false, "Device reset event handler is already registered.");
      return 1;
    }

    case Drv3dCommand::UNREGISTER_DEVICE_RESET_EVENT_HANDLER:
    {
      DeviceResetEventHandler *handler = static_cast<DeviceResetEventHandler *>(par1);
      auto it = eastl::find(eastl::begin(deviceResetEventHandlers), eastl::end(deviceResetEventHandlers), handler);
      if (it != eastl::end(deviceResetEventHandlers))
        deviceResetEventHandlers.erase(it);
      else
        G_ASSERTF(false, "Could not find device reset event handler to unregister.");
      return 1;
    }

    case Drv3dCommand::GET_TEXTURE_HANDLE:
    {
      const Texture *texture = (const Texture *)par1;
      (*(void **)par2) = ((BaseTex *)texture)->tex.texRes;
      return 1;
    }

    case Drv3dCommand::BEGIN_EXTERNAL_ACCESS:
    {
      os_spinlock_lock(&dx_context_cs);
      return 1;
    }

    case Drv3dCommand::END_EXTERNAL_ACCESS:
    {
      os_spinlock_unlock(&dx_context_cs);
      return 1;
    }

    case Drv3dCommand::BEGIN_GENERIC_RENDER_PASS_CHECKS:
    {
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
      ContextAutoLock contextLock;
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive, "DX11: Nested generic render pass detected");
      g_render_state.isGenericRenderPassActive = true;
#endif
      return 1;
    }

    case Drv3dCommand::END_GENERIC_RENDER_PASS_CHECKS:
    {
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
      ContextAutoLock contextLock;
      VALIDATE_GENERIC_RENDER_PASS_CONDITION(g_render_state.isGenericRenderPassActive,
        "DX11: End generic render pass called without a begin call");
      g_render_state.isGenericRenderPassActive = false;
#endif
      return 1;
    }

    case Drv3dCommand::D3D_FLUSH:
    {
      TIME_D3D_PROFILE(Drv3dCommand::D3D_FLUSH);
      auto query = create_event_query();
      issue_event_query(query);
      {
        ContextAutoLock lock;
        dx_context->Flush();
      }
      while (!get_event_query_status(query, true)) {}
      release_event_query(query);
      return 1;
    }

    case Drv3dCommand::SET_VS_DEBUG_INFO:
    {
      set_vertex_shader_debug_info(*(VPROG *)par1, (const char *)par2);
      return 1;
    }

    case Drv3dCommand::SET_PS_DEBUG_INFO:
    {
      set_pixel_shader_debug_info(*(FSHADER *)par1, (const char *)par2);
      return 1;
    }

    case Drv3dCommand::IS_HDR_AVAILABLE:
    {
      bool hdrAvailable = false;

      // If the display's advanced color state has changed (e.g. HDR display plug/unplug, or OS HDR setting on/off),
      // then DXGI factory is invalidated and must be created anew in order to retrieve up-to-date display information.
      // GetContainingOutput will return a stale dxgi output once DXGIFactory::IsCurrent() is false. In addition,
      // re-creating swapchain to resolve the stale dxgi output will cause a short period of black screen.

      HMODULE dxgiDll = LoadLibraryA("dxgi.dll");
      if (dxgiDll)
      {
        HRESULT hr;
        typedef HRESULT(WINAPI * LPCREATEDXGIFACTORY1)(REFIID, void **);
        LPCREATEDXGIFACTORY1 pCreateDXGIFactory1 = NULL;
        pCreateDXGIFactory1 = (LPCREATEDXGIFACTORY1)GetProcAddress(dxgiDll, "CreateDXGIFactory1");
        if (pCreateDXGIFactory1)
        {
          IDXGIFactory1 *dxgiFactory = NULL;
          hr = pCreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)&dxgiFactory);
          if (SUCCEEDED(hr) && dxgiFactory)
          {
            for (uint32_t adapterIndex = 0;; adapterIndex++)
            {
              IDXGIAdapter1 *dxgiAdapter = NULL;
              hr = dxgiFactory->EnumAdapters1(adapterIndex, &dxgiAdapter);
              if (FAILED(hr))
                break;

              for (uint32_t outputIndex = 0;; outputIndex++)
              {
                IDXGIOutput *dxgiOutput;
                hr = dxgiAdapter->EnumOutputs(outputIndex, &dxgiOutput);
                if (FAILED(hr))
                  break;

                IDXGIOutput6 *dxgiOutput6 = NULL;
                hr = dxgiOutput->QueryInterface(&dxgiOutput6);
                if (SUCCEEDED(hr))
                {
                  DXGI_OUTPUT_DESC1 outputDesc1 = {0};
                  dxgiOutput6->GetDesc1(&outputDesc1);

                  bool avalableOnThisOutput = false;
                  if (outputDesc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)
                    hdrAvailable = avalableOnThisOutput = true;
                  debug("HDR %savailable on %d/%d (%dx%d), outputColorSpace=%d", avalableOnThisOutput ? "" : "not ", adapterIndex,
                    outputIndex, outputDesc1.DesktopCoordinates.right - outputDesc1.DesktopCoordinates.left,
                    outputDesc1.DesktopCoordinates.bottom - outputDesc1.DesktopCoordinates.top, outputDesc1.ColorSpace);

                  dxgiOutput6->Release();
                }
                dxgiOutput->Release();
              }
              dxgiAdapter->Release();
            }
            dxgiFactory->Release();
          }
        }
        FreeLibrary(dxgiDll);
      }

      return (int)hdrAvailable;
    }

    case Drv3dCommand::IS_HDR_ENABLED:
    {
      return (int)hdr_enabled;
    }

    case Drv3dCommand::INT10_HDR_BUFFER:
    {
      DXGI_SWAP_CHAIN_DESC desc;
      return SUCCEEDED(swap_chain->GetDesc(&desc)) &&
             (desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UNORM || desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_UINT ||
               desc.BufferDesc.Format == DXGI_FORMAT_R10G10B10A2_TYPELESS);
    }

    case Drv3dCommand::HDR_OUTPUT_MODE:
    {
      if (hdr_enabled)
      {
        return (int)HdrOutputMode::HDR_ONLY;
      }
      return (int)HdrOutputMode::SDR_ONLY;
    }

    case Drv3dCommand::GET_LUMINANCE:
    {
      return 0; // -V1037
    }

    case Drv3dCommand::MEM_STAT:
    {
      get_mem_stat((String *)par1);
      return 1;
    }

    case Drv3dCommand::PIX_GPU_BEGIN_CAPTURE:
    {
      if (docAPI)
        docAPI->StartFrameCapture(nullptr, nullptr);
      return 0;
    }

    case Drv3dCommand::PIX_GPU_END_CAPTURE:
    {
      if (docAPI)
        docAPI->EndFrameCapture(nullptr, nullptr);
      return 0;
    }

    case Drv3dCommand::PIX_GPU_CAPTURE_NEXT_FRAMES:
    {
      if (docAPI)
        docAPI->TriggerMultiFrameCapture(reinterpret_cast<uintptr_t>(par3));
      return 0;
    }

    case Drv3dCommand::EXECUTE_DLSS:
    {
      const nv::DlssParams<BaseTexture> *dlssParams = static_cast<nv::DlssParams<BaseTexture> *>(par1);
      int viewIndex = par2 ? *(int *)par2 : 0;
      AutoAcquireGpu gpuLock;
      ContextAutoLock contextLock;
      streamlineAdapter->evaluateDlss(streamlineAdapter->getFrameId(), viewIndex,
        nv::convertDlssParams(*dlssParams,
          [](BaseTexture *i) { return i ? static_cast<void *>(static_cast<BaseTex *>(i)->tex.texRes) : nullptr; }),
        dx_context);
      return 1;
    }

    case Drv3dCommand::EXECUTE_DLSS_G:
    {
      const nv::DlssGParams<BaseTexture> *dlssGParams = static_cast<nv::DlssGParams<BaseTexture> *>(par1);
      int viewIndex = par2 ? *(int *)par2 : 0;
      ContextAutoLock contextLock;
      streamlineAdapter->evaluateDlssG(viewIndex,
        nv::convertDlssGParams(*dlssGParams,
          [](BaseTexture *i) { return i ? static_cast<void *>(static_cast<BaseTex *>(i)->tex.texRes) : nullptr; }),
        dx_context);
      return 1;
    }

    case Drv3dCommand::GET_DLSS:
    {
      *static_cast<void **>(par1) = streamlineAdapter ? static_cast<nv::DLSS *>(&streamlineAdapter.value()) : nullptr;
      return 1;
    }

    case Drv3dCommand::GET_REFLEX:
    {
      *static_cast<void **>(par1) = streamlineAdapter ? static_cast<nv::Reflex *>(&streamlineAdapter.value()) : nullptr;
      return 1;
    }

    case Drv3dCommand::GET_MONITORS:
    {
      Tab<String> &monitorList = *reinterpret_cast<Tab<String> *>(par1);
      clear_and_shrink(monitorList);

      IDXGIAdapter1 *dxgiAdapter = get_active_adapter(dx_device);
      if (dxgiAdapter)
      {
        IDXGIOutput *output;
        for (uint32_t outputIndex = 0; SUCCEEDED(dxgiAdapter->EnumOutputs(outputIndex, &output)); outputIndex++)
        {
          monitorList.push_back(get_monitor_name_from_output(output));
          output->Release();
        }
        dxgiAdapter->Release();
        return 1;
      }
      return 0;
    }

    case Drv3dCommand::GET_MONITOR_INFO:
    {
      const char *monitorName = *reinterpret_cast<const char **>(par1);
      String *friendlyName = reinterpret_cast<String *>(par2);
      int *monitorIndex = reinterpret_cast<int *>(par3);
      return get_monitor_info(monitorName, friendlyName, monitorIndex) ? 1 : 0;
    }

    case Drv3dCommand::GET_RESOLUTIONS_FROM_MONITOR:
    {
      const char *monitorName = resolve_monitor_name(*reinterpret_cast<const char **>(par1));
      Tab<String> &resolutions = *reinterpret_cast<Tab<String> *>(par2);

      clear_and_shrink(resolutions);

      ComPtr<IDXGIOutput> dxgiOutput = get_output_monitor_by_name_or_default(dx11_DXGIFactory, monitorName);
      if (!dxgiOutput && FAILED(swap_chain->GetContainingOutput(&dxgiOutput)))
        return 0;

      if (dxgiOutput)
      {
        resolutions = get_resolutions_from_output(dxgiOutput.Get());
        return 1;
      }
      return 0;
    }

    case Drv3dCommand::GET_VIDEO_MEMORY_BUDGET:
    {
      ComPtr<IDXGIAdapter3> adapter3;
      if (SUCCEEDED(get_active_adapter(dx_device)->QueryInterface(COM_ARGS(&adapter3))))
      {
        DXGI_ADAPTER_DESC2 adapterDesc;
        DXGI_QUERY_VIDEO_MEMORY_INFO info;
        if ((SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &info)) && info.Budget > 0) ||
            (SUCCEEDED(adapter3->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &info)) && info.Budget > 0))
        {
          adapter3->GetDesc2(&adapterDesc);
          UINT64 currentUsage = info.CurrentUsage;
          if (par1)
            *reinterpret_cast<uint32_t *>(par1) = uint32_t(info.Budget >> 10);
          if (par2)
            *reinterpret_cast<uint32_t *>(par2) = info.Budget > currentUsage ? uint32_t((info.Budget - currentUsage) >> 10) : 0;
          if (par3)
            *reinterpret_cast<uint32_t *>(par3) = uint32_t(currentUsage >> 10);
          return uint32_t(adapterDesc.DedicatedVideoMemory >> 10);
        }
      }
      return 0;
    }
    case Drv3dCommand::GET_RESOURCE_STATISTICS:
    {
#if DAGOR_DBGLEVEL > 0
      drv3d_dx11::dump_resources(*static_cast<Tab<ResourceDumpInfo> *>(par1));
      return 1;
#else
      return 0;
#endif
    }
    default: return 0;
  }
  return 0;
}

static bool wait_on_swapchain(HANDLE waitable_object, DWORD timeout /*milliseconds*/)
{
  TIME_PROFILE(wait_on_swapchain__present_dx11);
  DWORD result = WAIT_OBJECT_0;
  if (waitable_object)
    result = WaitForSingleObjectEx(waitable_object, timeout, false);

  if (DAGOR_UNLIKELY(result != WAIT_OBJECT_0))
    logwarn("Swaphchain waitable object is not signaled. [Timeout %ums][Result 0x%08X]", timeout, result);

  return DAGOR_LIKELY(result == WAIT_OBJECT_0);
}


VPROG d3d::create_vertex_shader_dagor(const VPRTYPE * /*tokens*/, int /*len*/) { return BAD_VPROG; }
VPROG d3d::create_vertex_shader_asm(const char * /*asm_text*/) { return BAD_VPROG; }
FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE * /*tokens*/, int /*len*/) { return BAD_FSHADER; }
FSHADER d3d::create_pixel_shader_asm(const char * /*asm_text*/) { return BAD_FSHADER; }

//---------------------------------------------------------------------------------

bool d3d::setscissor(int x, int y, int w, int h)
{
  G_ASSERTF(w > 0 && h > 0, "%s(%d, %d, %d, %d)", __FUNCTION__, x, y, w, h);
  g_render_state.nextRasterizerState.scissor_x = x;
  g_render_state.nextRasterizerState.scissor_y = y;
  g_render_state.nextRasterizerState.scissor_w = w;
  g_render_state.nextRasterizerState.scissor_h = h;
  g_render_state.modified = g_render_state.scissorModified = true;

  return true;
}


static Tab<DXGI_FRAME_STATISTICS> frameStatisticsHistory(midmem_ptr());
static const float MEASUREMENT_TIME = 0.2f;
static const int MIN_STATISTICS_HISTORY = 6;
static const int MAX_STATISTICS_HISTORY = 30;

namespace drv3d_dx11
{
extern bool _no_vsync;
extern IDXGIOutput *target_output;
extern void clear_pools_garbage();
} // namespace drv3d_dx11

bool d3d::update_screen(bool app_active)
{
  for (FrameEvents *callback : frontEndFrameCallbacks)
    callback->endFrameEvent();
  frontEndFrameCallbacks.clear();

  CHECK_MAIN_THREAD();

  SCOPED_LATENCY_MARKER(lowlatency::get_current_render_frame(), PRESENT_START, PRESENT_END);

  if (flush_on_present)
  {
    ContextAutoLock contextLock;
    dx_context->Flush();
  }

  for (FrameEvents *callback : backEndFrameCallbacks)
    callback->endFrameEvent();
  backEndFrameCallbacks.clear();

  drv3d_dx11::clear_pools_garbage();
  drv3d_dx11::advance_buffers();

  HRESULT presentHr = 0;

  {
    REGISTER_SCOPED_SLOP(lowlatency::get_current_render_frame(), present_dx11);
    if (present_dest_idx >= 0)
    {
      drv3d_dx11::SwapChainAndHwnd &r = swap_chain_pairs[present_dest_idx];
      RectInt rect = {0, 0, r.w, r.h};
      d3d::stretch_rect(g_driver_state.backBufferColorTex, r.buf, &rect, &rect);
      ContextAutoLock contextLock;
      presentHr = r.sw->Present(0, 0);
    }
    else
    {
      ContextAutoLock contextLock;
      bool reset = !wait_on_swapchain(waitableObject.get(), SWAPCHAIN_WAIT_TIMEOUT);
      if (DAGOR_UNLIKELY(reset)) // Swapchain waitable object is not signaled.
      {                          // We reset device to avoid possible present freeze.
        dagor_d3d_force_driver_reset = true;
        return true;
      }
      presentHr = swap_chain->Present(_no_vsync ? 0 : 1, _no_vsync && use_tearing ? DXGI_PRESENT_ALLOW_TEARING : 0);

      if (SUCCEEDED(presentHr) && !app_active && is_swapchain_window_occluded())
        presentHr = DXGI_STATUS_OCCLUDED;
    }
  }

  occluded_window = (presentHr == DXGI_STATUS_OCCLUDED);

  if (device_should_reset(presentHr, "Present"))
    return true;

  /*todo:Windows8 only
  HRESULT hr = swap_chain->Present(_no_vsync ? 0 : 1, wait ? 0 : DXGI_PRESENT_DO_NOT_WAIT);
  if (hr == DXGI_ERROR_WAS_STILL_DRAWING)
    return false;*/

  if (::dgs_on_swap_callback)
    ::dgs_on_swap_callback();

  static bool useFrameStatistics = use_gpu_dt;
  static int failCount = 0;
  gpu_frame_time = -1.f;
  if (useFrameStatistics && ::dgs_app_active && SUCCEEDED(presentHr))
  {
    DXGI_FRAME_STATISTICS frameStatistics;
    ContextAutoLock contextLock;
    HRESULT hr = swap_chain->GetFrameStatistics(&frameStatistics);
    if (SUCCEEDED(hr))
    {
      failCount = -1;

      if (!frameStatisticsHistory.empty())
      {
        if (frameStatistics.PresentCount > frameStatisticsHistory[0].PresentCount)
        {
          gpu_frame_time = (double)(frameStatistics.SyncQPCTime.QuadPart - frameStatisticsHistory[0].SyncQPCTime.QuadPart) /
                           (double)(qpc_freq.QuadPart * (frameStatistics.PresentCount - frameStatisticsHistory[0].PresentCount));
        }

        if (frameStatistics.PresentCount < frameStatisticsHistory.back().PresentCount ||
            frameStatistics.SyncRefreshCount < frameStatisticsHistory.back().SyncRefreshCount || gpu_frame_time > MEASUREMENT_TIME)
        {
          gpu_frame_time = -1.f;
          frameStatisticsHistory.clear();
        }
      }

      int history = clamp<int>(safediv(MEASUREMENT_TIME, gpu_frame_time), MIN_STATISTICS_HISTORY, MAX_STATISTICS_HISTORY);
      int erase = frameStatisticsHistory.size() - history + 1;
      if (erase > 0)
        erase_items(frameStatisticsHistory, 0, erase);

      frameStatisticsHistory.push_back(frameStatistics);
    }
    else
    {
      if (failCount >= 0 && ++failCount > 100)
      {
        debug("gpu_frame_time: GetFrameStatistics failed (presentHr=0x%08x, hr=0x%08x)", presentHr, hr);
        useFrameStatistics = false;
      }
    }
  }

  if (secondary_swap_chain)
  {
    {
      ContextAutoLock contextLock;
      presentHr = secondary_swap_chain->Present(0, 0);
    }
    device_should_reset(presentHr, "secondary_swap_chain->Present");
  }

  g_render_state.resetDipCounter();
  d3d::set_render_target(); // Render target is unbound on each present with some presentation modes on PC as well.

#if DEBUG_MEMORY_METRICS
  if (memory_metrics)
    memory_metrics->update();
#endif

  return true;
}

void d3d::wait_for_async_present(bool) {}

void d3d::gpu_latency_wait() {}

bool d3d::is_window_occluded() { return occluded_window; }

static bool check_uav_format(unsigned format)
{
  DXGI_FORMAT dxgiFormat = dxgi_format_from_flags(format);

  UINT support = 0;
  if (FAILED(dx_device->CheckFormatSupport(dxgiFormat, &support)))
    return false;

  return !!(support & D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW);
}

bool d3d::should_use_compute_for_image_processing(std::initializer_list<unsigned> formats)
{
#if _TARGET_PC_WIN
  if (d3d::get_driver_desc().shaderModel < 5.0_sm)
  {
    debug("d3d::should_use_compute_for_image_processing is false due to lack of cs_5_0 support.");
    return false;
  }

  for (unsigned format : formats)
    if (!check_uav_format(format))
    {
      debug("d3d::should_use_compute_for_image_processing is false due to lack of UAV support for %u.", (format & TEXFMT_MASK));
      return false;
    }

  return true;
#else
  return false;
#endif
}

namespace drv3d_dx11
{
extern DXGI_GAMMA_CONTROL gamma_control;
extern bool gamma_control_valid;
} // namespace drv3d_dx11

bool d3d::setgamma(float power)
{
  if (hdr_enabled)
    return false;

  HRESULT hr;

  ContextAutoLock contextLock;

  IDXGIOutput *dxgiOutput = NULL;
  hr = swap_chain->GetContainingOutput(&dxgiOutput);

  if (SUCCEEDED(hr))
  {
    DXGI_GAMMA_CONTROL_CAPABILITIES gammaControlCapabilities;
    hr = dxgiOutput->GetGammaControlCapabilities(&gammaControlCapabilities);
    if (SUCCEEDED(hr) && gammaControlCapabilities.NumGammaControlPoints > 1)
    {
      if (power <= 0.f)
        power = 1.f;
      power = 1 / power;
      gamma_control.Offset.Red = gamma_control.Offset.Green = gamma_control.Offset.Blue = 0.f;
      gamma_control.Scale.Red = gamma_control.Scale.Green = gamma_control.Scale.Blue = 1.f;
      for (uint32_t dstPointNo = 0; dstPointNo < gammaControlCapabilities.NumGammaControlPoints; dstPointNo++)
      {
        gamma_control.GammaCurve[dstPointNo].Red = gamma_control.GammaCurve[dstPointNo].Green =
          gamma_control.GammaCurve[dstPointNo].Blue = powf(gammaControlCapabilities.ControlPointPositions[dstPointNo], power);
      }
      hr = dxgiOutput->SetGammaControl(&gamma_control);
      gamma_control_valid = (hr == S_OK);
    }
    dxgiOutput->Release();
  }

  return hr == S_OK;
}

float d3d::get_screen_aspect_ratio() { return drv3d_dx11::screen_aspect_ratio; }
void d3d::change_screen_aspect_ratio(float) {}


static ID3D11Texture2D *backBufferStaging = NULL;
static uint32_t mappedSubresource = 0;


bool d3d::pcwin32::set_capture_full_frame_buffer(bool /*ison*/) { return false; }

void *d3d::fast_capture_screen(int &w, int &h, int &stride_bytes, int &format)
{
  G_ASSERT(!backBufferStaging);

  D3D11_TEXTURE2D_DESC stagingDesc;
  ID3D11Texture2D *backBuffer = NULL;
  {
    ContextAutoLock contextLock;
    HRESULT hr = swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&backBuffer);
    G_ASSERT(SUCCEEDED(hr));
    G_ASSERT(backBuffer);
  }

  backBuffer->GetDesc(&stagingDesc);
  bool msaa = stagingDesc.SampleDesc.Count > 1;
  stagingDesc.Usage = D3D11_USAGE_STAGING;
  stagingDesc.BindFlags = 0;
  stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE; // Swap channels in-place.
  stagingDesc.SampleDesc.Count = 1;
  stagingDesc.SampleDesc.Quality = 0;

  {
    HRESULT hr = dx_device->CreateTexture2D(&stagingDesc, NULL, &backBufferStaging);
    DXERR(hr, "CreateTexture2D");
    if (FAILED(hr))
      return NULL;
  }

  if (msaa)
  {
    ID3D11Texture2D *backBufferResolved;
    D3D11_TEXTURE2D_DESC resolvedDesc = stagingDesc;
    resolvedDesc.Usage = D3D11_USAGE_DEFAULT;
    resolvedDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
    resolvedDesc.CPUAccessFlags = 0;
    {
      HRESULT hr = dx_device->CreateTexture2D(&resolvedDesc, NULL, &backBufferResolved);
      DXERR(hr, "CreateTexture2D");
      if (FAILED(hr))
      {
        backBufferStaging->Release();
        backBufferStaging = NULL;
        return NULL;
      }
    }

    ContextAutoLock contextLock;
    disable_conditional_render_unsafe();
    dx_context->ResolveSubresource(backBufferResolved, 0, backBuffer, 0, stagingDesc.Format);
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
      "DX11: fast_capture_screen uses CopyResource inside a generic render pass");
    dx_context->CopyResource(backBufferStaging, backBufferResolved);

    backBufferResolved->Release();
  }
  else
  {
    ContextAutoLock contextLock;
    disable_conditional_render_unsafe();
    VALIDATE_GENERIC_RENDER_PASS_CONDITION(!g_render_state.isGenericRenderPassActive,
      "DX11: fast_capture_screen uses CopyResource inside a generic render pass");
    dx_context->CopyResource(backBufferStaging, backBuffer);
  }

  mappedSubresource = D3D11CalcSubresource(0, 0, 1);
  static D3D11_MAPPED_SUBRESOURCE lockMsr;
  {
    ContextAutoLock contextLock;
    HRESULT hr = dx_context->Map(backBufferStaging, mappedSubresource,
      D3D11_MAP_READ_WRITE, // Swap channels in-place.
      0, &lockMsr);
    DXERR(hr, "ID3D11DeviceContext::Map");
    if (FAILED(hr))
    {
      backBufferStaging->Release();
      backBufferStaging = NULL;
      return NULL;
    }
  }

  w = stagingDesc.Width;
  h = stagingDesc.Height;
  stride_bytes = lockMsr.RowPitch;
  format = CAPFMT_X8R8G8B8; // Make ScreenShotSystem happy with fast capture.

  if (stagingDesc.Format == DXGI_FORMAT_R8G8B8A8_UNORM)
  {
    for (unsigned int pos = 0; pos < stagingDesc.Height * lockMsr.RowPitch; pos += 4)
    {
      uint8_t tmp = ((uint8_t *)lockMsr.pData)[pos];
      ((uint8_t *)lockMsr.pData)[pos] = ((uint8_t *)lockMsr.pData)[pos + 2];
      ((uint8_t *)lockMsr.pData)[pos + 2] = tmp;
      ((uint8_t *)lockMsr.pData)[pos + 3] = 0xFF;
    }
  }
  else if (stagingDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
  {
    for (unsigned int pos = 0; pos < stagingDesc.Height * lockMsr.RowPitch; pos += 4)
      ((uint8_t *)lockMsr.pData)[pos + 3] = 0xFF;
  }
  else if (stagingDesc.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
  {
    uint16_t *src = (uint16_t *)lockMsr.pData;
    unsigned char *dst = (unsigned char *)lockMsr.pData;
    for (unsigned int pos = 0; pos < stagingDesc.Height * lockMsr.RowPitch; pos += 4 * sizeof(uint16_t))
    {
      uint16_t src0 = src[0];
      dst[0] = min<int>(255, 255.f * safe_sqrt(half_to_float(src[2])));
      dst[1] = min<int>(255, 255.f * safe_sqrt(half_to_float(src[1])));
      dst[2] = min<int>(255, 255.f * safe_sqrt(half_to_float(src0)));
      dst[3] = 0xFF;
      src += 4;
      dst += 4;
    }
    stride_bytes /= 2;
  }
  else
  {
    G_ASSERT(0);
    ContextAutoLock contextLock;
    dx_context->Unmap(backBufferStaging, mappedSubresource);
    backBufferStaging->Release();
    backBufferStaging = NULL;
    backBuffer->Release();
    return NULL;
  }

  backBuffer->Release();

  return lockMsr.pData;
}


void d3d::end_fast_capture_screen()
{
  ContextAutoLock contextLock;
  G_ASSERT(backBufferStaging);

  dx_context->Unmap(backBufferStaging, mappedSubresource);
  backBufferStaging->Release();
  backBufferStaging = NULL;
}

TexPixel32 *d3d::capture_screen(int & /*w*/, int & /*h*/, int & /*stride_bytes*/) { return NULL; }
void d3d::release_capture_buffer() {}

void d3d::get_screen_size(int &w, int &h)
{
  w = g_driver_state.width;
  h = g_driver_state.height;
}

static void na_func() { DAG_FATAL("D3DI function not implemented"); }


void d3d::reserve_res_entries(bool /*strict_max*/, int max_tex, int /*max_vs*/, int /*max_ps*/, int /*max_vdecl*/, int max_vb,
  int max_ib, int /*max_stblk*/)
{
  //  reserve_tex(max_tex);
}

void d3d::get_max_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = 1000;
  max_vb = 1000;
  max_ib = 1000;
  max_vs = max_ps = max_vdecl = max_stblk = 0;
}

void d3d::get_cur_used_res_entries(int &max_tex, int &max_vs, int &max_ps, int &max_vdecl, int &max_vb, int &max_ib, int &max_stblk)
{
  max_tex = 1000;
  max_vb = 1000;
  max_ib = 1000;
  max_vs = max_ps = max_vdecl = max_stblk = 0;
}

d3d::EventQuery *d3d::create_event_query() { return (d3d::EventQuery *)(intptr_t)create_query(D3D11_QUERY_EVENT); }

void d3d::release_event_query(EventQuery *q)
{
  if (!q)
    return;
  release_query((int)(intptr_t)q);
}

bool d3d::issue_event_query(EventQuery *query)
{
  int pQuery = (int)(intptr_t)query;
  if (pQuery <= 0 || !all_queries[pQuery].query)
    return false;
  ContextAutoLock contextLock;
  dx_context->End(all_queries[pQuery].query);
  return true;
}

bool d3d::get_event_query_status(d3d::EventQuery *query, bool flush)
{
  int pQuery = (int)(intptr_t)query;
  if (pQuery <= 0 || !all_queries[pQuery].query)
    return true;
  ContextAutoLock contextLock;
  HRESULT hr = dx_context->GetData(all_queries[pQuery].query, NULL, 0, flush ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
  return hr != S_FALSE;
}

namespace drv3d_dx11
{
typedef d3d::PredicateGeneric<ID3D11Predicate *, nullptr> Predicate;
static d3d::PredicateStorage<Predicate> predicates;
static int saved_predicates_count = 0;
extern bool conditional_render_enabled;

void save_predicates() { saved_predicates_count = predicates.list.size(); }

void recreate_predicates()
{
  for (int i = 0; i < saved_predicates_count; ++i)
    d3d::create_predicate();

  saved_predicates_count = 0;
}

void close_predicates() { close_predicates_generic(predicates); }

void close_frame_callbacks()
{
  // signaling end of frame before deleting reference to callback
  // to make sure it doesn't remain open waiting for frame end
  for (FrameEvents *callback : frontEndFrameCallbacks)
    callback->endFrameEvent();
  frontEndFrameCallbacks.clear();

  for (FrameEvents *callback : backEndFrameCallbacks)
    callback->endFrameEvent();
  backEndFrameCallbacks.clear();
}

struct CreatePredicateCB
{
  ID3D11Predicate *operator()()
  {
    ID3D11Predicate *p = nullptr;
    D3D11_QUERY_DESC qdesc;
    qdesc.MiscFlags = D3D11_QUERY_MISC_PREDICATEHINT;
    qdesc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;

    if (FAILED(dx_device->CreatePredicate(&qdesc, &p)))
      p = nullptr;

    return p;
  }
};

struct DeletePredicateCB
{
  void operator()(ID3D11Predicate *p) { p->Release(); }
};
} // namespace drv3d_dx11

int d3d::create_predicate()
{
  if (!g_device_desc.caps.hasOcclusionQuery)
    return -1;

  return create_predicate_generic<CreatePredicateCB>(predicates);
}

void d3d::free_predicate(int id) { free_predicate_generic<DeletePredicateCB>(predicates, id); }

bool d3d::begin_survey(int id)
{
  if (-1 == id)
    return false;
  ID3D11Predicate *p = begin_survey_generic(predicates, id);
  if (p)
  {
    ContextAutoLock contextLock;
    if (flush_before_survey)
      dx_context->Flush();
    dx_context->Begin(p);
  }
  return p != nullptr;
}

void d3d::end_survey(int id)
{
  if (-1 == id)
    return;
  ID3D11Predicate *p = end_survey_generic(predicates, id);
  if (p)
  {
    ContextAutoLock contextLock;
    dx_context->End(p);
  }
}

void d3d::begin_conditional_render(int id)
{
  ID3D11Predicate *p = begin_conditional_render_generic(predicates, id);
  if (p)
  {
    ContextAutoLock lock;
    dx_context->SetPredication(p, FALSE);
    conditional_render_enabled = true;
  }
}

void d3d::end_conditional_render(int id)
{
  ID3D11Predicate *p = end_conditional_render_generic(predicates, id);
  if (p)
  {
    ContextAutoLock lock;
    dx_context->SetPredication(NULL, FALSE);
    conditional_render_enabled = false;
  }
}

bool d3d::get_vrr_supported() { return use_tearing; }
bool d3d::get_vsync_enabled() { return !_no_vsync; }
bool d3d::enable_vsync(bool enable)
{
  _no_vsync = !enable;
  return true;
}

void d3d::pcwin32::set_present_wnd(void *hwnd)
{
  if (!dx11_DXGIFactory || !hwnd)
  {
    present_dest_idx = -1;
    return;
  }

  RECT rect;
  GetClientRect((HWND)hwnd, &rect);

  for (int i = 0; i < swap_chain_pairs.size(); i++)
    if (swap_chain_pairs[i].hwnd == hwnd)
    {
      drv3d_dx11::SwapChainAndHwnd &r = swap_chain_pairs[i];
      if (rect.right - rect.left != r.w || rect.bottom - rect.top != r.h)
      {
        // recreate swapchain
        del_d3dres(r.buf);
        r.sw->Release();
        erase_items(swap_chain_pairs, i, 1);
        break;
      }

      present_dest_idx = i;
      return;
    }

  drv3d_dx11::SwapChainAndHwnd &r = swap_chain_pairs.push_back();

  DXGI_SWAP_CHAIN_DESC scd;
  memset(&scd, 0, sizeof(scd));
  scd.BufferDesc.Width = rect.right - rect.left;
  scd.BufferDesc.Height = rect.bottom - rect.top;
  scd.BufferDesc.RefreshRate.Numerator = 0;
  scd.BufferDesc.RefreshRate.Denominator = 1;
  scd.BufferDesc.Format = PC_BACKBUFFER_DXGI_FORMAT;
  scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  scd.SampleDesc.Count = 1;
  scd.SampleDesc.Quality = 0;
  scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
  scd.BufferCount = 1;
  scd.OutputWindow = (HWND)hwnd;
  scd.Windowed = true;
  scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  scd.Flags = 0;
  dx11_DXGIFactory->CreateSwapChain(dx_device, &scd, &r.sw);

  r.hwnd = (HWND)hwnd;
  r.buf = create_backbuffer_tex(0, r.sw);
  r.w = scd.BufferDesc.Width;
  r.h = scd.BufferDesc.Height;
}

void d3d::get_video_modes_list(Tab<String> &list)
{
  G_ASSERT(swap_chain);

  clear_and_shrink(list);

  IDXGIOutput *dxgiOutput;
  HRESULT hr = swap_chain->GetContainingOutput(&dxgiOutput);
  if (SUCCEEDED(hr))
  {
    list = get_resolutions_from_output(dxgiOutput);
    dxgiOutput->Release();
  }
}

bool d3d::set_srgb_backbuffer_write(bool on)
{
  if (g_render_state.srgb_bb_write == on)
    return on;
  g_render_state.srgb_bb_write = on;
  g_render_state.modified = g_render_state.rtModified = true;
  return !on;
}


#if DAGOR_DBGLEVEL > 0

#include <osApiWrappers/dag_unicode.h>

#ifndef __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__
#define __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__

extern "C" const IID IID_ID3DUserDefinedAnnotation;
MIDL_INTERFACE("b2daad8b-03d4-4dbf-95eb-32ab4b63d0ab")
ID3DUserDefinedAnnotation : public IUnknown
{
public:
  virtual INT STDMETHODCALLTYPE BeginEvent(
    /* [annotation] */
    _In_ LPCWSTR Name) = 0;

  virtual INT STDMETHODCALLTYPE EndEvent(void) = 0;

  virtual void STDMETHODCALLTYPE SetMarker(
    /* [annotation] */
    _In_ LPCWSTR Name) = 0;

  virtual BOOL STDMETHODCALLTYPE GetStatus(void) = 0;
};
#endif /* __ID3DUserDefinedAnnotation_INTERFACE_DEFINED__ */


// Module and function pointers
#include <d3d9.h>
typedef INT(WINAPI *LPD3DPERF_BEGINEVENT)(D3DCOLOR, LPCWSTR);
typedef INT(WINAPI *LPD3DPERF_ENDEVENT)(void);
typedef VOID(WINAPI *LPD3DPERF_SETMARKER)(D3DCOLOR, LPCWSTR);
typedef VOID(WINAPI *LPD3DPERF_SETREGION)(D3DCOLOR, LPCWSTR);
typedef BOOL(WINAPI *LPD3DPERF_QUERYREPEATFRAME)(void);
typedef VOID(WINAPI *LPD3DPERF_SETOPTIONS)(DWORD dwOptions);
typedef DWORD(WINAPI *LPD3DPERF_GETSTATUS)(void);
static HMODULE s_hModD3D9 = NULL;
static LPD3DPERF_BEGINEVENT s_DynamicD3DPERF_BeginEvent = NULL;
static LPD3DPERF_ENDEVENT s_DynamicD3DPERF_EndEvent = NULL;
static LPD3DPERF_SETMARKER s_DynamicD3DPERF_SetMarker = NULL;
static LPD3DPERF_SETREGION s_DynamicD3DPERF_SetRegion = NULL;
static LPD3DPERF_QUERYREPEATFRAME s_DynamicD3DPERF_QueryRepeatFrame = NULL;
static LPD3DPERF_SETOPTIONS s_DynamicD3DPERF_SetOptions = NULL;
static LPD3DPERF_GETSTATUS s_DynamicD3DPERF_GetStatus = NULL;

// Ensure function pointers are initialized
static bool ensureD3D9APIs()
{
  // If the module is non-NULL, this function has already been called.  Note
  // that this doesn't guarantee that all ProcAddresses were found.
  if (s_hModD3D9 != NULL)
    return true;

  // This may fail if Direct3D 9 isn't installed
  s_hModD3D9 = LoadLibraryA("d3d9.dll");
  if (s_hModD3D9 != NULL)
  {
    s_DynamicD3DPERF_BeginEvent = (LPD3DPERF_BEGINEVENT)GetProcAddress(s_hModD3D9, "D3DPERF_BeginEvent");
    s_DynamicD3DPERF_EndEvent = (LPD3DPERF_ENDEVENT)GetProcAddress(s_hModD3D9, "D3DPERF_EndEvent");
    s_DynamicD3DPERF_SetMarker = (LPD3DPERF_SETMARKER)GetProcAddress(s_hModD3D9, "D3DPERF_SetMarker");
    s_DynamicD3DPERF_SetRegion = (LPD3DPERF_SETREGION)GetProcAddress(s_hModD3D9, "D3DPERF_SetRegion");
    s_DynamicD3DPERF_QueryRepeatFrame = (LPD3DPERF_QUERYREPEATFRAME)GetProcAddress(s_hModD3D9, "D3DPERF_QueryRepeatFrame");
    s_DynamicD3DPERF_SetOptions = (LPD3DPERF_SETOPTIONS)GetProcAddress(s_hModD3D9, "D3DPERF_SetOptions");
    s_DynamicD3DPERF_GetStatus = (LPD3DPERF_GETSTATUS)GetProcAddress(s_hModD3D9, "D3DPERF_GetStatus");
  }
  debug("d3d9_getStatus %p : %d", s_DynamicD3DPERF_GetStatus, s_DynamicD3DPERF_GetStatus ? s_DynamicD3DPERF_GetStatus() : -1);

  return s_hModD3D9 != NULL;
}

static ID3DUserDefinedAnnotation *annotation = NULL;

static bool perf_analysis_inited = false;

static void init_perf_analysis()
{
  if (perf_analysis_inited)
    return;
  ensureD3D9APIs();
  ContextAutoLock lock;
  HRESULT hr = dx_context->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void **)&annotation);
  debug("annotation = %p status = %d", annotation, annotation ? annotation->GetStatus() : -1);
  perf_analysis_inited = true;
}

void close_perf_analysis()
{
  perf_analysis_inited = false;
  if (annotation)
    annotation->Release();
  annotation = NULL;
  if (s_hModD3D9)
    FreeLibrary(s_hModD3D9);
  s_hModD3D9 = NULL;
}

void d3d::beginEvent(const char *s)
{
  push_aftermath_context(s);
  init_perf_analysis();
  if (!annotation && !s_DynamicD3DPERF_BeginEvent)
    return;
  wchar_t wbuf[128];
  wchar_t *wstr = utf8_to_wcs(s, wbuf, 128);
  {
    ContextAutoLock lock;
    if (annotation)
      annotation->BeginEvent(wstr);
    else if (s_DynamicD3DPERF_BeginEvent)
      s_DynamicD3DPERF_BeginEvent(0xFFFFFFFF, wstr);
  }

  d3dhang::hang_if_requested(s);
}

void d3d::endEvent()
{
  pop_aftermath_context();
  if (!annotation && !s_DynamicD3DPERF_EndEvent)
    return;
  ContextAutoLock lock;
  if (annotation)
    annotation->EndEvent();
  else if (s_DynamicD3DPERF_EndEvent)
    s_DynamicD3DPERF_EndEvent();
}

#else
void close_perf_analysis() {}
void d3d::beginEvent(const char *) {}
void d3d::endEvent() {}
#endif

ResourceAllocationProperties d3d::get_resource_allocation_properties(const ResourceDescription &desc)
{
  G_UNUSED(desc);
  return {};
}
ResourceHeap *d3d::create_resource_heap(ResourceHeapGroup *heap_group, size_t size, ResourceHeapCreateFlags flags)
{
  G_UNUSED(heap_group);
  G_UNUSED(size);
  G_UNUSED(flags);
  return nullptr;
}
void d3d::destroy_resource_heap(ResourceHeap *heap) { G_UNUSED(heap); }
Sbuffer *d3d::place_buffer_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  G_UNUSED(heap);
  G_UNUSED(desc);
  G_UNUSED(offset);
  G_UNUSED(alloc_info);
  G_UNUSED(name);
  return nullptr;
}
BaseTexture *d3d::place_texture_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
  const ResourceAllocationProperties &alloc_info, const char *name)
{
  G_UNUSED(heap);
  G_UNUSED(desc);
  G_UNUSED(offset);
  G_UNUSED(alloc_info);
  G_UNUSED(name);
  return nullptr;
}
ResourceHeapGroupProperties d3d::get_resource_heap_group_properties(ResourceHeapGroup *heap_group)
{
  G_UNUSED(heap_group);
  return {};
}
void d3d::map_tile_to_resource(BaseTexture *tex, ResourceHeap *heap, const TileMapping *mapping, size_t mapping_count)
{
  G_UNUSED(tex);
  G_UNUSED(mapping);
  G_UNUSED(mapping_count);
  G_UNUSED(heap);
}
TextureTilingInfo d3d::get_texture_tiling_info(BaseTexture *tex, size_t subresource)
{
  G_UNUSED(tex);
  G_UNUSED(subresource);
  return TextureTilingInfo{};
}

IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC()
IMPLEMENT_D3D_RUB_API_USING_GENERIC()
