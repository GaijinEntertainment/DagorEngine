//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resPtr.h>

struct Flare
{
  Flare() = default;
  void init(const Point2 low_res_size, const char *lense_covering_tex_name, const char *lense_radial_tex_name);
  void close();
  void apply(Texture *src_tex, TEXTUREID src_id);

  void toggleEnabled(bool enabled);
  void toggleCovering(bool enabled);

  UniqueTex flareTex[2];
  SharedTexHolder flareCoveringTex;
  SharedTexHolder flareColorTex;
  PostFxRenderer *flareDownsample = nullptr;
  PostFxRenderer *flareFeature = nullptr;
  PostFxRenderer *flareBlur = nullptr;
};
