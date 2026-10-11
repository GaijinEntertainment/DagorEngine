//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_carray.h>
#include <math/dag_Point3.h>
#include <math/dag_integer.h>
#include <3d/dag_resPtr.h>
#include <shaders/dag_postFxRenderer.h>
#include <render/specialVision/specialVision.h>

class NightVisionPostFx
{
public:
  NightVisionPostFx();
  ~NightVisionPostFx();

  NightVisionPostFx(const NightVisionPostFx &) = delete;
  NightVisionPostFx &operator=(const NightVisionPostFx &) = delete;

  bool init(bool full_deferred);
  void resolve();
  void releaseTargets();
  void initTargets(int ch_state, int sizeX, int sizeY, int targetsizeX, int targetsizeY);
  void invalidatePrevFrameTextures();
  int getState() const { return state; }
  bool hasTargets() const { return glowTex[0].getTex2D() != nullptr; }
  void apply(Texture *src_tex, Texture *dest_tex);
  void applySettings(float nv_ghosting, float th_ghosting, Point3 nv_baseColor, Point3 nv_midColor, Point3 nv_brightColor,
    float nv_lightMultiplier, float nv_noiseFactor, bool black_is_hot);
  IPoint2 sourceResolution;

protected:
  carray<UniqueTex, 2> glowTex;
  UniqueTex tempTex;
  UniqueTex downsampleTex;

  PostFxRenderer applyFullscreen;
  PostFxRenderer blur;
  PostFxRenderer downsample;
  PostFxRenderer resolveShader;

  int state = render::special_vision::OFF;
  IPoint2 baseResolution, glowResolution, currentResolution;
  int currentTexIndex;
  bool fullDeferred = true;
  bool invalidatedPrevFrameTextures = false;
  bool isInitialized = false;
};
