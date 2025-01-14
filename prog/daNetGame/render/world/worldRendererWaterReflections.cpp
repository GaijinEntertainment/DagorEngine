// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "private_worldRenderer.h"
#include "global_vars.h"
#include "render/skies.h"
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_lock.h>
#include <render/gaussMipRenderer.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_console.h>
#include <render/deferredRenderer.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_mathUtils.h>
#include "worldRendererQueries.h"
#include <render/viewVecs.h>

static const int WATER_PLANAR_REFLECTION_RESOLUTION_DIV = 6;

void WorldRenderer::initWaterPlanarReflection()
{
  PreparedSkiesParams reflectionSkiesParams;
  reflectionSkiesParams.panoramic = PreparedSkiesParams::Panoramic::OFF;
  reflectionSkiesParams.reprojection = PreparedSkiesParams::Reprojection::ON;
  reflectionSkiesParams.skiesLUTQuality = 1;
  reflectionSkiesParams.scatteringScreenQuality = 1;
  reflectionSkiesParams.scatteringDepthSlices = 32;
  reflectionSkiesParams.transmittanceColorQuality = 1;
  reflectionSkiesParams.scatteringRangeScale = 1.0f;
  refl_pov_data = get_daskies()->createSkiesData("refl", reflectionSkiesParams);

  planarReflectionMipRenderer = eastl::make_unique<GaussMipRenderer>();
  planarReflectionMipRenderer->init();

  waterPlanarReflectionCloudsNode =
    dabfg::register_node("water_planar_reflection_clouds_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.multiplex(dabfg::multiplexing::Mode::None);
      auto waterPlanarReflection = registry
                                     .createTexture2d("water_planar_reflection_clouds", dabfg::History::No,
                                       {TEXFMT_A16B16G16R16F | TEXCF_RTARGET,
                                         registry.getResolution<2>("main_view", 1.0f / WATER_PLANAR_REFLECTION_RESOLUTION_DIV), 5})
                                     .atStage(dabfg::Stage::POST_RASTER)
                                     .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                     .handle();
      registry.create("water_planar_reflection_clouds_sampler", dabfg::History::No).blob(d3d::request_sampler({}));
      return [this, waterPlanarReflection]() {
        FRAME_LAYER_GUARD(-1);
        ShaderGlobal::set_int(use_custom_fogVarId, 0);

        Point3 reflectionPos = waterPlanarReflectionViewItm.getcol(3);
        Point3 reflectionDir = waterPlanarReflectionViewItm.getcol(2);

        d3d::set_render_target(waterPlanarReflection.get(), 0);

        d3d::settm(TM_VIEW, waterPlanarReflectionViewTm);
        d3d::settm(TM_PROJ, &waterPlanarReflectionProjTm);
        set_viewvecs_to_shader(waterPlanarReflectionViewTm, waterPlanarReflectionProjTm);

        get_daskies()->renderEnvi(true, dpoint3(reflectionPos), dpoint3(reflectionDir), 0, UniqueTex{}, UniqueTex{}, BAD_TEXTUREID,
          refl_pov_data, waterPlanarReflectionViewTm, waterPlanarReflectionProjTm, waterPlanarReflectionPersp);

        planarReflectionMipRenderer->render(waterPlanarReflection.get());

        ShaderGlobal::set_int(use_custom_fogVarId, 1);
      };
    });
}

void WorldRenderer::initWaterPlanarReflectionTerrainNode()
{
  waterPlanarReflectionTerrainNode =
    dabfg::register_node("water_planar_reflection_terrain_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.setPriority(dabfg::PRIO_AS_LATE_AS_POSSIBLE); // Avoid waiting in renderLmeshReflection();

      // Avoid waiting in renderLmeshReflection(); since PRIO_AS_LATE_AS_POSSIBLE doesn't work
      registry.orderMeAfter("resolve_gbuffer_node");
      auto waterPlanarReflection = registry
                                     .createTexture2d("water_planar_reflection_terrain", dabfg::History::No,
                                       {TEXFMT_R11G11B10F | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f), 1})
                                     .atStage(dabfg::Stage::POST_RASTER)
                                     .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                     .handle();
      auto waterPlanarReflectionDepth = registry
                                          .createTexture2d("water_planar_reflection_terrain_depth", dabfg::History::No,
                                            {TEXFMT_DEPTH16 | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f), 1})
                                          .atStage(dabfg::Stage::POST_RASTER)
                                          .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                          .handle();
      return [this, waterPlanarReflection, waterPlanarReflectionDepth]() {
        FRAME_LAYER_GUARD(-1);
        d3d::set_render_target(waterPlanarReflection.get(), 0);
        d3d::set_depth(waterPlanarReflectionDepth.get(), DepthAccess::RW);
        d3d::clearview(CLEAR_ZBUFFER | CLEAR_TARGET, 0, 0, 0);

        d3d::settm(TM_VIEW, waterPlanarReflectionViewTm);
        d3d::settm(TM_PROJ, &waterPlanarReflectionProjTm);
        set_viewvecs_to_shader(waterPlanarReflectionViewTm, waterPlanarReflectionProjTm);

        renderLmeshReflection();

        waterPlanarReflectionDepth.view()->disableSampler();
      };
    });
}


void WorldRenderer::closeWaterPlanarReflection()
{
  get_daskies()->destroy_skies_data(refl_pov_data);
  refl_pov_data = nullptr;
  waterPlanarReflectionCloudsNode = {};
  planarReflectionMipRenderer.reset();
}

void WorldRenderer::calcWaterPlanarReflectionMatrix()
{
  bool volumtetricClouds = get_daskies() && !get_daskies()->panoramaEnabled();
  if (!isWaterPlanarReflectionTerrainEnabled() && !volumtetricClouds)
    return;

  TMatrix reflection;
  reflection.setcol(0, 1.f, 0.f, 0.f);
  reflection.setcol(1, 0.f, -1.f, 0.f);
  reflection.setcol(2, 0.f, 0.f, 1.f);
  reflection.setcol(3, 0.f, 2.f * waterLevel, 0.f);

  TMatrix itm = reflection * currentFrameCamera.viewItm;
  itm.setcol(1, -itm.getcol(1)); // Flip Y to preserve culling.

  TMatrix4 proj = currentFrameCamera.noJitterProjTm; // jitter causes flickering while looking down close to the waterLevel

  proj(2, 1) *= -1; // if perspective matrix is not symmetrical, we should have the opposite y offset for waterProj matrix
  Point4 worldClipPlane(0, 1, 0, -waterLevel);
  TMatrix4 waterProj = oblique_projection_matrix_reverse<true>(proj, itm, worldClipPlane);
  if (fabs(det4x4(waterProj)) < 1e-15) // near plane is not perpendicular to water plane
    waterProj = currentFrameCamera.noJitterProjTm;

  waterPlanarReflectionViewItm = itm;
  waterPlanarReflectionViewTm = inverse(itm);
  waterPlanarReflectionProjTm = waterProj;
  // some params like star brightness depend on persp, for these we want to use the same as for sky rendering
  waterPlanarReflectionPersp = currentFrameCamera.noJitterPersp;
}

void WorldRenderer::setupSkyPanoramaAndReflectionFromSetting(bool first_init)
{
  const DataBlock &blk = *originalLevelBlk.getBlockByNameEx("skies");
  panoramaPosition = blk.getPoint3("panoramaPosition", Point3(0, 10, 0)); // by default, set at 10meters above sea level
  bool usePanoramaLevel = blk.getBool("usePanorama", true);
  const char *cloudsQuality = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("cloudsQuality", "default");
  bool volumetricClouds = strcmp(cloudsQuality, "volumetric") == 0;
  bool usePanorama = is_panorama_forced() || (isForwardRender() ? true : (usePanoramaLevel && !volumetricClouds));

  setupSkyPanoramaAndReflection(usePanorama, first_init);
}

void WorldRenderer::setupSkyPanoramaAndReflection(bool use_panorama, bool first_init)
{
  // Close both, just in case the console command made the actual state inconsistent with the graphics setting
  get_daskies()->closePanorama();
  closeWaterPlanarReflection();

  if (!use_panorama)
  {
    debug("Initializing volumetric clouds and planar reflection");
    initWaterPlanarReflection();
  }
  else
  {
    const DataBlock &blk = *originalLevelBlk.getBlockByNameEx("skies");
    eastl::string cloudsQuality = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("cloudsQuality", "default");
    int panoramaScale = (cloudsQuality == "highres" || cloudsQuality == "volumetric") ? 2 : 1; // Use highres when panorama is forced
    bool panoramaBlending = blk.getBool("panoramaBlending", false);
    int panoramaResolution = blk.getInt("panoramaResolution", 2048) * panoramaScale;
    bool compressPanorama = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("compress_panorama", false);

    debug("Initializing panorama sky with scale %d", panoramaScale);
    get_daskies()->initPanorama(main_pov_data, panoramaBlending, panoramaResolution, 0, compressPanorama);
    get_daskies()->invalidatePanorama(true);
    if (!first_init)
    {
      TMatrix viewTm = TMatrix::IDENT;
      viewTm.setcol(3, panoramaPosition);
      Driver3dPerspective persp{1, 1, 0.001, 10000};
      TMatrix4 projTm;
      d3d::calcproj(persp, projTm);
      // This is to reinit panorama with a good looking position.
      // If this weren't here, panorama could be rendered from an airplane within a cloud, and that looks really bad.
      d3d::GpuAutoLock lock;
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
      get_daskies()->prepareSkyAndClouds(true, dpoint3(panoramaPosition), DPoint3(1, 0, 0), 3, TextureIDPair(), TextureIDPair(),
        main_pov_data, viewTm, projTm, true, false);
    }
  }

  // Envi node depends on panorama
  if (!isForwardRender())
    createEnvironmentNode();

  validate_volumetric_clouds_settings();
}

// for debugging
void WorldRenderer::switchVolumetricAndPanoramicClouds()
{
  bool usePanorama = !get_daskies()->panoramaEnabled();
  setupSkyPanoramaAndReflection(usePanorama, false);
  if (usePanorama)
    console::print_d("switching to panoramic clouds");
  else
    console::print_d("switching to volumetric clouds with planar water reflection");
}