//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

// #include <stdlib.h>//for abs


#include <heightmap/heightmapCulling.h>
#include <math/integer/dag_IPoint2.h>
#include <math/integer/dag_IBBox2.h>
#include <math/dag_bounds3.h>
#if DAGOR_DBGLEVEL > 0
#include <math/dag_TMatrix4.h>
#endif

class LandMeshManager;


#define LANDMESH_MAX_CELLS    (64 * 3 * 4 + 4 * 4 + 1) //(maxRadius*(maxmirroring=3)*(max_sides=4)+max_sides*MAX_LAND_MESH_REGIONS+1
#define LANDMESH_INVALID_CELL LANDMESH_MAX_CELLS

struct LandMeshCellDesc
{
  uint16_t next;
  int8_t borderX;
  int8_t borderY;
  uint8_t countX;
  uint8_t countY;
};

static constexpr int MAX_LAND_MESH_REGIONS = 4;

// How the terrain heightmap is drawn for a culled batch. Travels with the cull result
// (LandMeshCullingData) instead of the old persistent LandMeshRenderer::renderHeightmapType
// member, so the decision is per-cull and not inherited from the last prepare().
enum class HmapDrawType : uint8_t
{
  NONE,       // no heightmap (only land mesh cells)
  TESSELATED, // full LOD-gridded heightmap (LandMeshCullingData::heightmapData)
  ONEQUAD,    // single quad patch (HeightmapHandler::renderOnePatch); ortho one-shader passes only
};

// Strong wrapper for the heightmap-LOD origin. render()/renderDecals()/cull descs take the view
// position (a raw Point3) and this origin as separate arguments; wrapping the origin makes the two
// impossible to swap at a call site (an accidental swap would silently center terrain tessellation
// on the wrong view).
struct HmapOrigin
{
  Point3 pos;
  // Scales the tessellated-heightmap switch distance (shouldRenderTessellatedHmap); per-level
  // caller config. Perspective passes must pass the same value LandMeshRenderer::prepare() got,
  // or the cell exclusion decided there and the pass's heightmap draw decision disagree (holes).
  float distanceMul = 1.0f;
  explicit HmapOrigin(const Point3 &p, float distance_mul = 1.0f) : pos(p), distanceMul(distance_mul) {}
};

struct LandMeshCullingData
{
  LandMeshCullingData(IMemAlloc *mem = midmem) : heightmapData(mem) {}
  LandMeshCellDesc cells[LANDMESH_MAX_CELLS];
  int count = 0;
  struct Region
  {
    short head, tail;
  } regions[MAX_LAND_MESH_REGIONS];
  int regionsCount;
  HmapDrawType hmapDrawType = HmapDrawType::NONE;
  // Render-side hide decision (skip land cells under the tessellated heightmap), read by the pass
  // instead of the shared manager state. Usually the cull desc's bit echoed back; clipmap-like
  // render() culls without exclusion and overrides this from the manager's persistent bit.
  bool useExclBox = false;
  // Cull traversal center (from LandMeshCullDesc::viewPos) in cells + position within the cell.
  // renderCulled() measures geometry LOD from it, so cull and render agree on the center for
  // every batch, whether or not the renderer was prepare()'d for this view.
  IPoint2 centerCell = IPoint2(0, 0);
  Point2 centerCellFract = Point2(0, 0);
  // Cull frustum's world bbox. The pass clips decal elements to it when it renders without a
  // renderInBBox, so decal filtering always matches the cull that produced this data.
  bbox3f frustumWorldBBox = {};
  // Set by frustum_cull. The render entries reject data with cells that was not produced by a
  // cull: centerCell/hmapDrawType/frustumWorldBBox above are required cull outputs.
  bool fromCull = false;
  LodGridCullData heightmapData;
#if DAGOR_DBGLEVEL > 0
  TMatrix4 culltm;
#endif
};

class LandMeshRenderer;
class Occlusion;

// View + pass inputs that affect CULLING. Pure caller-built input for landmesh::frustum_cull: no
// LandMeshRenderer involved, so any number of culls can run concurrently on any thread once the
// level data is loaded. Build with forView(); frustum_cull rejects default-constructed descs.
struct LandMeshCullDesc
{
  Point3 viewPos = Point3(0, 0, 0); // front-to-back traversal center; derives the start cell
  Frustum frustum;
  const Occlusion *occlusion = nullptr;
  HmapOrigin hmapOrigin{Point3(0, 0, 0)}; // heightmap tessellation LOD center; usually == viewPos
  BBox3 renderInBBox;                     // empty = whole map
  const IBBox2 *regions = nullptr;
  int regionsCount = 0;
  bool useDetailedHmap = true;
  bool useExclBox = false; // hide land cells under the tessellated heightmap (see HmapDrawType)
  bool noCulling = false;  // NO_CULLING fast path for bbox-restricted texture bakes
  HeightmapMetricsQuality hmapMetrics;

  // The only valid construction: the heightmap LOD center cannot be forgotten (a default origin
  // would silently center terrain LOD on the world origin). The two-arg overload ties it to the
  // view position, which is what nearly every pass wants.
  static LandMeshCullDesc forView(const Point3 &view_pos, const HmapOrigin &hmap_origin, const Frustum &f)
  {
    LandMeshCullDesc desc;
    desc.viewPos = view_pos;
    desc.hmapOrigin = hmap_origin;
    desc.frustum = f;
    desc.constructed = true;
    return desc;
  }
  static LandMeshCullDesc forView(const Point3 &view_pos, const Frustum &f) { return forView(view_pos, HmapOrigin(view_pos), f); }
  bool isConstructed() const { return constructed; }

protected:
  // Default construction only makes a job member awaiting capture, not a valid cull input.
  bool constructed = false;
};

namespace landmesh
{
// Culling as a pure function of (level data, desc) writing into caller-owned out data; the only
// cull entry point (the old renderer-snapshot path is gone).
void frustum_cull(LandMeshManager &provider, const LandMeshCullDesc &desc, LandMeshCullingData &out);
} // namespace landmesh

struct LandMeshCullingState
{

  /*
    static __forceinline int mirror_x(int x, int x0, int x1)//allows to mirror more then once
    {
      x -= x0;
      int w = x1-x0+1;
      x += (x<0 ? 1 : 0);
      int x_div_w = x/w;
      int abs_xmodw = abs(x%w);
      return x0 + ((x_div_w & 1) ? (w-1 - abs_xmodw) : abs_xmodw);
    }*/
  static __forceinline int mirror_x(int x, int x0, int x1) { return x < x0 ? 2 * x0 - 1 - x : (x > x1 ? 2 * x1 + 1 - x : x); }
  // LandMeshManagerAces states:
  BBox3 *cellBoundings;
  int cellBoundingsCount;
  float *cellBoundingsRadius;
  int cellBoundingsRadiusCount;

  IPoint2 origin;
  int mapSizeX, mapSizeY;

  real landCellSize;
  real gridCellSize;
  Point3 landMeshOffset;
  IBBox2 regions[MAX_LAND_MESH_REGIONS]; // every cell, that is in earliest region will be there. All cells, that not falling into any
                                         // region will be in last one

  // LandMeshRendererAces states:
  int visRange;
  IPoint2 centerCell;
  Point2 centerCellFract;

  int numBorderCellsXPos;
  int numBorderCellsXNeg;
  int numBorderCellsZPos;
  int numBorderCellsZNeg;

  BBox3 renderInBBox;
  IBBox2 exclBox;
  bool useExclBox;

  // Culling functionality:
  BBox3 getBBox(int x, int y, float *sphere_radius = NULL);

  bool calcCellBox(LandMeshManager &provider, int borderX, int borderY, int x0, int y0, int x1, int y1, BBox3 &res);
  void cullDataWithBbox(LandMeshCullingData &dest_data, const LandMeshCullingData &src_data, const BBox2 &bbox);

  void cullCell(LandMeshManager &provider, int borderX, int borderY, int x0, int y0, int x1, int y1, const Frustum &frustum,
    const Frustum::BoxCorrectSAT &sat, const bbox3f &frustumBox, const Occlusion *occlusion, LandMeshCullingData &data);

  void frustumCulling(LandMeshManager &provider, LandMeshCullingData &data, const IBBox2 *regions, int regions_count,
    const HeightmapFrustumCullingInfo &fi);


  // Per-view/per-pass inputs come from the desc, mirror border counts are derived from the
  // manager's mirroring config; no LandMeshRenderer involved.
  void copyLandmeshState(LandMeshManager &provider, const LandMeshCullDesc &desc);


  // Culling modes:
  enum CullMode
  {
    ASYNC_CULLING,
    SYNC_CULLING,
    NO_CULLING,
  };

  CullMode cullMode;

  LandMeshCullingState() { memset(this, 0, sizeof(*this)); }
};
