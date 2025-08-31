//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/3d/dag_resId.h>
#include <3d/dag_ringCPUTextureLock.h>
#include <3d/dag_textureIDHolder.h>
#include <shaders/dag_postFxRenderer.h>
#include <generic/dag_tab.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_Point4.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>

class ToroidalHeightmap;
class AcesScene;
class VisibilityFinder;

// for rendering of rendinst/static scene, which also should affect physical displacement
class IRenderGroundPhysdetailsCB
{
public:
  virtual void renderGroundPhysdetails(const Point3 &, mat44f_cref, VisibilityFinder &) = 0;
};

class IRenderLandmeshPhysdetailsCB
{
public:
  virtual void renderLandmeshPhysdetails(const Point3 &, const BBox2 &) = 0;
};

class GroundDisplacementCPU
{
public:
  GroundDisplacementCPU();
  ~GroundDisplacementCPU();
  void init(int tex_size = 128, float heightmap_texel_size = 0.2f, float invalidation_scale = 0.3f);
  void beforeRenderFrame(float dt);
  void update(const Point3 &center_pos, ToroidalHeightmap *hmap);
  float getDisplacementHeight(const Point3 &world_pos);
  float getYOffset(const Point3 &world_pos, const float &hmap_height, float tank_speed, float ground_normal_y,
    float current_offset) const;
  bool isDisplaced(const Point3 &world_pos) const;
  void setMaxPhysicsHeight(const float &height, const float &radius);
  void updateGroundPhysdetails(IRenderGroundPhysdetailsCB *render_depth_cb, VisibilityFinder &vf);
  void invalidate();
  const float maxSpeedUp = 0.75f;
  const float maxSpeedDown = -0.35f;

protected:
  float interpolateOffset(float new_offset, float current_offset, float delta_time) const;
  bool process(float *destData, uint32_t &frame);
  __forceinline float getPixel(int x, int y) const
  {
    x = clamp(x, 0, bufferSize - 1);
    y = clamp(y, 0, bufferSize - 1);
    return loadedDisplacement[x + y * bufferSize];
  }
  PostFxRenderer renderHeightAndDisplacement;
  PostFxRenderer renderGPUHeightAndDisplacement;
  float texelSize = 0.2f;
  int bufferSize = 128;
  float worldSize = 25.6f;
  float invalidationScale = 0.3f;
  float deltaTime = 0.1f;
  Point2 origin;
  Point2 curOrigin;
  Point4 worldToDisplacement;
  Point3 physDetailsOrigin;
  Point3 physDetailsCurOrigin;
  RingCPUTextureLock ringTextures;
  TextureIDHolder groundPhysDetailsTex;
  TextureIDHolderWithVar GPUgroundPhysDetailsTex;
  Tab<float> loadedDisplacement;
  int forceUpdateCounter = 0;
};

class LandmeshPhysmatsCPU
{
public:
  LandmeshPhysmatsCPU();
  ~LandmeshPhysmatsCPU();
  void init(int tex_size = 256, float landmesh_texel_size = 4.0f, float invalidation_scale = 0.3f);
  void update(const Point3 &center_pos, IRenderLandmeshPhysdetailsCB *render_physmats_cb);
  int getPhysmat(const Point3 &world_pos) const;
  void invalidate();

protected:
  bool process(uint8_t *destData, uint32_t &frame);
  __forceinline uint8_t getPixel(int x, int y) const
  {
    x = clamp(x, 0, bufferSize - 1);
    y = clamp(y, 0, bufferSize - 1);
    return loadedPhysmats[x + y * bufferSize];
  }
  PostFxRenderer renderPhysmats;
  float texelSize = 4.0f;
  int bufferSize = 256;
  float worldSize = 1024.0f;
  float invalidationScale = 0.3f;
  float deltaTime = 0.1f;
  Point2 origin;
  Point2 curOrigin;
  Point4 worldToPhysmats;
  RingCPUTextureLock ringTextures;

  Tab<uint8_t> loadedPhysmats;
  int forceUpdateCounter = 0;
};
