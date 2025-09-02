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
#include <drv/3d/dag_streamOutput.h>
#include <drv/3d/dag_shaderLibrary.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_texture.h>

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

bool d3d::pcwin::set_capture_full_frame_buffer(bool /*ison*/) { return false; }
void d3d::pcwin::set_present_wnd(void *) {}
bool d3d::pcwin::can_render_to_window() { return false; }
BaseTexture *d3d::pcwin::get_swapchain_for_window(void *) { return nullptr; }
void d3d::pcwin::present_to_window(void *) {}
void *d3d::pcwin::get_native_surface(BaseTexture *) { return nullptr; }
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

void d3d::set_stream_output_buffer(int, const StreamOutputBufferSetup &) {}

ShaderLibrary d3d::create_shader_library(const ShaderLibraryCreateInfo &) { return InvalidShaderLibrary; }

void d3d::destroy_shader_library(ShaderLibrary) {}

// texture aliasing

Texture *d3d::alias_tex(Texture *, TexImage32 *, int, int, int, int, const char *) { return nullptr; }

CubeTexture *d3d::alias_cubetex(CubeTexture *, int, int, int, const char *) { return nullptr; }

VolTexture *d3d::alias_voltex(VolTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_array_tex(ArrayTexture *, int, int, int, int, int, const char *) { return nullptr; }

ArrayTexture *d3d::alias_cube_array_tex(ArrayTexture *, int, int, int, int, const char *) { return nullptr; }

bool d3d::discard_tex(BaseTexture *) { return false; }


#include <legacyCaptureImpl.cpp.inl>
