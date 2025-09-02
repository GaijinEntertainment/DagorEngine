// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "level.h"
#include <ioSys/dag_fileIo.h>
#include <landMesh/lmeshManager.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshMirroring.h>
#include <shaders/dag_shaders.h>
#include <3d/dag_texMgrTags.h>
#include <perfMon/dag_statDrv.h>

struct LMesh
{
  TEXTUREID normalmapTexId = BAD_TEXTUREID;
  eastl::unique_ptr<LandMeshManager> lmeshMgr;
  eastl::unique_ptr<LandMeshRenderer> lmRenderer;
  void setNormalMap(TEXTUREID id)
  {
    acquire_managed_tex(normalmapTexId = id);
    ShaderGlobal::set_texture(get_shader_variable_id("landmesh_normalmap", true), normalmapTexId);
  }
  void closeNormalMap()
  {
    release_managed_tex(normalmapTexId);
    normalmapTexId = BAD_TEXTUREID;
  }


  void render(const BBox3 &box)
  {
    const float oldInvGeomDist = lmRenderer->getInvGeomLodDist();
    lmRenderer->setInvGeomLodDist(0.f);

    lmRenderer->prepare(*lmeshMgr, box.center(), 1.f);
    lmRenderer->setRenderInBBox(box);
    lmRenderer->render(*lmeshMgr, LandMeshRenderer::RenderType::RENDER_WITH_SPLATTING, box.center());
    lmRenderer->setInvGeomLodDist(oldInvGeomDist);
  }
  void render(const Point3 &view_pos)
  {
    if (!lmRenderer)
      return;
    lmRenderer->setRenderInBBox(BBox3());
    lmRenderer->prepare(*lmeshMgr, view_pos, 1.f);
    // LMeshRenderingMode::RENDERING_LANDMESH
    lmRenderer->render(*lmeshMgr, LandMeshRenderer::RenderType::RENDER_WITH_SPLATTING, view_pos);
  }
  bool loadMeshData(IGenLoad &crd)
  {
    int landMeshOffset = crd.tell();
    if (landMeshOffset >= 0)
    {
      textag_mark_end();
      const uint32_t dataNeeded = LC_TRIVIAL_DATA | LC_NORMAL_DATA; // LC_DETAIL_DATA |
      lmeshMgr = eastl::make_unique<LandMeshManager>();
      debug("[load]landmesh data");
      lmeshMgr->resetRenderDataNeeded(LC_ALL_DATA);
      lmeshMgr->setRenderDataNeeded(dataNeeded);

      if (!lmeshMgr->loadDump(crd, midmem, dataNeeded != 0))
      {
        lmeshMgr.reset();
        debug("can't load lmesh");
      }
      if (lmeshMgr)
      {
        lmRenderer.reset(lmeshMgr->createRenderer());
        Color4 world_to_lightmap(1.f / (lmeshMgr->getNumCellsX() * lmeshMgr->getLandCellSize()),
          1.f / (lmeshMgr->getNumCellsY() * lmeshMgr->getLandCellSize()),
          -(float)lmeshMgr->getCellOrigin().x / lmeshMgr->getNumCellsX(),
          -(float)lmeshMgr->getCellOrigin().y / lmeshMgr->getNumCellsY());

        ShaderGlobal::set_color4(get_shader_variable_id("world_to_lightmap"), world_to_lightmap);
        LmeshMirroringParams lmeshMirror;
        lmRenderer->setMirroring(*lmeshMgr, lmeshMirror.numBorderCellsXPos, lmeshMirror.numBorderCellsXNeg,
          lmeshMirror.numBorderCellsZPos, lmeshMirror.numBorderCellsZNeg, lmeshMirror.mirrorShrinkXPos, lmeshMirror.mirrorShrinkXNeg,
          lmeshMirror.mirrorShrinkZPos, lmeshMirror.mirrorShrinkZNeg);
      }
    }

    crd.seekrel(crd.getBlockRest());
    textag_mark_begin(TEXTAG_LAND);
    return true;
  }
};

const HeightmapHandler *get_lmesh_hmap(const LMesh &l)
{
  return l.lmeshMgr && l.lmeshMgr->getHmapHandler() ? l.lmeshMgr->getHmapHandler() : nullptr;
}

LMesh *create_lmesh(IGenLoad &cb)
{
  auto l = new LMesh;
  if (!l->loadMeshData(cb))
    del_it(l);
  return l;
}

bool init_lmesh(LMesh &l)
{
  if (!l.lmeshMgr || !l.lmeshMgr->getHmapHandler())
    return false;
  if (l.lmeshMgr->getHmapHandler())
  {
    auto &h = *l.lmeshMgr->getHmapHandler();
    h.invalidateCulling(IBBox2{{0, 0}, {h.hmapWidth.x - 1, h.hmapWidth.y - 1}});
  }
  return true;
}

bool load_lmesh_hmap(LMesh &l, IGenLoad &crd)
{
  return l.lmeshMgr ? l.lmeshMgr->loadHeightmapDump(crd, LC_TRIVIAL_DATA | LC_NORMAL_DATA) : false;
};

bool get_lmesh_min_max(const LMesh &l, const BBox3 &box, float &minHt, float &maxHt)
{
  if (!l.lmeshMgr->getHmapHandler())
    return false;
  auto &heightmap = *l.lmeshMgr->getHmapHandler();

  if (!heightmap.heightmapHeightCulling)
  {
    minHt = heightmap.worldBox[0].y;
    maxHt = heightmap.worldBox[1].y;
    return true;
  }
  const float patchSz = max(box.width().z, box.width().x);
  const int lod = floorf(heightmap.heightmapHeightCulling->getLod(patchSz));
  heightmap.heightmapHeightCulling->getMinMax(lod, Point2::xz(box[0]), patchSz, minHt, maxHt);
  return true;
}

bool get_height(const LMesh &l, const Point2 &p, float &ht, Point3 *normal) { return l.lmeshMgr->getHeight(p, ht, normal); }

void set_lmesh_normalmap(LMesh *l, TEXTUREID id)
{
  if (l)
    l->setNormalMap(id);
}

void destroy_lmesh(LMesh *l)
{
  if (l)
    l->closeNormalMap();
  del_it(l);
}

void render_lmesh(LMesh &l, const Point3 &vp, const BBox3 &b)
{
  if (b.isempty())
    l.render(vp);
  else
    l.render(b);
}

BBox3 get_lmesh_box(const LMesh &l) { return l.lmeshMgr->getBBox(); }

namespace rendinst
{

static DataBlock empty;
const DataBlock &get_detail_data(void *) { return empty; }

}; // namespace rendinst
