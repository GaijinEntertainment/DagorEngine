// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>
#include <render/world/global_vars.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/private_worldRenderer.h>
#include <render/world/shoreRenderer.h>
#include <main/water.h>
#include <fftWater/fftWater.h>
#include <drv/3d/dag_matricesAndPerspective.h>

static const ShaderVariableInfo mirror_water_reflection_texVarId("water_reflection_tex", true);
static const ShaderVariableInfo mirror_water_reflection_strength_texVarId("water_reflection_strength_tex", true);
static const ShaderVariableInfo mirror_downsampled_close_depth_texVarId("downsampled_close_depth_tex", true);
static const ShaderVariableInfo mirror_water_planar_reflection_cloudsVarId("water_planar_reflection_clouds", true);
static const ShaderVariableInfo mirror_water_rt_enabledVarId("water_rt_enabled", true);

dafg::NodeHandle create_dynamic_mirror_render_water_node()
{
  return get_dynamic_mirrors_namespace().registerNode("render_water", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestRenderPass().depthReadTestAndSample("mirror_gbuf_depth", {"downsampled_far_depth_tex"}).color({"mirror_texture"});
    registry.readBlob<OrderingToken>("mirror_envi_rendered_token");
    registry.requestState().setFrameBlock("global_frame");

    use_volfog(registry, dafg::Stage::PS);

    registry.read("wfx_hmap").texture().atStage(dafg::Stage::VS | dafg::Stage::PS).bindToShaderVar().optional();
    registry.read("wfx_normals").texture().atStage(dafg::Stage::PS).bindToShaderVar().optional();

    auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
    auto mirrorCameraHndl = registry.readBlob<DynamicMirrorRenderer::CameraData>("mirror_camera")
                              .bindAsView<&DynamicMirrorRenderer::CameraData::viewTm>()
                              .bindAsProj<&DynamicMirrorRenderer::CameraData::projTm>()
                              .handle();
    auto shaderVarsHndl = registry.readBlob<DynamicMirrorShaderVars>("mirror_vars").handle();

    return [mirrorActiveHndl, mirrorCameraHndl, shaderVarsHndl]() {
      if (!mirrorActiveHndl.ref() || is_water_hidden())
        return;
      auto worldRenderer = static_cast<WorldRenderer *>(get_world_renderer());
      if (worldRenderer == nullptr)
        return;
      auto water = worldRenderer->getWater();
      if (water == nullptr)
        return;

      SCOPE_VIEW_PROJ_MATRIX;
      const auto cameraData = mirrorCameraHndl.get();
      auto scopedVars = shaderVarsHndl.ref().getScopedVars();

      STATE_GUARD(ShaderGlobal::set_float4(world_view_posVarId, VALUE),
        Color4(cameraData->viewPos.x, cameraData->viewPos.y, cameraData->viewPos.z, 1), ShaderGlobal::get_float4(world_view_posVarId));
      STATE_GUARD(ShaderGlobal::set_float4(zn_zfarVarId, VALUE), Color4(cameraData->persp.zn, cameraData->persp.zf, 0, 0),
        ShaderGlobal::get_float4(zn_zfarVarId));
      // Use envi probe instead of planar clouds
      STATE_GUARD(ShaderGlobal::set_texture(water_refraction_texVarId, VALUE), static_cast<BaseTexture *>(nullptr),
        ShaderGlobal::get_tex_ptr(water_refraction_texVarId.get_var_id()));
      STATE_GUARD(ShaderGlobal::set_texture(mirror_water_reflection_texVarId, VALUE), static_cast<BaseTexture *>(nullptr),
        ShaderGlobal::get_tex_ptr(mirror_water_reflection_texVarId.get_var_id()));
      STATE_GUARD(ShaderGlobal::set_texture(mirror_water_reflection_strength_texVarId, VALUE), static_cast<BaseTexture *>(nullptr),
        ShaderGlobal::get_tex_ptr(mirror_water_reflection_strength_texVarId.get_var_id()));
      STATE_GUARD(ShaderGlobal::set_texture(mirror_downsampled_close_depth_texVarId, VALUE), static_cast<BaseTexture *>(nullptr),
        ShaderGlobal::get_tex_ptr(mirror_downsampled_close_depth_texVarId.get_var_id()));
      STATE_GUARD(ShaderGlobal::set_texture(mirror_water_planar_reflection_cloudsVarId, VALUE), static_cast<BaseTexture *>(nullptr),
        ShaderGlobal::get_tex_ptr(mirror_water_planar_reflection_cloudsVarId.get_var_id()));
      STATE_GUARD(ShaderGlobal::set_int(mirror_water_rt_enabledVarId, VALUE), 0,
        ShaderGlobal::get_int(mirror_water_rt_enabledVarId.get_var_id()));
      const IPoint4 mirrorSize = shaderVarsHndl.ref().gbufferViewSize;
      STATE_GUARD(ShaderGlobal::set_float4(lowres_tex_sizeVarId, VALUE),
        Color4(mirrorSize.x, mirrorSize.y, 2.0f / mirrorSize.x, 2.0f / mirrorSize.y), ShaderGlobal::get_float4(lowres_tex_sizeVarId));
      STATE_GUARD(ShaderGlobal::set_float4(lowres_rt_paramsVarId, VALUE), Color4(mirrorSize.x, mirrorSize.y, 0, 0),
        ShaderGlobal::get_float4(lowres_rt_paramsVarId));

      const ShoreRenderer *shore = worldRenderer->getShore();
      fft_water::render(water, cameraData->viewPos, shore ? shore->getDistanceFieldTexId() : BAD_TEXTUREID, cameraData->frustum,
        nullptr, cameraData->persp);
    };
  });
}
