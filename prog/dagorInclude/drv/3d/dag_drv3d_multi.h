//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tabFwd.h>
#include <drv/3d/dag_driver.h>

#if _TARGET_D3D_MULTI
#include <drv/3d/dag_interface_table.h>
#include <dag/dag_vector.h>

class String;
class D3dEventQuery;
class Driver3dInitCallback;

namespace d3d
{
typedef D3dEventQuery EventQuery;

bool init_driver();
bool is_inited();

bool init_video(void *hinst, main_wnd_f *f, const char *wcname, int ncmdshow, void *&mainwnd, void *renderwnd, void *hicon,
  const char *title, Driver3dInitCallback *cb);

void release_driver();
bool fill_interface_table(D3dInterfaceTable &d3dit);
void prepare_for_destroy();
void window_destroyed(void *hwnd);

struct BeforeWindowDestroyedCookie;
BeforeWindowDestroyedCookie *register_before_window_destroyed_callback(eastl::function<void()> callback);
void unregister_before_window_destroyed_callback(BeforeWindowDestroyedCookie *cookie);

static inline bool device_lost(bool *can_reset_now) { return d3di.device_lost(can_reset_now); }
static inline bool reset_device() { return d3di.reset_device(); }

static inline bool update_screen(uint32_t frame_id = 0, bool app_active = true) { return d3di.update_screen(frame_id, app_active); }
static inline void wait_for_async_present(bool force = false) { return d3di.wait_for_async_present(force); }
static inline void begin_frame(uint32_t frame_id, bool allow_wait) { d3di.begin_frame(frame_id, allow_wait); }
static inline void mark_simulation_start(uint32_t frame_id) { d3di.mark_simulation_start(frame_id); }
static inline void mark_simulation_end(uint32_t frame_id) { d3di.mark_simulation_end(frame_id); }
static inline void mark_render_start(uint32_t frame_id) { d3di.mark_render_start(frame_id); }
static inline void mark_render_end(uint32_t frame_id) { d3di.mark_render_end(frame_id); }

static inline GPUFENCEHANDLE insert_fence(GpuPipeline gpu_pipeline) { return d3di.insert_fence(gpu_pipeline); }
static inline void insert_wait_on_fence(GPUFENCEHANDLE &fence, GpuPipeline gpu_pipeline)
{
  d3di.insert_wait_on_fence(fence, gpu_pipeline);
}

static inline bool set_srgb_backbuffer_write(bool on) { return d3di.set_srgb_backbuffer_write(on); }

static inline bool setgamma(float power) { return d3di.setgamma(power); }

static inline float get_screen_aspect_ratio() { return d3di.get_screen_aspect_ratio(); }
static inline void change_screen_aspect_ratio(float ar) { return d3di.change_screen_aspect_ratio(ar); }

static inline void *fast_capture_screen(int &w, int &h, int &stride, int &fmt) { return d3di.fast_capture_screen(w, h, stride, fmt); }
static inline void end_fast_capture_screen() { return d3di.end_fast_capture_screen(); }

static inline TexPixel32 *capture_screen(int &w, int &h, int &stride_bytes) { return d3di.capture_screen(w, h, stride_bytes); }
static inline void release_capture_buffer() { return d3di.release_capture_buffer(); }

static inline int create_predicate() { return d3di.create_predicate(); }
static inline void free_predicate(int id) { d3di.free_predicate(id); }

static inline bool begin_survey(int index) { return d3di.begin_survey(index); }
static inline void end_survey(int index) { return d3di.end_survey(index); }

static inline void begin_conditional_render(int index) { return d3di.begin_conditional_render(index); }
static inline void end_conditional_render(int id) { return d3di.end_conditional_render(id); }

static inline VDECL get_program_vdecl(PROGRAM p) { return d3di.get_program_vdecl(p); }
static inline bool set_vertex_shader(VPROG ps) { return d3di.set_vertex_shader(ps); }
static inline bool set_pixel_shader(FSHADER ps) { return d3di.set_pixel_shader(ps); }

static inline VPROG create_vertex_shader_dagor(const VPRTYPE *p, int n) { return d3di.create_vertex_shader_dagor(p, n); }
static inline FSHADER create_pixel_shader_dagor(const FSHTYPE *p, int n) { return d3di.create_pixel_shader_dagor(p, n); }

static inline bool get_vrr_supported() { return d3di.get_vrr_supported(); }
static inline bool get_vsync_enabled() { return d3di.get_vsync_enabled(); }
static inline bool enable_vsync(bool enable) { return d3di.enable_vsync(enable); }
static inline void get_video_modes_list(Tab<String> &list) { d3di.get_video_modes_list(list); }

static inline void beginEvent(const char *s) { return d3di.beginEvent(s); }
static inline void endEvent() { return d3di.endEvent(); }

#include "rayTrace/rayTraceMulti.inl.h"

static inline void resource_barrier(const ResourceBarrierDesc &desc, GpuPipeline gpu_pipeline = GpuPipeline::GRAPHICS)
{
  d3di.resource_barrier(desc, gpu_pipeline);
}

#if _TARGET_PC_WIN | _TARGET_PC_MACOSX
namespace pcwin
{
static inline void set_present_wnd(void *hwnd) { return d3di.set_present_wnd(hwnd); }

static inline bool can_render_to_window() { return d3di.can_render_to_window(); }
static inline BaseTexture *get_swapchain_for_window(void *hwnd) { return d3di.get_swapchain_for_window(hwnd); }
static inline void present_to_window(void *hwnd) { return d3di.present_to_window(hwnd); }

static inline bool set_capture_full_frame_buffer(bool ison) { return d3di.set_capture_full_frame_buffer(ison); }
static inline unsigned get_texture_format(const BaseTexture *tex) { return d3di.get_texture_format(tex); }
static inline const char *get_texture_format_str(const BaseTexture *tex) { return d3di.get_texture_format_str(tex); }
static inline void *get_native_surface(BaseTexture *tex) { return d3di.get_native_surface(tex); }

dag::ConstSpan<DriverCode> get_supported_graphics_apis();
} // namespace pcwin
#endif
}; // namespace d3d
#endif
