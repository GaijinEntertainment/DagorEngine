//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <shaders/dag_shBindumps.h>

class TextureGenerator;
class DataBlock;

// Register a texgen node-shader backed by a precompiled dshl bindump variant (Stage-3 dshl pipeline),
// the dshl-path counterpart of add_pixel_shader_texgen. `variant_name` is the shader-class name inside
// `bindump` (produced by assemble_texgen_dshl + dsc2). `params_blk` is the source shader's params{}
// block (drives the runtime constant-buffer packing, same as the HLSL path). The created shader renders
// via ShaderElement::setStates() while texgen keeps binding textures/params/UAVs raw.
extern bool add_dshl_shader_texgen(TextureGenerator *tex_gen, const char *shader_name, const char *variant_name,
  ShaderBindumpHandle bindump, const DataBlock *params_blk, int inputs, int outputs, int def_sub_size);
