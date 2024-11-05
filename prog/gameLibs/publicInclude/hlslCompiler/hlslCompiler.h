//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <util/dag_string.h>
#include <generic/dag_span.h>

#include <debug/dag_debug.h>

// Used as a thin wrapper for hlslCompiler dynlib that provides functions for compiling hlsl into driver-compatible formats.
// To be used only in tools and development.
//
// Currently only compute shader compilation is supported

namespace hlsl_compiler
{

enum class Platform
{
  DX11 = 0,
  PS4,
  VULKAN,
  DX12,
  DX12_XBOX_ONE,
  DX12_XBOX_SCARLETT,
  PS5,
  MTL,
  VULKAN_BINDLESS,
  MTL_BINDLESS
};

bool is_intialized();

bool try_init_dynlib(const char *dll_root);
void deinit_dynlib();

bool compile_compute_shader(Platform platform, dag::ConstSpan<char> hlsl_src, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err);

// Not implemented
inline bool compile_pixel_shader(Platform, dag::ConstSpan<char>, const char *, const char *, Tab<uint32_t> &, String &)
{
  logwarn("Pixel shader compilation is not yet supported by hlslCompiler!");
  return false;
}
inline bool compile_vertex_shader(Platform, dag::ConstSpan<char>, const char *, const char *, Tab<uint32_t> &, String &)
{
  logwarn("Vertex shader compilation is not yet supported by hlslCompiler!");
  return false;
}

} // namespace hlsl_compiler