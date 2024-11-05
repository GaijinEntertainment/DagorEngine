//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_span.h>
#include <drv/3d/dag_tex3d.h>
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
} // namespace texture_util
