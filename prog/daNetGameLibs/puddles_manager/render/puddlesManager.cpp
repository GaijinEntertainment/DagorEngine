// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "puddlesManager.h"

#include <math/dag_Point2.h>
#include <math/integer/dag_IPoint2.h>
#include <render/scopeRenderTarget.h>
#include <landMesh/lmeshManager.h>
#include <heightmap/heightmapHandler.h>

#include <ioSys/dag_dataBlock.h>

#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <util/dag_convar.h>

#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_renderScene.h>

#include <perfMon/dag_statDrv.h>

#include <render/toroidal_update.h>

#include <render/puddles_remove_consts.hlsli>

#include <main/level.h>
#include <landMesh/lmeshManager.h>
#include <render/renderEvent.h>
#include <ecs/core/entityManager.h>
#include <ecs/render/updateStageRender.h>
#include <daECS/core/coreEvents.h>

#include "puddlesManagerEvents.h"

#define GLOBAL_VARS_LIST        \
  VAR(world_to_puddles_tex_ofs) \
  VAR(world_to_puddles_ofs)     \
  VAR(puddle_toroidal_view)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

CONSOLE_BOOL_VAL("render", puddles_invalidate, false);

void PuddlesManager::invalidatePuddles(bool force)
{
  G_UNUSED(force);
  // if (force)
  puddlesHelper.curOrigin = IPoint2(-100000, 330000);
}

void PuddlesManager::setPuddlesScene(RenderScene *scn)
{
  puddlesScene = scn;
  if (puddlesScene)
    puddlesSceneBbox = puddlesScene->calcBoundingBox();
  invalidatePuddles(false);
}

void PuddlesManager::preparePuddles(const Point3 &origin)
{
  if (!puddles.getTex2D() || !lmeshMgr || !lmeshMgr->getHmapHandler())
    return;

  if (removedPuddlesNeedUpdate && removedPuddlesActualSize > 0)
  {
    puddlesRemovedBuf.getBuf()->updateDataWithLock(0, sizeof(puddlesRemoved[0]) * removedPuddlesActualSize, puddlesRemoved.begin(),
      VBLOCK_DISCARD);
    removedPuddlesNeedUpdate = false;
  }

  Point2 alignedOrigin = Point2::xz(origin);
  const float fullDistance = 2 * puddlesDist;
  const float texelSize = (fullDistance / puddlesHelper.texSize);
  enum
  {
    TEXEL_ALIGN = 4
  };
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);
  if (puddles_invalidate.get())
    invalidatePuddles(true);

  enum
  {
    THRESHOLD = TEXEL_ALIGN * 4
  };
  IPoint2 move = abs(puddlesHelper.curOrigin - newTexelsOrigin);
  if (move.x >= THRESHOLD || move.y >= THRESHOLD)
  {
    TIME_D3D_PROFILE(puddles);
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * puddlesHelper.texSize;
    // if distance travelled is too big, there is no need to update movement in two steps
    if (max(move.x, move.y) < fullUpdateThresholdTexels)
    {
      if (move.x < move.y)
        newTexelsOrigin.x = puddlesHelper.curOrigin.x;
      else
        newTexelsOrigin.y = puddlesHelper.curOrigin.y;
    }
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    ToroidalGatherCallback::RegionTab tab;
    ToroidalGatherCallback cb(tab);
    toroidal_update(newTexelsOrigin, puddlesHelper, fullUpdateThresholdTexels, cb);

    Point2 ofs = point2((puddlesHelper.mainOrigin - puddlesHelper.curOrigin) % puddlesHelper.texSize) / puddlesHelper.texSize;

    alignedOrigin = point2(puddlesHelper.curOrigin) * texelSize;
    d3d::set_render_target(puddles.getTex2D(), 0);
    // todo: we'd better align to hmap world pos ofs
    ShaderGlobal::set_color4(world_to_puddles_ofsVarId, 1.0f / fullDistance, puddleLod, -alignedOrigin.x / fullDistance + 0.5,
      -alignedOrigin.y / fullDistance + 0.5);
    TMatrix vtm;
    if (puddlesScene)
    {
      vtm.setcol(0, 1, 0, 0);
      vtm.setcol(1, 0, 0, 1);
      vtm.setcol(2, 0, 1, 0);
      vtm.setcol(3, 0, 0, 0);
      d3d::settm(TM_VIEW, vtm);
    }
    for (auto &reg : tab)
    {
      ShaderGlobal::set_color4(world_to_puddles_tex_ofsVarId, texelSize * (reg.texelsFrom.x - reg.lt.x),
        texelSize * (reg.texelsFrom.y - reg.lt.y), puddleLod, texelSize);
      d3d::setview(reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y, 0, 1);
      puddlesRenderer.render();
      if (puddlesScene)
      {
        BBox2 cBox(texelSize * point2(reg.texelsFrom), texelSize * point2(reg.texelsFrom + reg.wd));
        if (cBox & BBox2(Point2::xz(puddlesSceneBbox[0]), Point2::xz(puddlesSceneBbox[1])))
        {
          alignas(16) TMatrix4 proj = matrix_ortho_off_center_lh(cBox[0].x, cBox[1].x, cBox[1].y, cBox[0].y, puddlesSceneBbox[0].y - 1,
            puddlesSceneBbox[1].y + 1);
          d3d::settm(TM_PROJ, &proj);
          shaders::overrides::set(blendMaxState);

          VisibilityFinder vf;
          Frustum cullingFrustum = TMatrix4(vtm) * proj;

          vf.set(v_splats(0), cullingFrustum, 0.0f, 0.0f, 1.0f, 1.0f, nullptr);
          puddlesScene->render(vf, -1);
          shaders::overrides::reset();
        }
      }
      if (removedPuddlesActualSize > 0)
      {
        ShaderGlobal::set_color4(puddle_toroidal_viewVarId, reg.lt.x, reg.lt.y, reg.wd.x, reg.wd.y);
        d3d::setvsrc(0, 0, 0);
        puddlesRemover.shader->setStates();
        d3d::draw_instanced(PRIM_TRISTRIP, 0, 2, removedPuddlesActualSize);
      }
    }
    ShaderGlobal::set_color4(world_to_puddles_tex_ofsVarId, ofs.x, ofs.y, 1. / puddlesHelper.texSize, puddleLod);
    d3d::resource_barrier({puddles.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE | RB_STAGE_PIXEL, 0, 0});
  }

  // SCOPE_RENDER_TARGET;
  // d3d::set_render_target(puddles.getTex2D(), 0, false);
  // puddlesRenderer.render();
}

void PuddlesManager::puddlesAfterDeviceReset()
{
  // this looks stupid but I'm simply preserving the original behavior
  setPuddlesScene(puddlesScene);
  if (!puddlesRemoved.empty() && removedPuddlesActualSize > 0)
    puddlesRemovedBuf.getBuf()->updateDataWithLock(0, sizeof(puddlesRemoved[0]) * removedPuddlesActualSize, puddlesRemoved.begin(),
      VBLOCK_DISCARD);
}

void PuddlesManager::init(const LandMeshManager *lmesh_mgr, const DataBlock &settings, int forced_max_resolution)
{
#define VAR(a) a##VarId = get_shader_variable_id(#a);
  GLOBAL_VARS_LIST
#undef VAR

  close();
  if (!lmesh_mgr || !lmesh_mgr->getHmapHandler())
    return;

  puddlesSettings = settings;

  lmeshMgr = lmesh_mgr;
  const int puddlesRes = forced_max_resolution > 0 ? forced_max_resolution : clamp(settings.getInt("puddlesDist", 512), 0, 2048);
  if (puddlesRes > 0)
  {
    float puddlesTexelSize = settings.getReal("puddlesTexel", 1.0f);
    puddlesDist = (puddlesRes / 2.f) * puddlesTexelSize;

    float hmapTexelSize = lmeshMgr->getHmapHandler()->getHeightmapCellSize();
    puddleLod = (int)max(0.f, floorf(log2f(puddlesTexelSize / hmapTexelSize) + 0.5f));

    debug("init puddles map rex %d^2, %f texel heightmapLod = %f", puddlesRes, puddlesTexelSize, puddleLod);
    // puddles.set(d3d::create_tex(NULL, puddlesRes, puddlesRes, TEXCF_RTARGET|TEXFMT_R8, 1, "puddles"), "puddles");
    puddles = dag::create_tex(NULL, puddlesRes, puddlesRes, TEXCF_RTARGET | TEXFMT_L16, 1, "puddles");
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
      ShaderGlobal::set_sampler(get_shader_variable_id("puddles_samplerstate"), d3d::request_sampler(smpInfo));
    }
    puddles.setVar();
    puddlesRenderer.init("make_puddles");
    puddlesRemover.init("remove_puddles_in_craters", nullptr, 0, "remove_puddles_in_craters");
    puddlesHelper.curOrigin = IPoint2(-1000000, 1000000);
    puddlesHelper.texSize = puddlesRes;
    puddlesRemovedBuf = dag::buffers::create_persistent_cb(MAX_PUDDLES_REMOVED, "removed_puddles_buf");
    puddlesRemoved.resize(MAX_PUDDLES_REMOVED);
    removedPuddlesNeedUpdate = false;
    removedPuddlesIndexToAdd = 0;
    removedPuddlesActualSize = 0;
  }
  else
    ShaderGlobal::set_color4(world_to_puddles_ofsVarId, 0, 10, 0, 0);


  shaders::OverrideState state;
  state.set(shaders::OverrideState::BLEND_OP);
  state.blendOp = BLENDOP_MAX;
  blendMaxState = shaders::overrides::create(state);
}

void PuddlesManager::reinit_same_settings(const LandMeshManager *lmesh_mgr, int forced_max_resolution)
{
  init(lmesh_mgr, puddlesSettings, forced_max_resolution);
}

void PuddlesManager::close()
{
  lmeshMgr = nullptr;
  puddlesRenderer.clear();
  puddlesRemover.close();
  puddles.close();
  puddlesRemoved.clear();
  puddlesRemovedBuf.close();
  blendMaxState.reset();
}

void PuddlesManager::removePuddlesInCrater(const Point3 &pos, float radius)
{
  if (puddlesRemoved.size() < MAX_PUDDLES_REMOVED)
  {
    // Puddles are not initialized yet
    return;
  }
  puddlesRemoved[removedPuddlesIndexToAdd] = Point4(pos.x, pos.y, pos.z, radius);
  removedPuddlesIndexToAdd++;
  removedPuddlesActualSize = max(removedPuddlesActualSize, removedPuddlesIndexToAdd);
  removedPuddlesIndexToAdd %= MAX_PUDDLES_REMOVED;
  removedPuddlesNeedUpdate = true;
}
