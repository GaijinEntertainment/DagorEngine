#include "de3_worldrenderer.h"
#include "de3_visibility_finder.h"
#include "de3_ICamera.h"
#include <3d/dag_drv3d.h>
#include <gui/dag_stdGuiRender.h>
#include <render/fx/dag_postfx.h>
#include <render/fx/dag_demonPostFx.h>
#include <fx/dag_hdrRender.h>
#include <scene/dag_visibility.h>
#include <shaders/dag_renderScene.h>
#include <shaders/dag_shaders.h>
#include <shaders/dag_shaderBlock.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_gameSettings.h>
#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>
#include <util/dag_globDef.h>
#include <shaders/dag_overrideStates.h>

static int worldViewPosGvId = -1;

static float visibleScale = 1.0;
static float maxVisibleRange = MAX_REAL;
static float visibility_multiplier = 1.0;
static float sunScale = 1.0;
static float ambScale = 1.0;
static float CSMlambda = 0.7;
static float shadowDistance = 450;
static float shadowCasterDist = 50;
enum
{
  SHADOWS_QUALITY_MAX = 5,
  SSAO_QUALITY_MAX = 3
};

bool WorldRenderer::renderWater = false, WorldRenderer::renderShadow = false, WorldRenderer::renderShadowVsm = false,
     WorldRenderer::renderEnvironmentFirst = true, WorldRenderer::enviReqSceneBlk = false;
int WorldRenderer::shadowQuality = 3;
static int globalFrameBlockId = -1;

struct VolFogCallback : public DemonPostFxCallback
{
  int process(int, int, Color4) override { return 0; }
};
static VolFogCallback volFogCallback;

WorldRenderer::WorldRenderer(WorldRenderer::IWorld *render_world, const DataBlock &blk)
{
  G_ASSERT(render_world);
  postFx = NULL;
  targetW = targetH = sceneFmt = 0;
  sceneRt = postfxRt = NULL;
  sceneRtId = postfxRtId = BAD_TEXTUREID;
  sceneDepth = NULL; // It isn't initialized as well as other RTs. It's here just for consistency if this code ever will be needed.
                     // (currently it's dead code)

  gameParamsBlk = new DataBlock;
  gameParamsBlk->setFrom(&blk);

  renderWorld = render_world;
  allowDynamicRender = useDynamicRender = false;
  globalFrameBlockId = ShaderGlobal::getBlockId("global_frame", ShaderGlobal::LAYER_FRAME);
  envSetts.init();
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_WRITE_DISABLE);
  colorOnlyOverride = shaders::overrides::create(state);
}

void WorldRenderer::close()
{
  waterRender.close();

  droplets.close();
  del_d3dres(sceneRt);
  del_d3dres(sceneRt);
  del_d3dres(postfxRt);
  del_d3dres(sceneDepth);
}

void WorldRenderer::update(float dt)
{
  windEffect.updateWind(dt);
  postFx->update(dt);
}

void WorldRenderer::render(DagorGameScene &)
{
  beforeRender();

  // render world to targtex
  Driver3dRenderTarget rt;
  d3d::get_render_target(rt);

  int viewportX, viewportY, viewportW, viewportH;
  float viewportMinZ, viewportMaxZ;
  d3d::getview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);

  d3d::set_render_target(sceneRt, 0);
  d3d::set_depth(sceneDepth, DepthAccess::RW);
  d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, E3DCOLOR(0, 0, 0, 0), DAGOR_FAR_DEPTH, 0);

  {
    classicRender();
    classicRenderTrans();
  }

  if (::hdr_render_mode != HDR_MODE_NONE)
  {
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    postFx->apply(sceneRt, sceneRtId, postfxRt, postfxRtId, ::grs_cur_view.tm, projTm);
    d3d::set_render_target(postfxRt, 0);
    d3d_err(d3d::stretch_rect(postfxRt, rt.getColor(0).tex, NULL, NULL));
  }
  else
    d3d_err(d3d::stretch_rect(sceneRt, rt.getColor(0).tex, NULL, NULL));

  d3d_err(d3d::set_render_target(rt));
  d3d::setview(viewportX, viewportY, viewportW, viewportH, viewportMinZ, viewportMaxZ);

  renderWorld->renderUi();
  StdGuiRender::reset_per_frame_dynamic_buffer_pos();
}

void WorldRenderer::restartPostfx(DataBlock *postfxBlkTemp)
{
  if (!postFx->updateSettings(postfxBlkTemp, NULL))
    postFx->restart(postfxBlkTemp, NULL, NULL);
}

static void set_visibility_range(float) { update_visibility_finder(); }

void WorldRenderer::prepareWaterReflection() {}

void WorldRenderer::beforeRender()
{
  ShaderGlobal::set_color4_fast(worldViewPosGvId, Color4(::grs_cur_view.pos.x, ::grs_cur_view.pos.y, ::grs_cur_view.pos.z, 1.f));
  envSetts.applyOnRender(true);
  renderWorld->beforeRender();

  update_visibility_finder();

  if (globalFrameBlockId != -1)
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  if (renderWater)
    prepareWaterReflection();
}

void WorldRenderer::classicRender()
{
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  renderGeomEnvi();
  renderGeomOpaque();
}
void WorldRenderer::classicRenderTrans()
{
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  renderWorld->renderGeomTrans();
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
}

void WorldRenderer::renderGeomOpaque()
{
  windEffect.setShaderVars(::grs_cur_view.itm);
  renderWorld->renderGeomOpaque();
}
void WorldRenderer::renderGeomEnvi()
{
  int l, t, w, h;
  float minZ, maxZ;
  d3d::getview(l, t, w, h, minZ, maxZ);
  d3d::setview(l, t, w, h, DAGOR_FAR_DEPTH, DAGOR_FAR_DEPTH);
  shaders::overrides::set(colorOnlyOverride);

  renderWorld->renderGeomEnvi();

  d3d::setview(l, t, w, h, minZ, maxZ);

  shaders::overrides::reset();
}
