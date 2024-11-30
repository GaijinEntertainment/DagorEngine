// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_tiledResource.h>
#include <drv/3d/dag_dispatchMesh.h>
#include <drv/3d/dag_shader.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_info.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_variableRateShading.h>
#include <drv/3d/dag_shaderLibrary.h>

bool d3d::is_window_occluded() { return false; }
bool d3d::should_use_compute_for_image_processing(std::initializer_list<unsigned>) { return false; }
Texture *d3d::get_secondary_backbuffer_tex() { return nullptr; }

// some display related functionality

bool d3d::setgamma(float) { return false; }
void d3d::change_screen_aspect_ratio(float) {}
bool d3d::get_vrr_supported() { return false; }

// mesh shaders

void d3d::dispatch_mesh(uint32_t /* thread_group_x */, uint32_t /* thread_group_y */, uint32_t /* thread_group_z */)
{
  G_ASSERT_RETURN(!"VK: dispatch_mesh not implemented yet", );
}

void d3d::dispatch_mesh_indirect(Sbuffer * /* args */, uint32_t /* dispatch_count */, uint32_t /* stride_bytes */,
  uint32_t /* byte_offset */)
{
  G_ASSERT_RETURN(!"VK: dispatch_mesh not implemented yet", );
}

void d3d::dispatch_mesh_indirect_count(Sbuffer * /*args*/, uint32_t /*args_stride_bytes*/, uint32_t /* args_byte_offset */,
  Sbuffer * /* count */, uint32_t /* count_byte_offset */, uint32_t /* max_count */)
{
  G_ASSERT_RETURN(!"VK: dispatch_mesh not implemented yet", );
}

// multi views

bool d3d::setviews(dag::ConstSpan<Viewport> /* viewports */)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}

bool d3d::setscissors(dag::ConstSpan<ScissorRect> /* scissorRects */)
{
  G_ASSERTF(false, "Not implemented");
  return false;
}

// old/debug PC dev specific stuff

#if _TARGET_PC_WIN
FSHADER d3d::create_pixel_shader_hlsl(const char *, unsigned, const char *, const char *, String *)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

VPROG d3d::create_vertex_shader_hlsl(const char *, unsigned, const char *, const char *, String *)
{
  G_ASSERT(false);
  return BAD_PROGRAM;
}

bool d3d::pcwin32::set_capture_full_frame_buffer(bool /*ison*/) { return false; }
void d3d::pcwin32::set_present_wnd(void *) {}
void *d3d::pcwin32::get_native_surface(BaseTexture *) { return nullptr; }
#endif

// variable shading rate

void d3d::set_variable_rate_shading(unsigned, unsigned, VariableRateShadingCombiner, VariableRateShadingCombiner)
{
  // needs VK_NV_shading_rate_image to support it
  // or VK_KHR_variable_rate_shading
}
void d3d::set_variable_rate_shading_texture(BaseTexture *)
{
  // needs VK_NV_shading_rate_image to support it
  // or VK_KHR_variable_rate_shading
}

// tiled (partial-resident) resources

void d3d::map_tile_to_resource(BaseTexture * /* tex */, ResourceHeap * /* heap */, const TileMapping * /* mapping */,
  size_t /* mapping_count */)
{}
TextureTilingInfo d3d::get_texture_tiling_info(BaseTexture * /* tex */, size_t /* subresource */) { return TextureTilingInfo{}; }

Texture *d3d::get_backbuffer_tex() { return nullptr; }

ShaderLibrary d3d::create_shader_library(const ShaderLibraryCreateInfo &) { return InvalidShaderLibrary; }

void d3d::destroy_shader_library(ShaderLibrary) {}

#include <legacyCaptureImpl.cpp.inl>
