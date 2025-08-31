// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "fomShadowsManager.h"

#include <perfMon/dag_statDrv.h>
#include "global_vars.h"
#include "render/fx/fxRenderTags.h"
#include <math/dag_TMatrix4more.h>
#include <math/dag_viewMatrix.h>
#include <shaders/dag_shaderBlock.h>
#include "render/fx/fxRenderTags.h"
#include "render/fx/fx.h"
#include <shaders/dag_shaderVar.h>

#include <render/world/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <render/rendererFeatures.h>
#include <drv/3d/dag_matricesAndPerspective.h>

static void reset_fom_shadows()
{
  ShaderGlobal::set_color4(fom_shadows_tm_xVarId, 0, 0, 0, -1);
  ShaderGlobal::set_color4(fom_shadows_tm_yVarId, 0, 0, 0, -1);
  ShaderGlobal::set_color4(fom_shadows_tm_zVarId, 0, 0, 0, -1e7);
}

FomShadowsManager::FomShadowsManager(int size, float z_distance, float xy_distance, float height) :
  shadowMapSize(size), zDistance(z_distance), xyViewBoxSize(xy_distance), zViewBoxSize(height)
{
  fomRenderingNode = dafg::register_node("particle_shadow_pass_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    const bool isForward = renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING);
    registry.orderMeAfter(isForward ? "transparent_effects_setup_mobile" : "acesfx_update_node");

    const dafg::Texture2dCreateInfo texCreateInfo = {TEXCF_RTARGET | TEXFMT_A16B16G16R16F, IPoint2(shadowMapSize, shadowMapSize)};
    registry.create("fom_shadows_cos", dafg::History::No)
      .texture(texCreateInfo)
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT)
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));
    registry.create("fom_shadows_sin", dafg::History::No)
      .texture(texCreateInfo)
      .atStage(dafg::Stage::PS)
      .useAs(dafg::Usage::COLOR_ATTACHMENT)
      .clear(make_clear_value(0.f, 0.f, 0.f, 0.f));
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
      smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
      registry.create("fom_shadows_sampler", dafg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    registry.requestRenderPass().color({"fom_shadows_cos", "fom_shadows_sin"});
    registry.requestState().setFrameBlock("global_frame");
    registry.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [this, cameraHndl] {
      ScopeViewProjMatrix scopedViewProj;
      STATE_GUARD(ShaderGlobal::set_color4(world_view_posVarId, VALUE, 1), fomViewPos,
        Point3::rgb(ShaderGlobal::get_color4(world_view_posVarId)));

      // align to fight aliasing
      d3d::settm(TM_PROJ, &fomProj);
      d3d::settm(TM_VIEW, fomView);
      STATE_GUARD(ShaderGlobal::set_int(fx_render_modeVarId, VALUE), FX_RENDER_MODE_FOM, FX_RENDER_MODE_NORMAL);

      TMatrix4 fomTm = TMatrix4(fomView) * fomProj;
      acesfx::set_dafx_globaldata(fomTm, fomView, fomViewPos);

      if (acesfx::renderTransSpecial(ERT_TAG_FOM))
      {
        TMatrix4 texTm = TMatrix4(fomView) * fomProj * screen_to_tex_scale_tm_xy();
        ShaderGlobal::set_color4(fom_shadows_tm_xVarId, texTm.m[0][0], texTm.m[1][0], texTm.m[2][0], texTm.m[3][0]);
        ShaderGlobal::set_color4(fom_shadows_tm_yVarId, texTm.m[0][1], texTm.m[1][1], texTm.m[2][1], texTm.m[3][1]);
        ShaderGlobal::set_color4(fom_shadows_tm_zVarId, texTm.m[0][2], texTm.m[1][2], texTm.m[2][2], texTm.m[3][2]);
      }
      else
      {
        reset_fom_shadows();
      }
      TMatrix4 globtm = TMatrix4(scopedViewProj.getViewTm()) * scopedViewProj.getProjTm();
      const TMatrix &itm = cameraHndl.ref().viewItm;
      acesfx::set_dafx_globaldata(globtm, itm, itm.getcol(3));
    };
  });
}

FomShadowsManager::~FomShadowsManager() { reset_fom_shadows(); }

void FomShadowsManager::prepareFOMShadows(const TMatrix &itm, float worldWaterLevel, float worldMinimumHeight, Point3 dirToSun)
{
  float minLevel = max(worldWaterLevel, worldMinimumHeight);

  Point3 groundPos = Point3::xVz(itm.getcol(3), minLevel);
  fomViewPos = groundPos + dirToSun * (zDistance + zViewBoxSize / dirToSun.y);

  BBox3 worldBox(groundPos - Point3(xyViewBoxSize, 0, xyViewBoxSize),
    groundPos + Point3(xyViewBoxSize, minLevel + zViewBoxSize, xyViewBoxSize));

  TMatrix lightViewITm = TMatrix::IDENT;
  view_matrix_from_look(-dirToSun, lightViewITm);
  fomView = orthonormalized_inverse(lightViewITm);

  const BBox3 spacebox = fomView * worldBox;
  fomProj = matrix_ortho_off_center_lh(spacebox[0].x, spacebox[1].x, spacebox[0].y, spacebox[1].y, spacebox[0].z - zDistance,
    spacebox[1].z + zDistance);

  TMatrix4 viewproj = TMatrix4(fomView) * fomProj;
  TMatrix4 offset = TMatrix4::IDENT;

  // align to fight aliasing
  const float halfTexel = 0.5f * shadowMapSize;
  const float texCoordX = viewproj._41 * halfTexel, texCoordY = viewproj._42 * halfTexel;
  offset._41 = floor(texCoordX) / halfTexel - viewproj._41;
  offset._42 = floor(texCoordY) / halfTexel - viewproj._42;
  fomProj = fomProj * offset;

  d3d::settm(TM_PROJ, &fomProj);
  d3d::settm(TM_VIEW, fomView);
  TMatrix4 globtm = TMatrix4(fomView) * fomProj;
  acesfx::prepare_fom_culling(globtm);
}
