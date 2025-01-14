// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "nodes.h"

#include <render/antiAliasing.h>
#include <render/renderEvent.h>
#include <render/skies.h>
#include <render/world/global_vars.h>
#include <render/world/private_worldRenderer.h>

#include <daECS/core/entityManager.h>
#include <render/debugGbuffer.h>
#include <render/deferredRenderer.h>

#include <scene/dag_occlusion.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/world/sunParams.h>
#include <fftWater/fftWater.h>

extern ConVarT<float, 1> taa_base_mip_scale;

static void storeCurrentGlobTmNoOffsInShader(const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  TMatrix viewRot = view_tm;
  viewRot.setcol(3, 0, 0, 0);
  TMatrix4 globtm = TMatrix4(viewRot) * proj_tm;
  globtm = globtm.transpose();

  static int globtm_no_ofs_psf_0VarId = ::get_shader_variable_id("globtm_no_ofs_psf_0", true);
  static int globtm_no_ofs_psf_1VarId = ::get_shader_variable_id("globtm_no_ofs_psf_1", true);
  static int globtm_no_ofs_psf_2VarId = ::get_shader_variable_id("globtm_no_ofs_psf_2", true);
  static int globtm_no_ofs_psf_3VarId = ::get_shader_variable_id("globtm_no_ofs_psf_3", true);

  ShaderGlobal::set_color4(globtm_no_ofs_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_color4(globtm_no_ofs_psf_3VarId, Color4(globtm[3]));
}


dabfg::NodeHandle mk_frame_data_setup_node()
{
  return dabfg::register_node("frame_data_setup_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");

    auto cameraHndl = registry.createBlob<CameraParams>("current_camera", dabfg::History::No).handle();
    auto sunParamsHndl = registry.createBlob<SunParams>("current_sun", dabfg::History::No).handle();
    auto belowCloudsHndl = registry.createBlob<bool>("below_clouds", dabfg::History::No).handle();
    auto waterLevelHndl = registry.createBlob<float>("water_level", dabfg::History::No).handle();
    auto texCtxHndl = registry.createBlob<TexStreamingContext>("tex_ctx", dabfg::History::No).handle();
    auto occlusionHndl = registry.createBlob<Occlusion *>("current_occlusion", dabfg::History::No).handle();
    auto riVisibilityHndl = registry.createBlob<RiGenVisibility *>("rendinst_main_visibility", dabfg::History::No).handle();
    return [cameraHndl, sunParamsHndl, belowCloudsHndl, waterLevelHndl, texCtxHndl, occlusionHndl, riVisibilityHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      storeCurrentGlobTmNoOffsInShader(wr.currentFrameCamera.viewTm, wr.currentFrameCamera.jitterProjTm);

      d3d::settm(TM_WORLD, TMatrix::IDENT);
      d3d::settm(TM_VIEW, wr.currentFrameCamera.viewTm);

      extern void set_add_lod_bias(float add, const char *name);

      eastl::unique_ptr<AntiAliasing> oldAntiAliasing;
      float cur_mip_scale = wr.getMipmapBias();
      static float last_mip_scale = 0;
      if (last_mip_scale != cur_mip_scale)
      {
        acesfx::set_default_mip_bias(cur_mip_scale);
        set_add_lod_bias(cur_mip_scale, "*");
        set_add_lod_bias(cur_mip_scale, "impostor");
        set_add_lod_bias(0, "character_micro_details*");
        ShaderGlobal::set_real(mip_biasVarId, cur_mip_scale);
        last_mip_scale = cur_mip_scale;
      }

      ShaderGlobal::set_int(sub_pixelsVarId, 1);

      {
        Driver3dPerspective persp = wr.currentFrameCamera.noJitterPersp;
        persp.ox = persp.oy = 0;

        Point2 jitterOffset(0, 0);
        if (wr.antiAliasing)
          jitterOffset = wr.antiAliasing->update(persp);

        // todo: replce that with 'double' version!
        d3d::setpersp(persp, &wr.currentFrameCamera.jitterProjTm);
        wr.currentFrameCamera.jitterPersp = persp;
        wr.currentFrameCamera.jitterGlobtm = TMatrix4(wr.currentFrameCamera.viewTm) * wr.currentFrameCamera.jitterProjTm;
        wr.currentFrameCamera.jitterFrustum = wr.currentFrameCamera.jitterGlobtm;

        wr.updateTransformations(jitterOffset, wr.currentFrameCamera.cameraWorldPos - wr.prevFrameCamera.cameraWorldPos);
      }

      const Point3 camPos = wr.currentFrameCamera.viewItm.getcol(3);

      occlusionHndl.ref() = current_occlusion;
      cameraHndl.ref() = wr.currentFrameCamera;
      Point3 panoramaDirToSun = wr.dir_to_sun.curr;
      auto skies = get_daskies();
      if (skies != nullptr)
        panoramaDirToSun = skies->calcPanoramaSunDir(camPos);
      sunParamsHndl.ref() = {wr.dir_to_sun.curr, wr.sun, panoramaDirToSun};
      riVisibilityHndl.ref() = wr.rendinst_main_visibility;
      texCtxHndl.ref() = wr.currentTexCtx;

      {
        const float water_level = wr.water ? fft_water::get_level(wr.water) : -10000;
        const bool belowClouds = get_daskies() ? max(camPos.y, water_level) < get_daskies()->getCloudsStartAlt() - 10.0f : true;
        waterLevelHndl.ref() = water_level;
        belowCloudsHndl.ref() = belowClouds;
      }
    };
  });
}
