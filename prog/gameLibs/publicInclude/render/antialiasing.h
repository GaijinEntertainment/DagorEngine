//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <math/dag_Point4.h>


class PostFxRenderer;
class BaseTexture;
typedef BaseTexture Texture;


enum FxaaType
{
  AA_TYPE_NONE = 0,
  AA_TYPE_LOW_FXAA = 1,
  AA_TYPE_HIGH_FXAA = 2
};


class Antialiasing
{
public:
  Antialiasing();
  ~Antialiasing();

  void apply(TEXTUREID source_color_tex_id, TEXTUREID source_depth_tex_id,
    const Point4 &tc_scale_offset = Point4(1.0f, 1.0f, 0.0f, 0.0f));

  void applyAdaptiveSharpen(TEXTUREID source_color_tex_id);
  void setType(FxaaType type);
  void setColorMul(const Color4 &color);

protected:
  int sourceColorTexVarId;
  int sourceDepthTexVarId;
  PostFxRenderer *antialiasingRenderer;
  PostFxRenderer *adaptiveSharpenRenderer;
  int fxaaHighQuality;
};
