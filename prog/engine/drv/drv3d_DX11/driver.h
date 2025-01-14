// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#define INSIDE_DRIVER 1
#define LLLOG \
  if (0)      \
  debug

#include <d3d11.h>
#define IDXGI_SWAP_CHAIN           IDXGISwapChain
#define IDXGI_FACTORY              IDXGIFactory1
#define ID3D11_DEV                 ID3D11Device
#define ID3D11_DEV1                ID3D11Device1
#define ID3D11_DEV3                ID3D11Device3
#define ID3D11_DEVCTX              ID3D11DeviceContext
#define ID3D11_DEVCTX1             ID3D11DeviceContext1
#define D3D11_RASTERIZER_DESC_NEXT D3D11_RASTERIZER_DESC2
struct RENDERDOC_API_1_5_0;
// #include <d3dx11.h>
// #include <d3dx10.h>

// using dx9 interface
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_consts_base.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform_pc.h>
#include <math/dag_mathBase.h>
#include <generic/dag_carray.h>
#include <generic/dag_tab.h>
#include <validation.h>

#include "frameStateTM.inc.h"
#include <perfMon/dag_graphStat.h>
#include <vecmath/dag_vecMath.h>
#include <math/dag_vecMathCompatibility.h>

#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <AmdDxExtDepthBoundsApi.h>
#include <supp/dag_comPtr.h>
#include <drv/3d/dag_resetDevice.h>
#include "streamline_adapter.h"
#include "winapi_helpers.h"
#include <EASTL/optional.h>
#include "memory_metrics.h"

/*
#include "drvdesc.h"
#include <util/dag_globDef.h>
#include <util/dag_string.h>
#include <stdarg.h>
*/
#include <osApiWrappers/dag_miscApi.h>

#include "drv_log_defs.h"

struct ID3D11Device1;
struct ID3D11Device3;
struct ID3D11DeviceContext1;

#if DAGOR_DBGLEVEL > 0
#define CHECK_MAIN_THREAD()                                                                                                         \
  G_ASSERTF((gpuAcquireRefCount && gpuThreadId == GetCurrentThreadId()), "curThread=%X != gpuThread=%d, is_main=%d acquire_ref=%d", \
    GetCurrentThreadId(), gpuThreadId, is_main_thread(), gpuAcquireRefCount)
#else
#define CHECK_MAIN_THREAD() ((void)0)
#endif
#define CHECK_THREAD CHECK_MAIN_THREAD()

#define MAX_PS_SAMPLERS 16
#define MAX_VS_SAMPLERS 16
#define MAX_CS_SAMPLERS 16

#define MAX_RESOURCES 32
// D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT

#define MAX_UAV           8
#define MAX_CONST_BUFFERS 9

#define MAX_VERTEX_STREAMS 4
#define PRIM_UNDEF         0

#define SAFE_RELEASE(obj) \
  if (obj != NULL)        \
  {                       \
    obj->Release();       \
    obj = NULL;           \
  }
#define COPY_KEY(name)          u.s.name = name
#define COPY_KEY_PTR(ptr, name) u.s.name = ptr->name

#define NVAPI_SLI_SYNC 0

#define PC_BACKBUFFER_DXGI_FORMAT DXGI_FORMAT_B8G8R8A8_UNORM

#define IMMEDIATE_CB_NAMESPACE namespace drv3d_dx11

#define DEBUG_MEMORY_METRICS DAGOR_DBGLEVEL > 0

namespace drv3d_dx11
{
const char *dx11_error(HRESULT hr);

extern void close_predicates();
extern void close_frame_callbacks();
extern bool device_should_reset(HRESULT hr, const char *text);
extern int get_present_dest_idx();
extern void dump_memory_if_needed(HRESULT hr);
} // namespace drv3d_dx11

#define __DXFATAL(x, ...) DAG_FATAL(x, ##__VA_ARGS__)
#define __DXERR(x, ...)   LOGERR_CTX(x, ##__VA_ARGS__)
//-V:DXFATAL:1048
#define DXFATAL(expr, text, ...)                                                                        \
  do                                                                                                    \
  {                                                                                                     \
    HRESULT __hres = (expr);                                                                            \
    drv3d_dx11::last_hres = __hres;                                                                     \
    if (FAILED(__hres))                                                                                 \
    {                                                                                                   \
      dump_memory_if_needed(__hres);                                                                    \
      __DXFATAL("dx11 error: %s hr=0x%X " text, drv3d_dx11::dx11_error(__hres), __hres, ##__VA_ARGS__); \
    }                                                                                                   \
  } while (0)
#define DXERR(expr, text, ...)                                                                        \
  do                                                                                                  \
  {                                                                                                   \
    HRESULT __hres = (expr);                                                                          \
    drv3d_dx11::last_hres = __hres;                                                                   \
    if (FAILED(__hres))                                                                               \
    {                                                                                                 \
      dump_memory_if_needed(__hres);                                                                  \
      __DXERR("dx11 error: %s hr=0x%X " text, drv3d_dx11::dx11_error(__hres), __hres, ##__VA_ARGS__); \
    }                                                                                                 \
  } while (0)

#define RETRY_CREATE_STAGING_TEX(HR, SZ, EXPR)                                       \
  if (DAGOR_UNLIKELY(FAILED(HR)) && (HR) == E_OUTOFMEMORY)                           \
  {                                                                                  \
    if (dgs_on_out_of_memory && dgs_on_out_of_memory(tql::sizeInKb(SZ + (2 << 20)))) \
      HR = EXPR;                                                                     \
  }

/*
   DriverState g_driver_state - device handling / framebuffers
   RenderState g_render_state - command buffer / states (per thread local in future)

   shared resources are MT protected global arrays
   xxx_shaders
   xxx_textures
   xxx_buffers

   where xxx = init | close | reset_frame
*/
interface DECLSPEC_UUID("9f251514-9d4d-4902-9d60-18988ab7d4b5") DECLSPEC_NOVTABLE IDXGraphicsAnalysis : public IUnknown
{
  STDMETHOD_(void, BeginCapture)() PURE;
  STDMETHOD_(void, EndCapture)() PURE;
};

namespace drv3d_dx11
{
struct SwapChainAndHwnd
{
  IDXGISwapChain *sw;
  void *hwnd;
  Texture *buf;
  int w, h;
};
extern Tab<SwapChainAndHwnd> swap_chain_pairs;
extern IDXGI_SWAP_CHAIN *swap_chain, *secondary_swap_chain;
extern IDXGI_FACTORY *dx11_DXGIFactory;
extern ID3D11_DEV *dx_device;
extern ID3D11_DEV1 *dx_device1;
extern ID3D11_DEV3 *dx_device3;
extern ID3D11_DEVCTX *dx_context;
extern ID3D11_DEVCTX1 *dx_context1;
extern os_spinlock_t dx_context_cs;
extern WinCritSec dx_res_cs;
extern SmartReadWriteFifoLock reset_rw_lock; // Read-lock for loading resources, write-lock for reset.
extern Texture *secondary_backbuffer_color_tex;
extern intptr_t gpuThreadId;
extern int gpuAcquireRefCount;
extern D3D_FEATURE_LEVEL featureLevelsSupported;
extern __declspec(thread) HRESULT last_hres;
extern bool is_nvapi_initialized;
extern int default_display_index;
extern IAmdDxExt *amdExtension;
extern IAmdDxExtDepthBounds *amdDepthBoundsExtension;
extern RENDERDOC_API_1_5_0 *docAPI;
extern bool nv_dbt_supported;
extern bool ati_dbt_supported;
extern IPoint2 resolution;
extern uint8_t override_max_anisotropy_level;
extern float gpu_frame_time;
extern LARGE_INTEGER qpc_freq;
extern bool flush_on_present;
extern bool flush_before_survey;
extern bool use_gpu_dt;
extern bool hdr_enabled;
extern bool int10_hdr_buffer;
extern bool command_list_wa;
extern eastl::optional<MemoryMetrics> memory_metrics;

extern bool window_occlusion_check_enabled;
extern bool immutable_textures;
extern bool resetting_device_now;
extern HRESULT device_is_lost;
extern unsigned texture_sysmemcopy_usage, vbuffer_sysmemcopy_usage, ibuffer_sysmemcopy_usage;
extern bool use_tearing;
extern double vsync_refresh_rate;
extern eastl::optional<StreamlineAdapter> streamlineAdapter;
extern HandlePointer waitableObject;

extern float screen_aspect_ratio;

inline float normalizeColor(int value)
{
  constexpr float oneOver255 = 1. / 255;
  return clamp<float>(oneOver255 * value, 0., 1.0);
}

/*  struct DriverDesc : public Driver3dDesc
  {
    int myflg1;
    int maxAniso;
    float maxPointSpriteSize;
  };*/

typedef Driver3dDesc DriverDesc;
extern DriverDesc g_device_desc;

struct ContextAutoLock
{
  ContextAutoLock() { os_spinlock_lock(&dx_context_cs); }

  ~ContextAutoLock() { os_spinlock_unlock(&dx_context_cs); }
};

struct ResAutoLock // Guard the streamable texture data access.
{
  ResAutoLock() { dx_res_cs.lock(); }

  ~ResAutoLock() { dx_res_cs.unlock(); }
};

void acquireD3dOwnership();
void releaseD3dOwnership();

static inline void acquireGpu() { acquireD3dOwnership(); }
static inline void releaseGpu() { releaseD3dOwnership(); }

struct AutoAcquireGpu
{
  AutoAcquireGpu() { acquireGpu(); }
  ~AutoAcquireGpu() { releaseGpu(); }
};
void remove_texture_from_samplers(BaseTexture *tex);
void remove_buffer_from_slot(Sbuffer *buf);
void remove_view_from_uav(ID3D11UnorderedAccessView *view);
void remove_view_from_uav_ignore_slot(int shader_stage, int ignore_slot, ID3D11UnorderedAccessView *view);
void disable_conditional_render_unsafe();
}; // namespace drv3d_dx11

using namespace drv3d_dx11;

#include "pools.h"
using namespace drv3d_generic;
#include "texture.h"
#include "genericBuffer.h"
#include "buffers.h"
#include "states.h"
#include "shaders.h"


namespace drv3d_dx11
{
enum
{
  HAS_HS = 1 << 0,
  HAS_DS = 1 << 1,
  HAS_GS = 1 << 2
};

// global driver state
struct DriverState
{
  Texture *backBufferColorTex;
  uint16_t width;
  uint16_t height;

  int msaaMode;
  int msaaSamples;
  int msaaQuality;
  bool msaaEnabled;

  bool createSurfaces(uint32_t screenWidth, uint32_t screenHeight);
};

// per thread state
struct RenderState : public FrameStateTM
{
  bool modified;
  bool rtModified;
  uint8_t viewModified; // VIEWMOD_
  bool scissorModified;
  bool vsModified;
  bool psModified;
  bool csModified;
  bool vertexInputModified;
  bool rasterizerModified;
  bool alphaBlendModified;
  bool depthStencilModified;
#if ENABLE_GENERIC_RENDER_PASS_VALIDATION
  bool isGenericRenderPassActive = false;
#endif

  bool srgb_bb_write;
  uint8_t hdgBits;      // flag bits HS(0x1), DS(0x2), GS(0x4) for current compound VS prog (and dirty(0x80) for faster cmp)
  uint8_t hullTopology; // 5 bits is enough but it will start from 33

  //- input assembler ------------------------------------

  VertexInput nextVertexInput;
  VertexInput currVertexInput;
  uint8_t currPrimType; // PRIM_UNDEF == 0 - unitialized

  //- shaders --------------------------------------------
  VDECL nextVdecl;
  int nextVertexShader;
  int nextPixelShader;
  int nextComputeShader;

  ConstantBuffers constants;
  TextureFetchState texFetchState;

  //- rasterizer -----------------------------------------

  RasterizerState nextRasterizerState;

  //- output merger --------------------------------------
  Driver3dRenderTarget nextRtState;
  Driver3dRenderTarget currRtState;

  uint8_t stencilRef;
  float blendFactor[BLEND_FACTORS_COUNT];

  int8_t maxUsedTarget;
  uint16_t currentFrameDip;

  RenderState() :
    modified(true),
    rtModified(true),
    viewModified(VIEWMOD_FULL),
    scissorModified(false),
    vsModified(true),
    psModified(true),
    csModified(true),
    vertexInputModified(true),
    rasterizerModified(true),
    alphaBlendModified(true),
    depthStencilModified(true),
    srgb_bb_write(0),
    hdgBits(0),
    hullTopology(0),
    nextVertexInput{},
    currVertexInput{},
    currPrimType(0),
    nextVdecl(BAD_HANDLE),
    nextVertexShader(BAD_HANDLE),
    nextPixelShader(BAD_HANDLE),
    nextComputeShader(BAD_HANDLE),
    nextRasterizerState{},
    nextRtState{},
    currRtState{},
    stencilRef(0),
    maxUsedTarget(-1),
    currentFrameDip(0)
  {
    blendFactor[0] = 1.0f;
    blendFactor[1] = 1.0f;
    blendFactor[2] = 1.0f;
    blendFactor[3] = 1.0f;
  }
  void reset()
  {
    // this is a bit hacky. We'd better correctly reset everything
    ConstantBuffers savedConstants = constants;
    int dip = currentFrameDip;
    *this = RenderState();
    currentFrameDip = dip; //-V1048
    constants = savedConstants;
    nextRtState.setBackbufColor();
  }

  void resetFrame()
  {
    modified = true;
    srgb_bb_write = false;
    rtModified = true;
    viewModified = VIEWMOD_FULL;
    scissorModified = true;
    psModified = true;
    vsModified = true;
    hdgBits = 0x80;
    vertexInputModified = true;
    rasterizerModified = true;
    alphaBlendModified = true;
    depthStencilModified = true;
    for (int i = 0; i < texFetchState.resources.size(); ++i)
      texFetchState.resources[i].modifiedMask = 0xffffffff;
    currentFrameDip = 0;
    resetDipCounter();
  };

  void resetCached() {}

  void resetDipCounter() { currentFrameDip = 0; }
  ~RenderState()
  {
    for (int i = 0; i < MAX_PS_SAMPLERS; ++i)
      d3d::set_tex(STAGE_PS, i, 0);
    for (int i = 0; i < MAX_VS_SAMPLERS; ++i)
      d3d::set_tex(STAGE_VS, i, 0);
    for (int i = MAX_PS_SAMPLERS; i < MAX_RESOURCES; ++i)
      d3d::set_buffer(STAGE_PS, i, 0);
    for (int i = MAX_VS_SAMPLERS; i < MAX_RESOURCES; ++i)
      d3d::set_buffer(STAGE_VS, i, 0);
  }
};

extern RenderState g_render_state;
extern DriverState g_driver_state;

bool init_rendertargets(RenderState &rs);
bool init_buffers(RenderState &rs, int imm_vb_size, int imm_ib_size);
bool init_shaders(RenderState &rs);
bool init_states(RenderState &rs);
bool init_textures();

void set_vertex_shader_debug_info(VPROG vpr, const char *debug_info);
void set_pixel_shader_debug_info(FSHADER fsh, const char *debug_info);

void flush_null_rendertargets(RenderState &rs);
void flush_null_cs_rendertargets(RenderState &rs);
void flush_rendertargets(RenderState &rs);
void flush_buffers(RenderState &rs);
void flush_shaders(RenderState &rs, bool should_flush_consts);
void flush_cs_shaders(RenderState &rs, bool should_flush_consts, bool async);
void flush_states(RenderState &rs);
void flush_samplers(RenderState &rs);
void flush_cs_objects(RenderState &rs, bool async);

void close_rendertargets();
void close_buffers();
void close_shaders();
void close_states();
void close_samplers();
void recreate_render_states();
void close_textures();
void recreate_all_queries();
void reset_all_queries();

static bool init_all(int imm_vb_size, int imm_ib_size)
{
  RenderState &rs = g_render_state;
  rs.init();
  bool res = true;

  res &= init_rendertargets(rs);
  res &= init_buffers(rs, imm_vb_size, imm_ib_size);
  res &= init_shaders(rs);
  res &= init_states(rs);
  init_textures();

  rs.resetFrame();
  return res;
}

static bool flush_all(bool should_flush_buffers = true)
{
  if (dagor_d3d_force_driver_reset)
    return false;
  RenderState &rs = g_render_state;
  if (!rs.modified)
    return true;

  // It must be a first flush before all other resources because setups a current shader set (which of
  // vs, ds, hs, gs are actually used and so on) which the rest resources will be passed to
  flush_shaders(rs, should_flush_buffers);
  if (dagor_d3d_force_driver_reset)
    return false;
  flush_null_rendertargets(rs); // Should be called before flush_samplers to reset RT targets to be bound to shaders.
  if (dagor_d3d_force_driver_reset)
    return false;
  flush_cs_objects(rs, /*async*/ false);
  if (dagor_d3d_force_driver_reset)
    return false;
  flush_samplers(rs); // Should be called before flush_rendertargets to reset RT bound textures.
  if (dagor_d3d_force_driver_reset)
    return false;
  flush_rendertargets(rs);
  if (dagor_d3d_force_driver_reset)
    return false;
  flush_buffers(rs);
  if (dagor_d3d_force_driver_reset)
    return false;
  flush_states(rs); // depends on rendertargets

  if (dagor_d3d_force_driver_reset)
    return false;

  rs.modified = false;
  rs.currentFrameDip++;
  return true;
}

static bool flush_cs_all(bool should_flush_buffers, bool async)
{
  if (dagor_d3d_force_driver_reset)
    return false;
  RenderState &rs = g_render_state;
  flush_null_cs_rendertargets(rs); // Should be called before flush_samplers to reset RT targets to be bound to shaders. For some
                                   // reason still a case for compute
  flush_cs_objects(rs, async);     // always before flushing ps/vs's srv
  if (dagor_d3d_force_driver_reset)
    return false;

  flush_cs_shaders(rs, should_flush_buffers, async);

  if (dagor_d3d_force_driver_reset)
    return false;

  rs.currentFrameDip++;
  return true;
}

static void reset_frame_all()
{
  RenderState &rs = g_render_state;

  flush_states(rs); // depends on rendertargets
}

DXGI_FORMAT dxgi_format_from_flags(uint32_t cflg);
int dxgi_format_to_bpp(DXGI_FORMAT fmt);
const char *dx11_error(HRESULT hr);
void override_unsupported_bc(uint32_t &cflg);
HRESULT set_fullscreen_state(bool fullscreen);
} // namespace drv3d_dx11
