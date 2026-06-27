//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class String;

// Runtime HLSL->shader hook for editor texgen. The Vulkan back end has no driver-side runtime HLSL
// path, so on Linux texgen compiles each node's shader through this hook instead of
// d3d::create_*_shader_hlsl. The daEditorX editor build installs it (see
// pluginService/texgen_hlsl_compile.cpp, which links gameLibs/spirv); it stays null elsewhere, where
// texgen reports a bad shader. Returns a ready VPROG (is_vertex) or FSHADER handle, or the matching
// BAD_* sentinel on failure.
extern int (*texgen_compile_hlsl_shader)(const char *hlsl, int len, const char *entry, const char *profile, bool is_vertex,
  String &out_err);
