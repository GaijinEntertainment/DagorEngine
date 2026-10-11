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
  void setNormalMap(TEXTUREID id) { acquire_managed_tex(normalmapTexId = id); }
  void closeNormalMap()
  {
    release_managed_tex(normalmapTexId);
    normalmapTexId = BAD_TEXTUREID;
  }


  // prepare() only binds shader vars and does a distance check; the once-per-frame heightmap
  // texture upload and pending terrain-edit apply live in makeBookKeeping() and must run before it.
  void prepareRenderer(const Point3 &pos)
  {
    ShaderGlobal::set_texture(get_shader_variable_id("landmesh_normalmap", true), normalmapTexId);
    ShaderGlobal::set_float4(get_shader_variable_id("world_to_lightmap"),
      Color4(1.f / (lmeshMgr->getNumCellsX() * lmeshMgr->getLandCellSize()),
        1.f / (lmeshMgr->getNumCellsY() * lmeshMgr->getLandCellSize()), -(float)lmeshMgr->getCellOrigin().x / lmeshMgr->getNumCellsX(),
        -(float)lmeshMgr->getCellOrigin().y / lmeshMgr->getNumCellsY()));
    if (auto *hmap = lmeshMgr->getHmapHandler())
      hmap->makeBookKeeping();
    lmRenderer->prepare(*lmeshMgr, HmapOrigin(pos));
  }

  void render(const RenderView &view, const BBox3 &box)
  {
    if (!lmRenderer)
      return;
    const Point3 &pos = box.isempty() ? view.position : box.center();
    const float oldInvGeomDist = lmRenderer->getInvGeomLodDist();
    if (!box.isempty())
      lmRenderer->setInvGeomLodDist(0.f);
    prepareRenderer(pos);
    lmRenderer->setRenderInBBox(box);
    view.bind();
    lmRenderer->render(view.culling, view.proj, Frustum(view.culling), *lmeshMgr, LandMeshRenderer::RenderType::RENDER_WITH_SPLATTING,
      pos, HmapOrigin(pos));
    lmRenderer->setInvGeomLodDist(oldInvGeomDist);
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
        LmeshMirroringParams lmeshMirror;
        lmRenderer->setMirroring(*lmeshMgr, lmeshMirror.numBorderCellsXPos, lmeshMirror.numBorderCellsXNeg,
          lmeshMirror.numBorderCellsZPos, lmeshMirror.numBorderCellsZNeg);
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

void lmesh_after_device_reset(LMesh &l)
{
  if (!l.lmeshMgr)
    return;
  l.lmeshMgr->afterDeviceReset(l.lmRenderer.get(), true);
  if (auto *hmap = l.lmeshMgr->getHmapHandler())
    hmap->fillHmapTextures();
}

bool load_lmesh_hmap(LMesh &l, IGenLoad &crd)
{
  return l.lmeshMgr ? l.lmeshMgr->loadHeightmapDump(crd, LC_TRIVIAL_DATA | LC_NORMAL_DATA) : false;
};

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

void render_lmesh(LMesh &l, const RenderView &view, const BBox3 &b) { l.render(view, b); }

BBox3 get_lmesh_box(const LMesh &l) { return l.lmeshMgr->getBBox(); }

bool cull_lmesh(const LMesh &l, mat44f_cref globtm, const Point3 &vp, LandMeshCullingData &out)
{
  // constraint: mirrors the draw decision of LandMeshRenderer::renderImpl, which is this simple for an ortho view only
  if (!l.lmRenderer || fabsf(v_extract_w(globtm.col3) - 1.f) >= 0.001f)
    return false;
  const LandMeshManager &mgr = *l.lmeshMgr;
  const bool hmap = mgr.getHmapHandler() && (!mgr.isInTools() || mgr.forceHeightmapRendering);
  LandMeshCullDesc desc = LandMeshCullDesc::forView(vp, Frustum(globtm));
  desc.useExclBox = hmap;
  desc.hmapMetrics = {0.f, 0, HeightmapMetricsQuality::BEST};
  landmesh::frustum_cull(*l.lmeshMgr, desc, out);
  return true;
}

void render_lmesh_culled(LMesh &l, const LandMeshCullingData &culled, const RenderView &view)
{
  l.lmRenderer->setRenderInBBox(BBox3());
  l.prepareRenderer(view.position);
  view.bind();
  l.lmRenderer->renderCulled(*l.lmeshMgr, LandMeshRenderer::RenderType::RENDER_WITH_SPLATTING, culled, view.position);
}

namespace rendinst
{

static DataBlock empty;
const DataBlock &get_detail_data(void *) { return empty; }

}; // namespace rendinst
