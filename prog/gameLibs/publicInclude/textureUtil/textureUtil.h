//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_span.h>
#include <3d/dag_tex3d.h>
#include <EASTL/shared_ptr.h>
#include <EASTL/unique_ptr.h>
#include <3d/dag_resPtr.h>

class ComputeShaderElement;

namespace texture_util
{
enum class TextureRotation
{
  _0,
  _90_CW,
  _180,
  _270_CW,
  _90_CCW = _270_CW,
  _270_CCW = _90_CW
};

bool rotate_texture(TEXTUREID source_texture_id, BaseTexture *dest_texture, TextureRotation rotation);
bool stitch_textures_horizontal(dag::ConstSpan<BaseTexture *> source_textures, BaseTexture *dest_texture);
bool stitch_textures_vertical(dag::ConstSpan<BaseTexture *> source_textures, BaseTexture *dest_texture);
TexPtr stitch_textures_horizontal(dag::ConstSpan<TEXTUREID> source_texture_ids, const char *dest_tex_name, int flags);

class ShaderHelper
{
public:
#define CLEAR_TYPE_VOLTEX_DECL(type, shader_name)      \
private:                                               \
  eastl::unique_ptr<ComputeShaderElement> shader_name; \
                                                       \
public:                                                \
  void clear_##type##_voltex_via_cs(VolTexture *texture);

  CLEAR_TYPE_VOLTEX_DECL(float, clear_float_volmap_cs);
  CLEAR_TYPE_VOLTEX_DECL(float3, clear_float3_volmap_cs);
  CLEAR_TYPE_VOLTEX_DECL(float4, clear_float4_volmap_cs);
  CLEAR_TYPE_VOLTEX_DECL(uint, clear_uint_volmap_cs);
  CLEAR_TYPE_VOLTEX_DECL(uint2, clear_uint2_volmap_cs);
  CLEAR_TYPE_VOLTEX_DECL(uint3, clear_uint3_volmap_cs);
  CLEAR_TYPE_VOLTEX_DECL(uint4, clear_uint4_volmap_cs);
};
eastl::shared_ptr<ShaderHelper> get_shader_helper();
} // namespace texture_util
