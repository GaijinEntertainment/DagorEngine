//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_tab.h>
#include <math/dag_Point2.h>
#include <vecmath/dag_vecMathDecl.h>
#include <render/toroidalHelper.h>
#include <shaders/dag_postFxRenderer.h>

class DynamicShaderHelper;

#define MAX_UPDATE_REGIONS 5

class IRenderDepthAOCB
{
public:
  virtual void renderDepthAO(const Point3 &, mat44f_cref, const float, int region, bool transparent = false) = 0;
  virtual void getMinMaxZ(float &, float &) = 0;
};

class DepthAOAboveRenderer
{
  struct RegionToRender
  {
    RegionToRender(ToroidalQuadRegion &r) : cullViewProj(), reg(r) {}
    TMatrix4_vec4 cullViewProj;
    ToroidalQuadRegion reg;
  };

  class BlurDepthRenderer
  {

    int blurred_depthVarId = -1;
    eastl::unique_ptr<DynamicShaderHelper> blurDepth;

  public:
    struct Vertex
    {
      Point2 pos;
      Point4 clampTc;
      Vertex() = default;
      Vertex(const Point2 &p, const Point4 &tc) : pos(p), clampTc(tc) {}
    };
    BlurDepthRenderer();
    ~BlurDepthRenderer();
    void render(BaseTexture *target, TEXTUREID depth_tid, ToroidalHelper &worldAODepthData, Tab<Vertex> &tris);
  };

  BlurDepthRenderer blurDepthRenderer;

  PostFxRenderer copyRegionsRenderer;
  shaders::UniqueOverrideStateId zFuncAlwaysStateId;
  shaders::UniqueOverrideStateId zWriteOnStateId;

  UniqueTexHolder worldAODepth;
  UniqueTexHolder worldAODepthWithTransparency;
  ToroidalHelper worldAODepthData;
  UniqueTexHolder blurredDepth;
  UniqueTexHolder blurredDepthWithTransparency;
  Point2 sceneMinMaxZ;
  ;
  Tab<IBBox2> invalidAORegions;
  float depthAroundDistance = 0;
  Tab<RegionToRender> regionsToRender;

  bool renderTransparent = false;
  void renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb, BaseTexture *depthTex,
    bool transparent);
  void renderAODepthQuadsTransparent(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb);
  void copyDepthAboveRegions(dag::ConstSpan<RegionToRender> regions);

public:
  DepthAOAboveRenderer(int texSize, float depth_around_distance, bool render_extra = false);
  ~DepthAOAboveRenderer() { setInvalidVars(); }

  void prepareAO(const Point3 &origin, IRenderDepthAOCB &renderDepthCb); // prepare and render
  void prepareRenderRegions(const Point3 &origin, float scene_min_z, float scene_max_z, float splitThreshold = -1.0f);
  void renderPreparedRegions(IRenderDepthAOCB &renderDepthCb);
  void renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb);
  dag::ConstSpan<RegionToRender> getRegionsToRender() const { return regionsToRender; }
  void invalidateAO(bool force);
  void invalidateAO(const BBox3 &box);
  void setDistanceAround(float distance_around);
  float getDistanceAround() const { return depthAroundDistance; }
  void setVars();
  void setInvalidVars();
};
