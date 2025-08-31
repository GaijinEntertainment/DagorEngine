// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zlibIo.h>

#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tiledResource.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_bindless.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_shaderLibrary.h>
#include "drv_utils.h"

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <resUpdateBufferGeneric.h>
#include <util/dag_string.h>
#include <resourceActivationGeneric.h>

#if _TARGET_PC_MACOSX
#include <drv/3d/dag_platform_pc.h>
#endif

void getCodeFromZ(const char *source, int sz, Tab<char> &code)
{
  InPlaceMemLoadCB crd(source, sz);
  ZlibLoadCB zlib_crd(crd, sz);

  int size = zlib_crd.readInt();

  code.resize(size + 1);
  zlib_crd.readExact(&code[0], size);
  code[size] = 0;

  zlib_crd.ceaseReading();
}

bool getUseRetina() { return get_settings_use_retina(); }

bool get_metal_settings_resolution(int &width, int &height, bool &is_retina, int def_width, int def_height, bool &out_is_auto)
{
  return get_settings_resolution(width, height, is_retina, def_width, def_height, out_is_auto);
}

bool get_allow_intel4000() { return dgs_get_settings()->getBlockByNameEx("video")->getBool("intel4000", false); }

bool get_lowpower_mode() { return dgs_get_settings()->getBlockByNameEx("video")->getBool("lowpower", false); }

bool get_aync_pso_compilation_supported() { return dgs_get_settings()->getBlockByNameEx("graphics")->getBool("asyncPSO", false); }
uint32_t get_shader_cache_version() { return dgs_get_settings()->getBlockByNameEx("graphics")->getInt("psoCacheVersion", 0); }

GPUFENCEHANDLE d3d::insert_fence(GpuPipeline /*gpu_pipeline*/) { return BAD_GPUFENCEHANDLE; }
void d3d::insert_wait_on_fence(GPUFENCEHANDLE & /*fence*/, GpuPipeline /*gpu_pipeline*/) {}

#if _TARGET_PC_MACOSX
VPROG d3d::create_vertex_shader_dagor(const VPRTYPE *p, int n) { return -1; }

FSHADER d3d::create_pixel_shader_dagor(const FSHTYPE *p, int n) { return -1; }

bool d3d::set_vertex_shader(VPROG vs) { return false; }

bool d3d::set_pixel_shader(FSHADER ps) { return false; }

VDECL d3d::get_program_vdecl(PROGRAM p) { return -1; }
#endif

// Device management
void *d3d::get_device() { return 0; }

/// returns raw pointer to device interface
void *d3d::get_context() { return 0; }

// Render states

void d3d::change_screen_aspect_ratio(float ar) {}

// Screen capture

/// capture screen to TexPixel32 buffer
/// slow, and not 100% guaranted
/// returns NULL on error
TexPixel32 *d3d::capture_screen(int &w, int &h, int &stride_bytes) { return 0; }

/// release buffer used to capture screen
void d3d::release_capture_buffer() {}

//! conditional rendering.
//! conditional rendering is used to skip rendering of triangles completelyon GPU.
//! the only commands, that would be ignored, if survey fails are DIPs
//! (all commands and states will still be executed), so it is better to use
//! reports to completely skip object rendering
//! max index is defined per platform
//! surveying.
// -1 if not supported or something goes wrong
int d3d::create_predicate() { return -1; }

void d3d::free_predicate(int id) {}

// false if not supported or bad id
bool d3d::begin_survey(int id) { return false; }

void d3d::end_survey(int id) {}

void d3d::begin_conditional_render(int id) {}

void d3d::end_conditional_render(int id) {}

bool d3d::set_depth_bounds(float zmin, float zmax) { return false; }

Texture *d3d::get_secondary_backbuffer_tex() { return nullptr; }

#if _TARGET_PC_MACOSX
bool d3d::pcwin::set_capture_full_frame_buffer(bool /*ison*/) { return false; }
void d3d::pcwin::set_present_wnd(void *) {}
unsigned d3d::pcwin::get_texture_format(const BaseTexture *) { return 0; }
const char *d3d::pcwin::get_texture_format_str(const BaseTexture *) { return "n/a"; }
void *d3d::pcwin::get_native_surface(BaseTexture *) { return nullptr; }
#endif

// Immediate constant buffers - valid within min(driver acquire, frame)
// to unbind, use NULL, 0 params
// if slot = 0 is empty (PS/VS stages), buffered constants are used
bool d3d::set_const_buffer(unsigned stage, unsigned slot, const float *data, unsigned num_regs) { return false; }

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

bool d3d::get_vrr_supported() { return false; }

ShaderLibrary d3d::create_shader_library(const ShaderLibraryCreateInfo &) { return InvalidShaderLibrary; }

void d3d::destroy_shader_library(ShaderLibrary) {}

void d3d::wait_for_async_present(bool force) { G_UNUSED(force); }

void d3d::begin_frame(uint32_t, bool) {}

void d3d::mark_simulation_start(uint32_t) {}
void d3d::mark_simulation_end(uint32_t) {}
void d3d::mark_render_start(uint32_t) {}
void d3d::mark_render_end(uint32_t) {}

IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC();

#include <legacyCaptureImpl.cpp.inl>
