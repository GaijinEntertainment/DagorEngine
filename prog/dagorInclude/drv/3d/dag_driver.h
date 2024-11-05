//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_barrier.h>
#include <drv/3d/dag_consts.h>
#include <drv/3d/dag_resource.h>
#include <drv/3d/dag_tex3d.h>
#include <util/dag_globDef.h>
#include <vecmath/dag_vecMathDecl.h>
#include <EASTL/initializer_list.h>
#include <drv/3d/dag_hangHandler.h>

// forward declarations for external classes
struct D3dInterfaceTable;
class Driver3dInitCallback;
class Sbuffer;
enum class Drv3dCommand;

//--- Driver3dDesc & initialization -------
#include "rayTrace/dag_drvRayTrace.h"

// main window proc
typedef intptr_t main_wnd_f(void *, unsigned, uintptr_t, intptr_t);

namespace d3d
{
//! opaque class that holds data for resource/texture update
class ResUpdateBuffer;
} // namespace d3d

//--- 3d driver interface -------
namespace d3d
{
enum
{
  USAGE_TEXTURE = 0x01,
  USAGE_DEPTH = 0x02,
  USAGE_RTARGET = 0x04,
  USAGE_AUTOGENMIPS = 0x08,
  USAGE_FILTER = 0x10,
  USAGE_BLEND = 0x20,
  USAGE_VERTEXTEXTURE = 0x40,
  USAGE_SRGBREAD = 0x80,
  USAGE_SRGBWRITE = 0x100,
  USAGE_SAMPLECMP = 0x200,
  USAGE_PIXREADWRITE = 0x400,
  USAGE_TILED = 0x800,
  USAGE_UNORDERED = 0x1000,
  /// Indicates the format supports unordered loads
  USAGE_UNORDERED_LOAD = 0x2000
};

enum
{
  CAPFMT_X8R8G8B8,
  CAPFMT_R8G8B8,
  CAPFMT_R5G6B5,
  CAPFMT_X1R5G5B5,
};

void update_window_mode();

static constexpr int RENDER_TO_WHOLE_ARRAY = 1023;
#if !_TARGET_D3D_MULTI
// Driver initialization API

/// initalizes 3d device driver
bool init_driver();

/// returns true when d3d API is inited
bool is_inited();

/// start up, read cfg, create window, init 3d hard&soft, ...
/// if renderwnd!=NULL, mainwnd and renderwnd are used for rendering (when possible)
/// on error, returns false
/// on sucess, return true
bool init_video(void *hinst, main_wnd_f *, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb);

/// shutdown driver and release all resources
void release_driver();

/// fills function-pointers table for d3d API, or returns false if unsupported
bool fill_interface_table(D3dInterfaceTable &d3dit);

/// notify driver before window destruction
void prepare_for_destroy();

/// notify driver on window destruction
void window_destroyed(void *hwnd);

struct BeforeWindowDestroyedCookie;
BeforeWindowDestroyedCookie *register_before_window_destroyed_callback(eastl::function<void()> callback);
void unregister_before_window_destroyed_callback(BeforeWindowDestroyedCookie *cookie);

// Device management

/// returns true when device is lost; when it is safe to reset device can_reset_now is set with 1
/// returns false when device is ok
bool device_lost(bool *can_reset_now);

/// performs device reset; returns true if succeded
bool reset_device();

// Rendering

/// flip video pages or copy to screen
/// app_active==false is hint to the driver
/// returns false on error
bool update_screen(bool app_active = true);

/**
 * @brief If the update_screen(bool) works asynchronously then under certain conditions it will block the execution
 * until the previously called update_screen(bool) is finished.
 * @param force Even if the internal conditions don't require the wait, the execution will wait for the present to finish
 */
void wait_for_async_present(bool force = false);

/**
 * @brief It blocks the execution until the GPU finishes on going render task for delaying the input sampling and using the most recent
 * user inputs for rendering the next frame.
 */
void gpu_latency_wait();

GPUFENCEHANDLE insert_fence(GpuPipeline gpu_pipeline);
void insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline);
// additional L2 cache flush calls would need to be added to support CPU <-> Compute Context data communication
// (at least on Xbox One, see XDK docs, article "Optimizing Monolithic Driver Performance", part "Manual Cache and Pipeline Flushing")

// Miscellaneous
bool set_srgb_backbuffer_write(bool); // returns previous result. switch on/off srgb write to backbuffer (default is off)

bool set_msaa_pass();

bool set_depth_resolve();

bool setgamma(float);

float get_screen_aspect_ratio();
void change_screen_aspect_ratio(float ar);

// Screen capture

/// capture screen to buffer.fast, but not guaranteed
/// many captures can be followed by only one end_fast_capture_screen()
void *fast_capture_screen(int &w, int &h, int &stride_bytes, int &format);
void end_fast_capture_screen();

/// capture screen to TexPixel32 buffer
/// slow, and not 100% guaranted
/// returns NULL on error
TexPixel32 *capture_screen(int &w, int &h, int &stride_bytes);
/// release buffer used to capture screen
void release_capture_buffer();

//! conditional rendering.
//! conditional rendering is used to skip rendering of triangles completelyon GPU.
//! the only commands, that would be ignored, if survey fails are DIPs
//! (all commands and states will still be executed), so it is better to use
//! reports to completely skip object rendering
//! max index is defined per platform
//! surveying.
int create_predicate(); // -1 if not supported or something goes wrong
void free_predicate(int id);

bool begin_survey(int id); // false if not supported or bad id
void end_survey(int id);

void begin_conditional_render(int id);
void end_conditional_render(int id);

void beginEvent(const char *); // for debugging
void endEvent();               // for debugging
bool get_vrr_supported();
bool get_vsync_enabled();
bool enable_vsync(bool enable);

#include "rayTrace/rayTracedrv3d.inl.h"

// See ResourceBarrierDesc and https://info.gaijin.lan/display/DE4/Resource+and+Execution+Barriers
void resource_barrier(ResourceBarrierDesc desc, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS);

#endif

#if _TARGET_XBOX || _TARGET_C1 || _TARGET_C2
void resummarize_htile(BaseTexture *tex);
#else
inline void resummarize_htile(BaseTexture *) {}
#endif

#if _TARGET_XBOX
void set_esram_layout(const wchar_t *);
void unset_esram_layout();
void reset_esram_layout();
void prefetch_movable_textures();
void writeback_movable_textures();
#else
inline void set_esram_layout(const wchar_t *) {}
inline void unset_esram_layout() {}
inline void reset_esram_layout() {}
inline void prefetch_movable_textures() {}
inline void writeback_movable_textures() {}
#endif

#if _TARGET_C1 || _TARGET_XBOX
#define D3D_HAS_QUADS 1
#else
#define D3D_HAS_QUADS 0
#endif
}; // namespace d3d


extern void
#if _MSC_VER >= 1300
  __declspec(noinline)
#endif
    d3derr_in_device_reset(const char *msg);
extern bool dagor_d3d_force_driver_reset;

#define d3derr(c, m)                                                   \
  do                                                                   \
  {                                                                    \
    bool res = (c);                                                    \
    G_ANALYSIS_ASSUME(res);                                            \
    if (!res)                                                          \
    {                                                                  \
      bool canReset;                                                   \
      if (dagor_d3d_force_driver_reset || d3d::device_lost(&canReset)) \
        d3derr_in_device_reset(m);                                     \
      else                                                             \
        DAG_FATAL("%s:\n%s", m, d3d::get_last_error());                \
    }                                                                  \
  } while (0)

#define d3d_err(c) d3derr((c), "Driver3d error")

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_drv3d_multi.h>
#endif
