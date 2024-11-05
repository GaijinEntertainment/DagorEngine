// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "../DebugLevel.h"
#include "defines.h"

extern DebugLevel hlslDebugLevel;
extern bool hlslEmbedSource;
extern bool enableBindless;

// include prog/engine/drv/drv3d_pc_multi/hlsl_dx.cpp directly, cause it has the desired functionality
#include <hlsl_dx.cpp>

// stubs for irrelevant code/data
DriverCode d3d::get_driver_code() { return DriverCode::make(d3d::null); }
VPROG drv3d_dx11::create_vertex_shader_unpacked(const void *, uint32_t, uint32_t, bool) { return -1; }
FSHADER drv3d_dx11::create_pixel_shader_unpacked(const void *, uint32_t, uint32_t, bool) { return -1; }
__declspec(thread) HRESULT drv3d_dx11::last_hres = S_OK;

DLL_EXPORT bool compile_compute_shader_dx11(const char *hlsl_text, unsigned len, const char *entry, const char *profile,
  Tab<uint32_t> &shader_bin, String &out_err)
{
  return d3d::compile_compute_shader_hlsl(hlsl_text, len, entry, profile, shader_bin, out_err);
}

DLL_EXPORT bool compile_compute_shader_xboxone(const char *, unsigned, const char *, const char *, Tab<uint32_t> &, String &)
{
  return false;
}
