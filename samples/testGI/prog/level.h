// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <streaming/dag_streamingBase.h>
#include <heightmap/heightmapHandler.h>
#include <heightmap/flexGridRenderer.h>
#include <EASTL/utility.h>
#include "dag_cur_view.h"

struct LMesh;
void destroy_lmesh(LMesh *l);
LMesh *create_lmesh(IGenLoad &cb);
void render_lmesh(LMesh &l, const Point3 &vp, const BBox3 &);
BBox3 get_lmesh_box(const LMesh &l);

class StrmSceneHolder : public BaseStreamingSceneHolder
{
  TEXTUREID normalmapTexId = BAD_TEXTUREID;

public:
  typedef BaseStreamingSceneHolder base;
  using BaseStreamingSceneHolder::mainBindump;
  HeightmapHandler heightmap;

  FlexGridConfig flexGridConfig;
  FlexGridRenderer flexGridRenderer;

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
  void renderHeightmap(const vec4f &viewPos, const Frustum &frustum, const TMatrix4 &proj);
  bool getMinMax(float &minHt, float &maxHt) const;
  bool getMinMax(const BBox3 &box, float &minHt, float &maxHt);
  void renderLmesh(BBox3 box = BBox3())
  {
    if (lMesh)
      render_lmesh(*lMesh, ::grs_cur_view.itm.getcol(3), box);
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
  void render(const VisibilityFinder &vf, Hmap hmap, Lmesh lmesh, const TMatrix4 &proj);
  void render(const VisibilityFinder &vf, int render_id, unsigned render_flags_mask)
  {
    base::render(vf, render_id, render_flags_mask);
  }

  bool getHeight(const Point2 &p, float &ht, Point3 *normal) const;
};
