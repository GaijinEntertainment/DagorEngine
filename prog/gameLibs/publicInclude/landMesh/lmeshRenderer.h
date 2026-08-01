//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_smallTab.h>
#include <generic/dag_tab.h>
#include <util/dag_stdint.h>
#include <math/dag_Point2.h>
#include <math/dag_Point3.h>
#include <math/dag_Point4.h>
#include <vecmath/dag_vecMathDecl.h>
#include <math/dag_color.h>
#include <math/dag_bounds3.h>
#include <math/integer/dag_IBBox2.h>
#include <math/integer/dag_IPoint2.h>
#include <3d/dag_texMgr.h>
#include <util/dag_simpleString.h>
#include <ioSys/dag_dataBlock.h>
#include <generic/dag_carray.h>
#include <landMesh/lmeshTools.h>
#include <landMesh/landClass.h>
#include <landMesh/lmeshRenderFlags.h>
#include <landMesh/lmeshCulling.h>
#include <shaders/dag_overrideStateId.h>
#include <EASTL/vector_map.h>
#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <drv/3d/dag_decl.h>
#include <physMap/physMap.h>

enum LandClassData
{
  LC_TRIVIAL_DATA = 1,
  LC_GRASS_DATA = 2,
  LC_DETAIL_DATA = 4,
  LC_NORMAL_DATA = 8,
  LC_SWAP_VERTICAL_DETAIL = 16,
  LC_ALL_DATA = 0xFFFFFFFF
};
namespace landmesh
{
struct OptimizedScene;
}
struct LandClassDetailTextures;

// BaseTexture / Texture / ArrayTexture typedefs and NUM_TEXTURES_STACK live in
// landMesh/lmeshTools.h (included above), so the land-class / virtual-texture code
// can use them without depending on this header.
struct LCTexturesLoaded;

struct CellState;
struct Frustum;
struct LandMeshCellDesc;
class LandMeshManager;
class LandVtexRenderer;
struct CellState;

class CellRegionCallback
{
public:
  virtual void startRenderCellRegion(int region) = 0;
  virtual void endRenderCellRegion(int region) = 0;
  virtual const IBBox2 *regions() const = 0;
  virtual int regionsCount() const = 0;
};

class EmptyRegionCallback : public CellRegionCallback
{
public:
  int count;
  EmptyRegionCallback() : count(1) {}
  EmptyRegionCallback(int c) : count(c) {}
  virtual void startRenderCellRegion(int) {}
  virtual void endRenderCellRegion(int /*region*/) {}
  virtual const IBBox2 *regions() const { return NULL; }

  virtual int regionsCount() const { return count; }
};

enum class LMeshRenderingMode : int
{
  RENDERING_WITH_SPLATTING = -1,
  RENDERING_LANDMESH = 0,
  RENDERING_CLIPMAP = 1,
  OBSOLETE_RENDERING_SPOT = 2,
  GRASS_COLOR = 3,
  GRASS_MASK = 4,
  OBSOLETE_SPOT_TO_GRASS_MASK = 5,
  RENDERING_HEIGHTMAP = 6,
  RENDERING_DEPTH = 7,
  RENDERING_VSM = 8,
  RENDERING_REFLECTION = 9,
  RENDERING_FEEDBACK = 10,
  LMESH_MAX
};

namespace landmesh
{
// Sets/reads the lmesh_rendering_mode shadervar (the shader-variant selector for the landmesh/heightmap
// shader family) without a renderer, for passes that draw those shaders outside LandMeshRenderer
// (decals3d, tools heightmap-only renders). The mode is process-global, shared with
// LandMeshRenderer::setLMeshRenderingMode; the setter returns the previous mode for save/restore.
LMeshRenderingMode set_rendering_mode_shadervar(LMeshRenderingMode mode);
LMeshRenderingMode get_rendering_mode_shadervar();
} // namespace landmesh

// How to DRAW a culled batch: the per-pass state that historically lived in renderer members set
// by setters between passes (and silently inherited by the next pass). Callers build one per pass;
// entries taking a desc leave no sticky state behind, except the mode member/shadervar sync that
// ambient-mode consumers still rely on (see the desc renderCulled entry).
struct LandMeshRenderDesc
{
  LMeshRenderingMode mode = LMeshRenderingMode::RENDERING_LANDMESH;
  uint32_t renderFlags = 0xFFFFFFFF; // landmesh::RENDER_* buckets; default = everything
  BBox3 renderInBBox;                // decal sub-cell clip; must match the cull's; empty = whole map
  float invGeomLodDist = -1;         // 1/distance to each next geom lod; <0 = the renderer's default, 0 forces LOD0
  CellRegionCallback *regionCb = nullptr;
  bool debugCells = false;
  bool decalsCompatibilityMode = false; // decal entries: skip the per-detail texture consts
  bool decalsSamplersNoMipBias = false; // decal entries: force the no-mip-bias detail samplers (implied in clipmap mode)
};

class LandMeshRenderer
{
public:
  // Render-control flags relocated to the neutral header landMesh/lmeshRenderFlags.h
  // so virtual-texture code does not depend on LandMeshRenderer. These are compat
  // aliases; lmesh_render_flags references the single landmesh:: storage.
  enum
  {
    RENDER_DECALS = landmesh::RENDER_DECALS,
    RENDER_COMBINED = landmesh::RENDER_COMBINED,
    RENDER_LANDMESH = landmesh::RENDER_LANDMESH,
    RENDER_HEIGHTMAP = landmesh::RENDER_HEIGHTMAP
  };
  static uint32_t &lmesh_render_flags;
  enum RenderPurpose
  {
    DEFAULT_RENDERING_PURPOSE,
    RENDER_FOR_GRASS
  };

  enum RenderType
  {
    RENDER_WITH_SPLATTING = 0,
    RENDER_CLIPMAP = 1,
    RENDER_GRASS_MASK = 2,
    MAX_RENDER_SPLATTING__ = RENDER_GRASS_MASK,
    RENDER_WITH_CLIPMAP = 3,
    RENDER_REFLECTION = 4,
    // RENDER_COMBINED_LAST  = 4,
    // MAX_WITH_CLIPMAP__ = RENDER_COMBINED_LAST,
    MAX_WITH_CLIPMAP__ = RENDER_REFLECTION,
    RENDER_DEPTH = 5,
    RENDER_ONE_SHADER = 6, // all shaders are the same! feedback, heightmap, vsm. render only landmesh and landmesh_combined
    RENDER_PATCHES = 7,
    RENDER_TYPES_COUNT
  };
  static constexpr int DET_TEX_NUM = ::DET_TEX_NUM; // single definition in landMesh/lmeshTools.h
  static constexpr int VER_LABEL = (DET_TEX_NUM << 8) | 2;


public:
  PhysMap *physMap = nullptr;
  LandMeshRenderer(LandMeshManager &provider);
  ~LandMeshRenderer();

  bool checkVerLabel() { return verLabel == VER_LABEL; }

  void resetOptSceneAndStates();
  // Heightmap per-frame upkeep (texture uploads, pending terrain edits) is not done here; call
  // HeightmapHandler::makeBookKeeping() once per frame before render() or the terrain renders stale.
  void prepare(LandMeshManager &provider, const HmapOrigin &origin);
  bool isPrepared() const { return prepared; }
  // void set_land_classes(dag::ConstSpan<SimpleString> land_classes);

  void resetTextures();
  void setCustomLcTextures();

  void setRenderInBBox(const BBox3 &bbox) { renderInBBox = bbox; }
  // Compat accessor for wrappers that render under an ambient box set by another module (the
  // editor grass service); goes away with the setter.
  const BBox3 &getRenderInBBox() const { return renderInBBox; }

  CellRegionCallback *setRegionCB(CellRegionCallback *cb)
  {
    CellRegionCallback *o = regionCallback;
    regionCallback = cb;
    return o;
  }

  void setMirroring(LandMeshManager &provider, int num_border_cells_x_pos, int num_border_cells_x_neg, int num_border_cells_z_pos,
    int num_border_cells_z_neg);

  void setCellsDebug(int dbg) { debugCells = dbg; }
  bool getCellsDebug() const { return debugCells; }

#if _TARGET_PC
  static void afterLostDevice();
#endif
#if DAGOR_DBGLEVEL > 0
  static bool check_cull_matrix(const TMatrix &realView, const TMatrix4 &realProj, const Driver3dPerspective &persp,
    const TMatrix4 &realGlobtm, const char *marker, const LandMeshCullingData &data, bool do_fatal);
#endif

  // Save/restore accessors for the process-global ambient mode; the lmeshRenderingMode member is
  // only read by legacyRenderDesc() to fill desc.mode for the desc-less compat entries.
  LMeshRenderingMode setLMeshRenderingMode(LMeshRenderingMode mode);
  LMeshRenderingMode getLMeshRenderingMode() const { return landmesh::get_rendering_mode_shadervar(); }

  void renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData, const TMatrix *realView,
    const TMatrix4 *realProj, const Driver3dPerspective *persp, const TMatrix4 *realGlobtm, const Point3 &view_pos,
    bool check_matrices = true, RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);
  void renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData, const Point3 &view_pos,
    RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);
  // Pass-descriptor entries: draw per desc; their one sticky side effect is setLMeshRenderingMode
  // (mode member + shadervar), which ambient-mode consumers rely on until the setters go.
  void renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc, const LandMeshCullingData &culledData,
    const Point3 &view_pos, RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE, const Frustum *cull_frustum = nullptr);
  void renderCulled(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc, const LandMeshCullingData &culledData,
    const TMatrix *realView, const TMatrix4 *realProj, const Driver3dPerspective *persp, const TMatrix4 *realGlobtm,
    const Point3 &view_pos, bool check_matrices, RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE,
    const Frustum *cull_frustum = nullptr);

  // hmap_origin is the world position the heightmap LOD stays centered on: the origin the
  // HeightmapHandler was prepare()'d with for the tessellation this pass draws. It usually equals
  // view_pos (terrain LOD follows the rendered view); a pass that intentionally keeps terrain LOD on
  // another view (a mirror, or an ortho bake centered on the main camera or bake box) passes that instead.
  // The heightmap draw decision is made here per call (never inherited from a prior prepare()): ortho
  // one-shader top-down projections draw a single quad, other ortho bakes always include the tessellated
  // heightmap (mayRenderHmap only gates perspective views), and perspective views honor the
  // shouldRenderTessellatedHmap distance gate at hmap_origin.
  void render(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider, RenderType rtype,
    const Point3 &view_pos, HmapOrigin hmap_origin, RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);
  void render(LandMeshManager &provider, RenderType rtype, const Point3 &view_pos, HmapOrigin hmap_origin,
    RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);
  // Pass-descriptor variants of the sync cull+render entries above. They draw through the desc
  // renderCulled entry and share its sticky setLMeshRenderingMode side effect.
  void render(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider, RenderType rtype,
    const LandMeshRenderDesc &desc, const Point3 &view_pos, HmapOrigin hmap_origin,
    RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);
  void render(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc, const Point3 &view_pos,
    HmapOrigin hmap_origin, RenderPurpose rpurpose = DEFAULT_RENDERING_PURPOSE);

  bool renderDecals(LandMeshManager &provider, RenderType rtype, const TMatrix4 &globtm, HmapOrigin hmap_origin,
    bool compatibility_mode, bool use_samplers_no_mipbias);
  bool renderCulledDecals(LandMeshManager &provider, const LandMeshCullingData &culledData, bool compatibility_mode,
    bool use_samplers_no_mipbias);
  // Pass-descriptor variants of the decal entries above; the decal options come from the desc
  // (decalsCompatibilityMode, decalsSamplersNoMipBias). They share the desc renderCulled entry's
  // sticky setLMeshRenderingMode side effect.
  bool renderDecals(LandMeshManager &provider, RenderType rtype, const LandMeshRenderDesc &desc, const TMatrix4 &globtm,
    HmapOrigin hmap_origin);
  bool renderCulledDecals(LandMeshManager &provider, const LandMeshRenderDesc &desc, const LandMeshCullingData &culledData);

  static int lod1_switch_radius;
  ///!sets inverse (1/distance) distance to next geom LOD
  void setInvGeomLodDist(float invGeomLodDistance) { invGeomLodDist = invGeomLodDistance; }
  float getInvGeomLodDist() const { return invGeomLodDist; }
  void evictSplattingData();
  TEXTUREID getDetailTileTex();
  d3d::SamplerHandle getDetailTileSmp() const;
  bool reloadGrassMaskTex(int land_class_id, TEXTUREID newGrassMaskTexId);

  Tab<LandClassDetailTextures> &getLandClasses();
  void updateCustomSamplers(LandMeshManager &provider);

protected:
  // Per-pass scratch: built on the stack at a render entry point and threaded through the
  // cell-rendering helpers. Replaces file/class statics that leaked state from one pass into
  // the next (and made two renderers or views in flight undefined behavior).
  struct RenderPassCtx
  {
    LandMeshRenderDesc desc; // how this pass draws; compat entries fill it from the setter members
    uint32_t mirrorMask = 0; // last mirror-scale ps const set this pass; 0 bit - x, 1 bit - z
    bool cullFlipped = false;
    shaders::OverrideStateId cullFlippedPrevStateId, cullFlippedCurStateId;
    bool skipBottomRendering = false;
    bool useExclBox = false; // this pass's cull decision (LandMeshCullingData::useExclBox)
    // This pass's cull frustum bbox (LandMeshCullingData::frustumWorldBBox); decal elements
    // clip to it when the desc has no renderInBBox.
    bbox3f frustumWorldBBox = {};
  };

  // The pass desc a legacy (setter-based) entry draws with, assembled from the setter members.
  LandMeshRenderDesc legacyRenderDesc() const;

  __forceinline ShaderMesh *prepareGeomCellsLM(LandMeshManager &provider, int cellNo, int lodNo);
  __forceinline ShaderMesh *prepareGeomCellsCM(LandMeshManager &provider, int cellNo, bool **isBig);
  void renderGeomCellsLM(LandMeshManager &provider, const RenderPassCtx &pass_ctx, dag::ConstSpan<uint16_t> cells, int lodNo,
    RenderType rtype, uint8_t use_exclusion, bool force_set_states);
  void renderGeomCellsCM(LandMeshManager &provider, dag::ConstSpan<uint16_t> cells, RenderType rtype, bool skip_not_big,
    bool force_set_states);

  struct MirroredCellState;
  void renderCulledImpl(LandMeshManager &provider, RenderType rtype, const LandMeshCullingData &culledData, const TMatrix *realView,
    const TMatrix4 *realProj, const Driver3dPerspective *persp, const TMatrix4 *realGlobtm, const Point3 &view_pos,
    bool check_matrices, RenderPurpose rpurpose, RenderPassCtx &pass_ctx, const Frustum *cull_frustum = nullptr);
  void renderImpl(mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, LandMeshManager &provider, RenderType rtype,
    const LandMeshRenderDesc &desc, const Point3 &view_pos, HmapOrigin hmap_origin, RenderPurpose rpurpose);
  bool renderCellDecals(LandMeshManager &provider, const RenderPassCtx &pass_ctx, const MirroredCellState &mirroredCell,
    bool force_samplers_no_mipbias = false);
  void renderCell(LandMeshManager &provider, RenderPassCtx &pass_ctx, int borderX, int borderY, RenderType rtype,
    RenderPurpose rpurpose, bool skip_combined_not_marked_as_big);

  void getCellState(LandMeshManager &provider, int cellX, int cellY, struct CellState &cell);

  int verLabel;
  IPoint2 centerCell;
  Point2 centerCellFract; // centerPos-centerCellFract*cellSize
  float invGeomLodDist;   //<Distance to each next geom lod
  bool debugCells;
  LMeshRenderingMode lmeshRenderingMode = LMeshRenderingMode::RENDERING_LANDMESH;
  CellState *cellStates;
  BBox3 renderInBBox;
  // Land-class / virtual-texture data and methods relocated into LandVtexRenderer.
  // Owned by LandMeshManager; this is a non-owning back-pointer.
  LandVtexRenderer *vtex = nullptr;

  struct MirroredCellState
  {
    Point4 posToWorldSet[2];
    Color4 detMapTcSet;
    struct MirrorScaleState
    {
      uint8_t xz; // x+z*3
      // MirrorScaleState(uint32_t xz_):xz(xz_){}
      MirrorScaleState() : xz(0) {}
    };
    uint8_t invcull : 1;  // x logical xor z
    uint8_t excluded : 1; // x logical xor z
    uint8_t cellX, cellY; // from 0 to provider.getNumCellsX(), provider.getNumCellsY()

    MirrorScaleState mirrorScaleState;

    bool getInvCull() const { return invcull; }

    void init(int borderX, int borderY, int mirrorX, int mirrorY, int x1, int y1, float cellSize, float gridCellSize,
      Point4 posToWorld[2], Color4 detMapTc, bool to_be_excluded);

    void setPosConsts(bool render_at_0 = false) const;
    void setDetMapTc() const;
    void setPsMirror(RenderPassCtx &pass_ctx) const;
    bool setFlipCull(LandMeshRenderer *renderer, RenderPassCtx &pass_ctx) const;
    static void startRender(RenderPassCtx &pass_ctx);
  };
  SmallTab<MirroredCellState, MidmemAlloc> mirroredCells;
  int tWidth; // int tWidth = provider.getNumCellsX()+numBorderCellsXNeg+numBorderCellsXPos;


  int numBorderCellsXPos;
  int numBorderCellsXNeg;
  int numBorderCellsZPos;
  int numBorderCellsZNeg;
  CellRegionCallback *regionCallback;
  Point4 worldMulPos[9][2]; // worldMulPos for all mirroring
  bool prepared = false;
  landmesh::OptimizedScene *optScn;

  enum StateDepthBias
  {
    STATE_DEPTH_BIAS_ZERO,
    STATE_DEPTH_BIAS_BOTTOM
  };

  shaders::OverrideStateId setOverride(const shaders::OverrideState &new_state);
  void resetOverride(shaders::OverrideStateId &prev_state);
  shaders::OverrideStateId setStateFlipCull(bool flip_cull);
  shaders::OverrideStateId setStateDepthBias(StateDepthBias depth_bias);

  eastl::vector_map<uint32_t, eastl::vector<shaders::UniqueOverrideStateId>> overrideStateMap;
};
