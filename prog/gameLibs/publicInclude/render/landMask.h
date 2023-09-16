//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_texMgr.h>
#include <3d/dag_drv3dConsts.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_tab.h>
#include <math/dag_TMatrix4.h>
#include <math/integer/dag_IPoint2.h>

#include <render/toroidalHelper.h>
#include <render/toroidal_update.h>
#include <shaders/dag_overrideStateId.h>

class BaseTexture;
typedef BaseTexture Texture;
class RenderableInstanceLodsResource;
class Clipmap;
class LandMeshManager;
class LandMeshRenderer;
class PostFxRenderer;
class BBox2;
class DataBlock;

struct ILandMaskRenderHelper
{
  virtual bool beginRender(const Point3 &center_pos, const BBox3 &box, const TMatrix4 &tm) = 0;
  virtual void endRender() = 0;
  virtual void renderHeight(float min_height, float max_height) = 0;
  virtual void renderColor() = 0;
  virtual void renderMask() = 0;
  virtual void renderExplosions() = 0;
  virtual bool isValid() const = 0;
};

class LandMask
{
public:
  LandMask(const DataBlock &level_blk, int tex_align, bool need_grass);
  ~LandMask();

  void loadParams(const DataBlock &level_blk);
  void invalidate();

  void beforeRender(const Point3 &center_pos, ILandMaskRenderHelper &render_helper, bool force_update = false);

  Texture *getLandHeightTex() const { return landHeightTex.getTex2D(); }
  Texture *getLandColorTex() const { return landColorTex.getTex2D(); }
  Texture *getGrassTypeTex() const { return grassTypeTex.getTex2D(); }
  BBox2 getRenderBbox() const;

  int getLandTexSize() const { return landTexSize; }

  float getWorldSize() const { return worldSize; }
  float getWorldAlign() const { return worldAlign; }
  void setWorldSize(float world_size, float world_align)
  {
    world_align = max(world_align, 1.0f);
    world_size = max(ceilf(world_size / world_align) * world_align, 1.0f);
    if (fabsf(worldSize - world_size) < 1e-3f && fabsf(worldAlign - world_align) < 1e-3f)
      return;
    worldAlign = world_align;
    worldSize = world_size;
    invalidate();
  }
  void invalidateBox(const BBox2 &box);

  Point2 getCenterPos() const { return Point2::xy(squareCenter) * worldAlign; }

  float getMinLandHeight() const { return minLandHeight; }
  float getMaxLandHeight() const { return maxLandHeight; }

  bool isInitialized() const;
  bool hasGrass() const { return getLandColorTex() != NULL; }

protected:
  void renderRegion(const Point3 &center_pos, const ToroidalQuadRegion &reg, ILandMaskRenderHelper &render_helper,
    const TMatrix &view);

  void addRegionToRender(const ToroidalQuadRegion &reg);

  float worldAlign;
  int texAlign;
  float worldSize;
  float maxSlope;

  IPoint2 squareCenter;

  float minLandHeight;
  float maxLandHeight;

  UniqueTexHolder landHeightTex;
  UniqueTexHolder landColorTex;
  UniqueTexHolder grassTypeTex;
  int landTexSize;

  PostFxRenderer *filterRenderer = NULL;

  Tab<int> shaderVars;
  Tab<IBBox2> invalidRegions;
  Tab<ToroidalQuadRegion> quadRegions;

  int status;
  ToroidalHelper torHelper;
  ToroidalGatherCallback::RegionTab regions;
  shaders::UniqueOverrideStateId flipCullDepthOnlyOverride;

  shaders::UniqueOverrideStateId landColorOverride, grassMaskAndTypeOverride, grassMaskOnlyOverride;
};
