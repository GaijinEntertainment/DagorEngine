//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <3d/dag_textureIDHolder.h>
#include <3d/dag_resPtr.h>
#include <generic/dag_tab.h>
#include <math/dag_Point2.h>
#include <vecmath/dag_vecMathDecl.h>
#include <render/toroidalHelper.h>
#include <shaders/dag_postFxRenderer.h>
#include <shaders/dag_overrideStateId.h>
#include <generic/dag_relocatableFixedVector.h>
#include <EASTL/unique_ptr.h>
#include <util/dag_fixedBitArray.h>

class DynamicShaderHelper;

#define MAX_UPDATE_REGIONS 5


enum RenderDepthAOType : uint32_t
{
  Transparent = 1 << 0,
  Terrain = 1 << 1,
  Opaque = 1 << 2,
  ALL = 0xFFFFFFFF,
};

class IRenderDepthAOCB
{
public:
  enum class RenderRegionState
  {
    DontCare,
    FullyLoaded,
  };
  virtual RenderRegionState prepareRegionToRender(int region, bool is_refresh_tile) = 0;
  virtual void renderDepthAO(const Point3 &, mat44f_cref, const float, int region, RenderDepthAOType type, int cascade_no) = 0;
  virtual void getMinMaxZ(float &, float &) = 0;
};


class DepthAOAboveRenderer
{
  static constexpr int TEXEL_ALIGN = 4;
  static constexpr int THRESHOLD = TEXEL_ALIGN * 4;

  enum class RenderDepthAOClearFirst
  {
    Yes,
    No,
  };

  struct RegionToRender
  {
    RegionToRender(ToroidalQuadRegion &r) : cullViewProj(), reg(r) {}
    TMatrix4_vec4 cullViewProj;
    ToroidalQuadRegion reg;
    bool isRefreshTile = false;
  };

  class BlurDepthRenderer
  {

    int blurred_depthVarId = -1;
    eastl::unique_ptr<DynamicShaderHelper> blurDepth;
    UniqueBuf quadsBuffer;

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
    void render(BaseTexture *target, TEXTUREID depth_tid, ToroidalHelper &worldAODepthData, Tab<Vertex> &tris, int cascade_no);
  };

  BlurDepthRenderer blurDepthRenderer;

  PostFxRenderer copyRegionsRenderer;
  shaders::UniqueOverrideStateId zFuncAlwaysStateId;
  shaders::UniqueOverrideStateId zWriteOnStateId;

  struct CascadeDependantData
  {
    ToroidalHelper worldAODepthData;
    Tab<IBBox2> invalidAORegions;
    float depthAroundDistance = 0;
    Tab<RegionToRender> regionsToRender;

    static constexpr int REFRESH_TILE_DIM = 16;
    static constexpr int REFRESH_TILE_DIM_SQ = REFRESH_TILE_DIM * REFRESH_TILE_DIM;
    using TileMap = FixedBitArray<REFRESH_TILE_DIM * REFRESH_TILE_DIM>;
    TileMap refreshTileMap;
    int cycleId = 0;
  };
  // Note: the code assumes that MAX_CASCADES == 2 and cannot handle more for now
  // Need more? Move cascade dependant shadervars and etc to an array
  static constexpr int MAX_CASCADES = 2;
  float extraCascadeMult = 2.0f;
  dag::RelocatableFixedVector<CascadeDependantData, MAX_CASCADES, false> cascadeDependantData;

  const unsigned int texSize;
  const unsigned int refreshTileSize;
  UniqueTex debugRegionMap;
  UniqueTexHolder worldAODepth;
  UniqueTexHolder worldAODepthWithTransparency;
  UniqueTexHolder blurredDepth;
  UniqueTexHolder blurredDepthWithTransparency;
  Point2 sceneMinMaxZ;

  bool renderTransparent = false;

  void renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb, int cascade_no);
  void renderAODepthQuads(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb, BaseTexture *depthTex,
    RenderDepthAOType type, RenderDepthAOClearFirst clear_mode, int cascade_no);
  void renderAODepthQuadsTransparent(dag::ConstSpan<RegionToRender> regions, IRenderDepthAOCB &renderDepthCb,
    RenderDepthAOClearFirst clear_mode, int cascade_no);
  void copyDepthAboveRegions(dag::ConstSpan<RegionToRender> regions, BaseTexture *depthTex, int cascade_no);
  void updateInvalidRegionsToRender(CascadeDependantData &cascadeData, float splitThreshold);

  static void fill_tile_map(CascadeDependantData::TileMap &region_map, IBBox2 raw_region, bool set_value);
  static IBBox2 get_refresh_tile_region(int cycle_id, int tile_size);
  void fillRefreshTiles(int cascade_no, IBBox2 region, bool set_value);
  void updateRefreshTilesToRender(CascadeDependantData &cascadeData);
  void refreshDebugRegionMap(int cascade_no);

public:
  DepthAOAboveRenderer(int texSize, float depth_around_distance, bool render_extra = false, bool use_extra_cascade = false,
    float extra_cascade_mult = 2.0f);
  ~DepthAOAboveRenderer() { setInvalidVars(); }

  void prepareRenderRegions(const Point3 &origin, float scene_min_z, float scene_max_z, float splitThreshold = -1.0f);
  void prepareRenderRegionsForCascade(const Point3 &origin, float scene_min_z, float scene_max_z, float splitThreshold = -1.0f,
    int cascade_no = 0);
  void renderPreparedRegions(IRenderDepthAOCB &renderDepthCb);
  void renderPreparedRegionsForCascade(IRenderDepthAOCB &renderDepthCb, int cascade_no = 0);
  dag::ConstSpan<RegionToRender> getRegionsToRender(int cascade_no = 0) const
  {
    G_ASSERT(cascade_no < cascadeDependantData.size());
    return cascadeDependantData[cascade_no].regionsToRender;
  }
  void invalidateAO(bool force);
  void invalidateAO(const BBox3 &box);
  void setDistanceAround(float distance_around);
  float getDistanceAround(int cascade_no = 0) const
  {
    G_ASSERT(cascade_no < cascadeDependantData.size());
    return cascadeDependantData[cascade_no].depthAroundDistance;
  }
  float getTexelSize(int cascade_no = 0) const;
  void setVars();
  void setInvalidVars();
  bool isValid() const;
};
