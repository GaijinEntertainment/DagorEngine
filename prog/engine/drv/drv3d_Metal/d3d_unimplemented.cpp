
#include <ioSys/dag_memIo.h>
#include <ioSys/dag_zlibIo.h>

#include <3d/dag_drv3d.h>
#include "drv_utils.h"

#include <ioSys/dag_dataBlock.h>
#include <startup/dag_globalSettings.h>
#include <resUpdateBufferGeneric.h>
#include <util/dag_string.h>
#include <renderPassGeneric.h>
#include <resourceActivationGeneric.h>

#if _TARGET_PC_MACOSX
#include <3d/dag_drv3d_pc.h>
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

int get_retina_mode() { return dgs_get_settings()->getBlockByNameEx("video")->getInt("retina_mode", 0); }

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
bool d3d::setantialias(int aa_type) { return false; }

int d3d::getantialias() { return 0; }

void d3d::discard_managed_textures() {}

void d3d::change_screen_aspect_ratio(float ar) {}

// Screen capture

/// capture screen to TexPixel32 buffer
/// slow, and not 100% guaranted
/// returns NULL on error
TexPixel32 *d3d::capture_screen(int &w, int &h, int &stride_bytes) { return 0; }

/// release buffer used to capture screen
void d3d::release_capture_buffer() {}

bool d3d::setwire(bool in) { return false; }

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

// returns true if hardware supports depth bounds. same as get_driver_desc().caps.hasDepthBoundsTest
bool d3d::supports_depth_bounds() { return false; }

bool d3d::set_tex_usage_hint(int w, int h, int mips, const char *format, unsigned int tex_num) { return false; }

Texture *d3d::get_backbuffer_tex() { return nullptr; }
Texture *d3d::get_secondary_backbuffer_tex() { return nullptr; }
Texture *d3d::get_backbuffer_tex_depth() { return nullptr; }

// Immediate constant buffers - valid within min(driver acquire, frame)
// to unbind, use NULL, 0 params
// if slot = 0 is empty (PS/VS stages), buffered constants are used
bool d3d::set_const_buffer(unsigned stage, unsigned slot, const float *data, unsigned num_regs) { return false; }

d3d::SamplerHandle d3d::create_sampler(const d3d::SamplerInfo &sampler_info) { return 0; }
void d3d::destroy_sampler(d3d::SamplerHandle sampler) {}

void d3d::set_sampler(unsigned shader_stage, unsigned slot, d3d::SamplerHandle sampler) {}

uint32_t d3d::register_bindless_sampler(BaseTexture *)
{
  G_ASSERTF(false, "d3d::register_bindless_sampler called on API without support");
  return 0;
}

uint32_t d3d::allocate_bindless_resource_range(uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::allocate_bindless_resource_range called on API without support");
  return 0;
}

uint32_t d3d::resize_bindless_resource_range(uint32_t, uint32_t, uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::resize_bindless_resource_range called on API without support");
  return 0;
}

void d3d::free_bindless_resource_range(uint32_t, uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::free_bindless_resource_range called on API without support");
}

void d3d::update_bindless_resource(uint32_t, D3dResource *)
{
  G_ASSERTF(false, "d3d::update_bindless_resource called on API without support");
}

void d3d::update_bindless_resources_to_null(uint32_t, uint32_t, uint32_t)
{
  G_ASSERTF(false, "d3d::update_bindless_resource_to_null called on API without support");
}

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
Sbuffer *d3d::place_buffere_in_resource_heap(ResourceHeap *heap, const ResourceDescription &desc, size_t offset,
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

bool d3d::get_vrr_supported() { return false; }

IMPLEMENT_D3D_RESOURCE_ACTIVATION_API_USING_GENERIC();
IMPLEMENT_D3D_RUB_API_USING_GENERIC()
IMPLEMENT_D3D_RENDER_PASS_API_USING_GENERIC()
