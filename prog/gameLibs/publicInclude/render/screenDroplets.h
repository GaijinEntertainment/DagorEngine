//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <EASTL/unique_ptr.h>
#include <3d/dag_textureIDHolder.h>
#include <fx/dag_fxInterface.h>
#include <shaders/dag_postFxRenderer.h>
#include <3d/dag_resourcePool.h>
#include <render/gaussMipRenderer.h>
#include <math/integer/dag_IPoint2.h>

class ScreenDroplets
{
  float rainOutside = 0;
  float rainCurrent = 0;
  float rainStarted = -100;
  float rainEnded = -100;
  float splashStarted = -100;
  bool isUnderwaterLast = false;
  bool inVehicle = false;
  PostFxRenderer screenDropletsFx;
  RTargetPool::Ptr rtTemp;
  Point3 rainDir = Point3(0, -1, 0);
  bool enabled = false;
  float intensity = 0;
  GaussMipRenderer mipRenderer;
  IPoint2 resolution;

  void updateShaderState() const;
  void render(BaseTexture *rtarget);

public:
  ScreenDroplets(int w, int h, uint32_t rtFmt);
  ScreenDroplets(int w, int h);

  ~ScreenDroplets();
  void setRain(float force, const Point3 &rain_dir);
  void setLeaks(bool has_leaks);
  void setInVehicle(bool in_vehicle);
  void abortDrops();
  // No need to provide dt, if rain_cone_off > rain_cone_max
  // if rain_cone_off < rain_cone_max, then the default dt value is basically instant change
  void update(bool is_underwater, const TMatrix &itm, float dt = 100000);
  void render(ManagedTexView rtarget, TEXTUREID frame_tex);
  RTarget::CPtr render(TEXTUREID frame_tex);

  void setEnabled(bool is_enabled);
  bool isVisible() const;

  // Unsatisfied link-time dependency, shall be implemented by client code
  static bool rayHitNormalized(const Point3 &p, const Point3 &dir, float t);

  static IPoint2 getSuggestedResolution(int rendering_w, int rendering_h);
  IPoint2 getResolution() { return resolution; }


  // Default values should be the same as the convars in screenDroplets.cpp
  struct ConvarParams
  {
    bool hasLeaks = true;
    float onRainFadeoutRadius = 0.3;
    float screenTime = 6;
    float rainConeMax = 0;
    float rainConeOff = 1;
    bool intensityDirectControl = false;
    float intensityChangeRate = 1.0 / 6;
  };
  void setConvars(const ConvarParams &convar_params);
};

extern eastl::unique_ptr<ScreenDroplets> screen_droplets_mgr;
inline ScreenDroplets *get_screen_droplets_mgr() { return screen_droplets_mgr.get(); }
// If rain_cone_off > rain_cone_max then it'll just turn off below rain_cone_max, and always rain with max intensity
void init_screen_droplets_mgr(int w, int h, uint32_t rtFmt, bool has_leaks = false, float rain_cone_max = 0, float rain_cone_off = 1);
void init_screen_droplets_mgr(int w, int h, bool has_leaks = false, float rain_cone_max = 0, float rain_cone_off = 1);

void close_screen_droplets_mgr();
