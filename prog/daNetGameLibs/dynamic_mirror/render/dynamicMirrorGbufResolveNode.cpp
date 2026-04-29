// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dynamicMirrorNodes.h"
#include "dynamicMirrorRenderer.h"

#include <render/daFrameGraph/daFG.h>
#include <EASTL/shared_ptr.h>
#include <render/deferredRenderer.h>

dafg::NodeHandle create_dynamic_mirror_gbuf_resolve_node(const char *resolve_pshader_name)
{
  return get_dynamic_mirrors_namespace().registerNode("resolve_gbuf", DAFG_PP_NODE_SRC,
    [resolve_pshader_name](dafg::Registry registry) {
      const auto mirrorResolution = get_mirror_resolution(registry);

      auto resultTextureHandle = registry.create("mirror_texture")
                                   .texture({TEXFMT_R11G11B10F | TEXCF_RTARGET, mirrorResolution})
                                   .atStage(dafg::Stage::PS)
                                   .useAs(dafg::Usage::COLOR_ATTACHMENT)
                                   .handle();

      registry.read("mirror_gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf");
      registry.read("mirror_gbuf_0").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("albedo_gbuf");
      registry.read("mirror_gbuf_1").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("normal_gbuf");
      registry.read("mirror_gbuf_2").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("material_gbuf").optional();
      registry.read("mirror_gbuf_3").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("motion_gbuf").optional();
      registry.read("gbuf_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("depth_gbuf_samplerstate")
        .bindToShaderVar("albedo_gbuf_samplerstate")
        .bindToShaderVar("normal_gbuf_samplerstate")
        .bindToShaderVar("material_gbuf_samplerstate")
        .bindToShaderVar("motion_gbuf_samplerstate")
        .optional();

      registry.requestState().setFrameBlock("global_frame");

      auto mirrorActiveHndl = registry.readBlob<bool>("is_mirror_active").handle();
      auto mirrorCameraHndl = registry.readBlob<DynamicMirrorRenderer::CameraData>("mirror_camera").handle();

      return [mirrorActiveHndl, mirrorCameraHndl, shadingResolver = eastl::make_unique<ShadingResolver>(resolve_pshader_name),
               resultTextureHandle]() {
        if (!mirrorActiveHndl.ref())
          return;
        const auto cameraData = mirrorCameraHndl.get();
        auto view = resultTextureHandle.view();
        shadingResolver->resolve(view.getBaseTex(), cameraData->viewTm, cameraData->projTm, nullptr, ShadingResolver::ClearTarget::No,
          TMatrix4::IDENT, nullptr);
      };
    });
}
