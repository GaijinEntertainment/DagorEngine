//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#if !(_TARGET_PC | _TARGET_XBOX)
#error must not be included for non-PC target!
#endif

#include <drv/3d/dag_driver.h>
#include <generic/dag_tabFwd.h>

class String;

namespace d3d
{
#if _TARGET_PC_WIN
VPROG create_vertex_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile, String *out_err = NULL);
FSHADER create_pixel_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile, String *out_err = NULL);
bool compile_compute_shader_hlsl(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);
#endif

#if !_TARGET_D3D_MULTI
VDECL get_program_vdecl(PROGRAM);

bool set_vertex_shader(VPROG ps);
bool set_pixel_shader(FSHADER ps);
#if _TARGET_PC_WIN | _TARGET_PC_MACOSX
namespace pcwin
{
//! return D3DFORMAT for given texture
unsigned get_texture_format(const BaseTexture *tex);
//! return D3DFORMAT for given texture as string
const char *get_texture_format_str(const BaseTexture *tex);
void *get_native_surface(BaseTexture *tex);
} // namespace pcwin
#endif

VPROG create_vertex_shader_asm(const char *asm_text);
VPROG create_vertex_shader_dagor(const VPRTYPE *p, int n);

FSHADER create_pixel_shader_asm(const char *asm_text);
FSHADER create_pixel_shader_dagor(const FSHTYPE *p, int n);

#if _TARGET_PC_WIN | _TARGET_PC_MACOSX
// additional d3d::pcwin interface (PC specific)
namespace pcwin
{
void set_present_wnd(void *hwnd);

bool can_render_to_window();
BaseTexture *get_swapchain_for_window(void *hwnd);
void present_to_window(void *hwnd);

// set capture whole framebuffer with capture_screen(), not only window data. returns previous state.
bool set_capture_full_frame_buffer(bool ison);
} // namespace pcwin
#endif
//! returns current state of VSYNC
bool get_vsync_enabled();
//! enables or disables strong VSYNC (flips only on VBLANK); returns true on success
bool enable_vsync(bool enable);
//! retrieve list of available display modes
void get_video_modes_list(Tab<String> &list);
#endif
}; // namespace d3d
