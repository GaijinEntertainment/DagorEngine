//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

class Point2;

#include <render/staticShadowsCB.h>

#include <3d/dag_resPtr.h>
#include <math/dag_Point3.h>
#include <math/dag_bounds3.h>
#include <math/dag_TMatrix.h>
#include <math/integer/dag_IBBox2.h>
#include <render/toroidalHelper.h>
#include <math/dag_TMatrix4.h>
#include <render/viewTransformData.h>
#include <generic/dag_tab.h>

#include <math/dag_TMatrix.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/vector.h>

struct ShadowDepthScroller;

class ToroidalStaticShadowCascade
{
public:
  struct BeforeRenderReturned
  {
    uint32_t pixelsRendered;
    bool varsChanged;
    bool scrolled;
  };

protected:
  struct FrameData
  {
    float maxHtRange = 100;
    float minHt = 0;
    float maxHt = 100;
    double zn = 0;
    double zf = 1;
    Point3 sunDir = {0, 1, 0};
    TMatrix lightViewProj; // identical for all cascades if not in transition
    Point2 torOfs = {0, 0};
    Point2 alignedOrigin = {0, 0};
    bool isOrtho = false;
    bool operator!=(const FrameData &a) const
    {
      return fabs(zf - a.zf) > 1e-6 || fabs(zn - a.zn) > 1e-6 || isOrtho != a.isOrtho || sunDir != a.sunDir ||
             fabsf(minHt - a.minHt) > 1e-5 || fabsf(maxHt - a.maxHt) > 1e-5;
    }
  } current, next;

  // invalidate everything (including origin)
  void invalidate(bool force);

  // invalidateBox method splits added box, so if pixel is already in list do not invalidate twice
  void invalidateBox(const BBox3 &invalidBox);
  void invalidateBoxes(const BBox3 *invalidBox, uint32_t cnt);
  void init(int cascade_id, int texSize, float distance, float maxHtRange);
  void setShadervarsToInvalid();

  bool setSunDir(const TMatrix &lightViewProj, const Point3 &dir_to_sun, bool isOrtho, float cos_force_threshold,
    float cos_lazy_threshold); // true if will require re-render
  BeforeRenderReturned updateOrigin(const ManagedTex &tex, ShadowDepthScroller *s, const Point3 &origin,
    float move_texels_threshold_part, float move_z_threshold, float max_update_texels_part, bool uniformUpdate);
  void render(IStaticShadowsCB &cb);
  void close();
  void setShaderVars();
  void enableTransitionCascade(bool en);

  float getDistance() const { return distance; }
  void setMaxHtRange(float maxHtRange); //
  float getTexelSize() const { return texelSize; }
  uint32_t getTexSize() const { return helper.texSize; }
  // Maximum 4 regions (in case the bbox is placed on a corner of the toroidal texture).
  void getBoxViews(const BBox3 &box, ViewTransformData *out_views, int &out_cnt) const;
  void setWorldBox(const BBox3 &box);
  void setDistance(float distance);
  void beginFrameTransition();
  bool endFrameTransition(const ManagedTex &tex);
  //! returns true, if box covered by static shadows
  bool isInside(const BBox3 &box) const;
  //! returns true, if box has no never rendered (empty) or invalid (invalidated) regions
  bool isCompletelyValid(const BBox3 &box) const;
  //! returns true, if box has no never rendered (empty) regions
  bool isValid(const BBox3 &box) const;
  void updateSunBelowHorizon(const ManagedTex &tex);
  bool isSunBelowHorizon() const { return next.sunDir.y < 0.0f; }
  float constDepthBias = 0.f;
  int cascade_id = 0;
  BBox3 worldBox;
  Tab<IBBox2> invalidRegions;
  Tab<IBBox2> notRenderedRegions;
  ToroidalHelper helper;
  float distance = 0, configDistance = 0;
  float texelSize = 1;
  int static_shadow_matrixVarId[4] = {-1, -1, -1, -1};
  int static_shadow_cascade_scale_ofs_z_torVarId = -1;
  int framesSinceLastUpdate = 100000;

  bool optimizeBigQuadsRender = true;
  bool stopRenderingAfterRegionSplit = false;

  FrameData backFrame;     // frame to read from while in transition
  UniqueTex transitionTex; // texture to write to while in transition
  bool inTransition = false;

  inline const FrameData &getReadFrameData() const { return inTransition ? backFrame : current; }
  void calcScaleOfs(const FrameData &fr, float &xy_scale, Point2 &xy_ofs, float &z_scale, float &z_ofs) const;
  void calcScaleOfs(const FrameData &fr, float &xy_scale, Point2 &xy_ofs, float &z_scale, float &z_ofs, const Point2 &origin,
    float bias) const;
  TMatrix calcToCSPMatrix(const FrameData &fr, float bias, const Point2 &origin) const;
  Point3 clipPointToWorld(const Point3 &pos) const;
  IBBox2 getClippedBoxRegion(const BBox3 &box) const;
  ToroidalQuadRegion getToroidalQuad(const IBBox2 &region) const;
  TMatrix getLightDirOrthoTm() const;
  TMatrix4 getLightProjTm(const TMatrix &light_dir_ortho_tm, const BBox2 &region_world_box, Point2 zn_zf) const;
  TMatrix4 getCullProjTm(const BBox2 &region_world_box, Point2 zn_zf) const;
  // Point2 getSunOffsetXZ() const;
  bool changeDepth(const ManagedTex &tex, ShadowDepthScroller *s, float scale, float ofs);

  Point3 toTexelSpace(const Point3 &p0) const;
  BBox3 toTexelSpace(const BBox3 &p0) const;
  BBox3 toTexelSpaceClipped(const BBox3 &p0) const;
  friend class ToroidalStaticShadows;
  void calcNext(const Point3 &p0);
  IPoint2 getTexelOrigin(const Point3 &) const;


  void setRenderParams(BaseTexture *target);
  void discardRegion(const IPoint2 &leftTop, const IPoint2 &widthHeight);
  void renderRegions(dag::ConstSpan<ToroidalQuadRegion> regionsToRender);

  TMatrix getLightViewProj() const;

  struct RenderData
  {
    BaseTexture *target;
    TMatrix viewTm;
    TMatrix viewItm;
    TMatrix wtm;
    Point2 znzf;
    float minHt;
    float maxHt;
    bool isSunBelowHorizon;

    struct RegionToDiscard
    {
      IPoint2 lt;
      IPoint2 wh;
    };
    eastl::vector<RegionToDiscard> regionsToDiscard;

    struct RegionToRender
    {
      TMatrix4 projMatrix;
      TMatrix4_vec4 cullViewProj;
      ViewTransformData transform;
    };
    eastl::vector<RegionToRender> regionsToRender;
  } renderData{};

  int getRegionToRenderCount() const;
  void clearRegionToRender();
  TMatrix4 getRegionToRenderCullTm(int region) const;

  bool copyTransitionAfterRender = false;
  BaseTexture *transitionCopyTarget = nullptr;
};

class ToroidalStaticShadows
{
public:
  // invalidate everything (including origin)
  void invalidate(bool force);

  // invalidateBox method splits added box, so if pixel is already in list do not invalidate twice
  void invalidateBox(const BBox3 &invalidBox);
  void invalidateBoxes(const BBox3 *invalidBox, uint32_t cnt);
  ToroidalStaticShadows(int texSize, int cascades, float maxDistance, float maxHtRange, bool isArray);
  ~ToroidalStaticShadows();

  void enableTransitionCascade(int cascade_id, bool en);
  void enableOptimizeBigQuadsRender(int cascade_id, bool en);
  void enableStopRenderingAfterRegionSplit(int cascade_id, bool en);

  // true if will require re-render
  bool setSunDir(const Point3 &dir_to_sun, float cos_force_threshold = 0.99f, float cos_lazy_threshold = 0.9999f);

  ToroidalStaticShadowCascade::BeforeRenderReturned updateOriginAndRender(const Point3 &origin, float move_texels_threshold_part,
    float move_z_threshold, float max_update_texels_part, bool uniform_update, IStaticShadowsCB &cb,
    bool update_only_last_cascade = false);

  ToroidalStaticShadowCascade::BeforeRenderReturned updateOrigin(const Point3 &origin, float move_texels_threshold_part,
    float move_z_threshold, bool uniform_update, float max_update_texels_part, bool update_only_last_cascade = false);
  int getRegionToRenderCount(int cascade) const;
  void clearRegionToRender(int cascade); // not should be called under normal circumstances
  TMatrix4 getRegionToRenderCullTm(int cascade, int region) const;
  void render(IStaticShadowsCB &cb);

  void setShaderVars();
  int cascadesCount() const { return cascades.size(); }

  float getMaxWorldDistance() const;
  float calculateCascadeDistance(int cascade, int cascade_count, float max_distance) const;

  float getDistance() const { return maxDistance; }
  const ManagedTexHolder &getTex() { return staticShadowTex; }
  void restoreShadowSampler();
  void setMaxHtRange(float max_ht_range); // that is only for skewed matrix
  float getSmallestTexelSize() const { return cascades.empty() ? -1 : cascades[0].texelSize; }
  float getBiggestTexelSize() const { return cascades.empty() ? -1 : cascades.back().texelSize; }
  uint32_t getTexSize() const { return cascades.empty() ? 0 : texSize; }
  // Maximum 4 regions (in case the bbox is placed on a corner of the toroidal texture).
  int getBoxViews(const BBox3 &box, ViewTransformData *out_views, int &out_cnt) const;
  bool getCascadeWorldBoxViews(int cascade_id, ViewTransformData *out_views, int &out_cnt) const;
  void setWorldBox(const BBox3 &box); // causes unforced invalidation, if box is different
  void setDistance(float distance);   // causes forced invalidation, if is different
  void setCascadeDistance(int cascade, float distance);
  bool isInside(const BBox3 &box) const;          // returns true, if box covered by static shadows
  bool isCompletelyValid(const BBox3 &box) const; // returns true, if box has no never rendered (empty) or invalid (invalidated)
                                                  // regions
  bool isValid(const BBox3 &box) const;           // returns true, if box has no never rendered (empty) regions
  inline bool isOrthoShadow() const { return isOrtho; }
  bool setDepthBias(float f);
  bool setDepthBiasForCascade(int cascade, float f);
  void enableTextureSpaceAlignment(bool en);
  void clearTexture();
  bool isAnyCascadeInTransition() const;

  TMatrix getLightViewProj(const int cascadeId) const;

  // isSunBelowHorizon: To allow things behind the far plane to be in shadow, when the texture is empty.
  //   If it was cleared to 1, z>1 would be clamped to 1, thus SampleCmp always returns 1.
  static float GetShadowClearValue(bool isSunBelowHorizon) { return isSunBelowHorizon ? float(0xFFFE) / float(0xFFFF) : 1.0; }

  static constexpr float ORTHOGONAL_THRESHOLD = 0.1; // auto switch to ortho shadows

protected:
  bool isOrtho = false;
  bool isRendered = false;
  float maxDistance = 0; // biggest cascade distance
  float maxHtRange = 512.f;
  UniqueTexHolder staticShadowTex;
  int texSize;
  BBox3 worldBox;
  bool useTextureSpaceAlignment = false;
  TMatrix lightViewProj = TMatrix::IDENT;
  eastl::vector<ToroidalStaticShadowCascade> cascades;
  eastl::unique_ptr<ShadowDepthScroller> depthScroller; // we only need depthScroller, if not orthogonal or maxHtRange is bigger than
                                                        // worldBox (or worldBox is unknown)
};
