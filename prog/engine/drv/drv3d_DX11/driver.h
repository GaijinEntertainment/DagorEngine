// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "driver_defs.h"
#include <drv/3d/dag_tex3d.h>
#include <drv/3d/dag_driverDesc.h>
#include <generic/dag_tab.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_rwLock.h>
#include <osApiWrappers/dag_spinlock.h>
#include <generic/dag_initOnDemand.h>
#include "gpuVendorAmd.h"
#include "streamline_adapter.h"
#include "memory_metrics.h"
#include "winapi_helpers.h"

struct ID3D11Device1;
struct ID3D11Device3;
struct ID3D11DeviceContext1;

namespace drv3d_dx11
{
const char *dx11_error(HRESULT hr);

extern void close_predicates();
extern void close_frame_callbacks();
extern bool device_should_reset(HRESULT hr, const char *text);
extern int get_present_dest_idx();
extern void dump_memory_if_needed(HRESULT hr);
} // namespace drv3d_dx11

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
extern BaseTexture *secondary_backbuffer_color_tex;
extern intptr_t gpuThreadId;
extern int gpuAcquireRefCount;
extern D3D_FEATURE_LEVEL featureLevelsSupported;
extern __declspec(thread) HRESULT last_hres;
extern bool is_nvapi_initialized;
extern int default_display_index;
#if HAS_AMD_DX_EXT
extern IAmdDxExt *amdExtension;
extern IAmdDxExtDepthBounds *amdDepthBoundsExtension;
#endif
extern RENDERDOC_API_1_5_0 *docAPI;
extern bool nv_dbt_supported;
extern bool ati_dbt_supported;
extern IPoint2 resolution;
extern uint8_t override_max_anisotropy_level;
extern float gpu_frame_time;
extern LARGE_INTEGER qpc_freq;
extern bool flush_on_present;
extern bool flush_before_survey;
extern bool flush_before_sep_ablend_and_blend_factor_for_RT1;
extern int max_pending_frames;
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

namespace drv3d_dx11
{
enum
{
  HAS_HS = 1 << 0,
  HAS_DS = 1 << 1,
  HAS_GS = 1 << 2
};

struct DriverState;
struct RenderState;

extern InitOnDemand<RenderState, false> g_render_state_val;
extern InitOnDemand<DriverState, false> g_driver_state_val;
#define g_render_state (*g_render_state_val)
#define g_driver_state (*g_driver_state_val)

bool init_rendertargets(RenderState &rs);
bool init_buffers(RenderState &rs, int imm_vb_size);
bool init_shaders(RenderState &rs);
bool init_states(RenderState &rs);
bool init_textures();

void set_vertex_shader_debug_info(VPROG vpr, const char *debug_info);
void set_pixel_shader_debug_info(FSHADER fsh, const char *debug_info);
void set_compute_shader_debug_info(PROGRAM fsh, const char *debug_info);

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
void recreate_shaders();
void close_textures();
void recreate_all_queries();
void reset_all_queries();
void reset_timestamp_frequency();

bool init_all(int imm_vb_size);

enum class FlushStages
{
  Shaders,
  NullRTs,
  CsObjects,
  Samplers,
  RTs,
  Buffers,
  States,
};

bool flush_states(FlushStages last_stage, bool should_flush_buffers = true);

inline bool flush_all(bool should_flush_buffers = true) { return flush_states(FlushStages::States, should_flush_buffers); }

bool flush_cs_all(bool should_flush_buffers, bool async);

DXGI_FORMAT dxgi_format_from_flags(uint32_t cflg);
int dxgi_format_to_bpp(DXGI_FORMAT fmt);
const char *dx11_error(HRESULT hr);
void override_unsupported_bc(uint32_t &cflg);
void set_fullscreen_state(bool fullscreen);
} // namespace drv3d_dx11
