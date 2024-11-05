//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_bounds3.h>
#include <render/toroidalHelper.h>
#include <shaders/dag_overrideStateId.h>

class GrassTranlucencyCB
{
public:
  virtual void start(const BBox3 &box) = 0;
  virtual void renderTranslucencyMask() = 0;
  virtual void renderTranslucencyColor() = 0;
  virtual void finish() = 0;
};

class GrassTranslucency
{
public:
  GrassTranslucency() : grassAmount(NO), grass_tex_size(-1) {}
  ~GrassTranslucency() { close(); }
  void close();
  void init(int tex_size);
  enum
  {
    NO,
    SOME,
    GRASSY
  };
  void setGrassAmount(int grass_amount);
  void invalidate()
  {
    last_grass_color_box.setempty();
    torHelper.curOrigin = IPoint2(-1000000, 100000);
  }
  bool update(const Point3 &view_pos, float half_size, GrassTranlucencyCB &cb);

protected:
  void recreateTex(int sz);
  TextureIDHolderWithVar grass_color_tex, grass_mask_tex;
  PostFxRenderer decode_grass_mask;
  BBox3 last_grass_color_box;
  int grass_tex_size;
  int grassAmount;
  ToroidalHelper torHelper;
  shaders::UniqueOverrideStateId flipCullOverride, flipCullAlphaOverride;
};
