// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <streaming/dag_streamingBase.h>
#include <heightmap/heightmapHandler.h>
#include <EASTL/utility.h>
#include "renderView.h"

struct LMesh;
struct LandMeshCullingData;
void destroy_lmesh(LMesh *l);
LMesh *create_lmesh(IGenLoad &cb);
void render_lmesh(LMesh &l, const RenderView &view, const BBox3 &);
BBox3 get_lmesh_box(const LMesh &l);
// any thread; false when only render_lmesh draws the land mesh for globtm
bool cull_lmesh(const LMesh &l, mat44f_cref globtm, const Point3 &vp, LandMeshCullingData &out);
void render_lmesh_culled(LMesh &l, const LandMeshCullingData &culled, const RenderView &view);

class StrmSceneHolder : public BaseStreamingSceneHolder
{
  TEXTUREID normalmapTexId = BAD_TEXTUREID;

public:
  typedef BaseStreamingSceneHolder base;
  using BaseStreamingSceneHolder::mainBindump;
  HeightmapHandler heightmap;
  UniqueBuf heightmap_edges, heightmap_morph;

  const HeightmapHandler *getHeightmap() const;
  struct LmeshDeleter
  {
    void operator()(LMesh *ptr) { ptr ? destroy_lmesh(ptr) : (void)0; }
  };
  eastl::unique_ptr<LMesh, LmeshDeleter> lMesh;
  enum
  {
    No,
    Loaded,
    Inited
  } state = No;
  void onSceneLoaded();
  void afterDeviceReset();
  bool bdlCustomLoad(unsigned /*bindump_id*/, int tag, IGenLoad &crd, dag::ConstSpan<TEXTUREID> texMap) override;
  void bdlTextureMap(unsigned /*bindump_id*/, dag::ConstSpan<TEXTUREID> texId) override;

  bool loadMeshData(IGenLoad &loadCb);
  bool loadMeshData2(IGenLoad &cb);
  void loadHeightmap(IGenLoad &crd);
  void loadHeightmap(const char *fn);
  void loadHeightmapRaw(const char *fn, float cell_size);
  void loadHeightmapRaw16(const char *fn, float cell_size, float hMin, float hMax);
  void loadLMesh(IGenLoad &crd) { lMesh.reset(create_lmesh(crd)); }
  void initOnDemand();
  void renderHeightmap(const RenderView &view);
  bool getMinMax(float &minHt, float &maxHt) const;
  void renderLmesh(const RenderView &view, BBox3 box = BBox3())
  {
    if (lMesh)
      render_lmesh(*lMesh, view, box);
  }
  bool getLmeshBBox3(BBox3 &box)
  {
    if (!lMesh)
      return false;
    box = get_lmesh_box(*lMesh);
    return true;
  }
  enum class Hmap
  {
    Off,
    On
  };
  enum class Lmesh
  {
    Off,
    On
  };
  void render(const RenderView &view, Hmap hmap, Lmesh lmesh, const LandMeshCullingData *culled = nullptr);
  void renderTrans(const RenderView &view);
  void render(const RenderView &view, int render_id, unsigned render_flags_mask)
  {
    view.bind();
    base::render(view.visibility(), render_id, render_flags_mask);
  }

  bool getHeight(const Point2 &p, float &ht, Point3 *normal) const;
};
