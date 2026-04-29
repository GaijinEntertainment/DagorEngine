// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>

#include <shaders/dag_postFxRenderer.h>
#include <drv/3d/dag_rwResource.h>

#include <render/renderEvent.h>
#include <render/world/frameGraphHelpers.h>


namespace var
{
static ShaderVariableInfo packed_gbuf_1_tex("packed_gbuf_1_tex");
static int packed_gbuf_1_uav_no = -1;
} // namespace var

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_gbuffer_mixing_nodes_es(const OnCameraNodeConstruction &evt)
{
  bool isNormalsPacked = renderer_has_feature(GBUFFER_PACKED_NORMALS);
  if (!isNormalsPacked)
    return;

  auto ns = dafg::root() / "opaque" / "mixing";
  evt.nodes->push_back(ns.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto prev_ns = registry.root() / "opaque" / "decorations";
    registry.requestRenderPass()
      .color({registry.createTexture2d("unpacked_normals",
        {TEXFMT_A2B10G10R10 | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("main_view")})})
      .depthRoAndBindToShaderVars(prev_ns.rename("gbuf_depth_done", "gbuf_depth").texture(), {"depth_gbuf"});

    prev_ns.rename("gbuf_0_done", "gbuf_0").texture();
    prev_ns.rename("gbuf_2_done", "gbuf_2").texture().optional();
    prev_ns.rename("gbuf_3_done", "gbuf_3").texture().optional();

    auto packed_gbuf_1_handler =
      prev_ns.rename("gbuf_1_done", "gbuf_1_ro").texture().atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    ShaderGlobal::get_int_by_name("packed_gbuf_1_uav_no", var::packed_gbuf_1_uav_no);

    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    registry.create("gbuf_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));

    PostFxRenderer normalUnpacker("normal_unpack");
    return [normalUnpacker, packed_gbuf_1_handler]() {
      ShaderGlobal::set_texture(var::packed_gbuf_1_tex, packed_gbuf_1_handler.get());
      normalUnpacker.render();
      ShaderGlobal::set_texture(var::packed_gbuf_1_tex, nullptr);
      d3d::set_rwtex(STAGE_PS, var::packed_gbuf_1_uav_no, packed_gbuf_1_handler.get(), 0, 0);
    };
  }));

  evt.nodes->push_back(ns.registerNode("token_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createBlob<OrderingToken>("static_decals_rendered");
    registry.createBlob<OrderingToken>("dynamic_decals_rendered");
    registry.createBlob<OrderingToken>("dagdp_decals_rendered");

    registry.createBlob<OrderingToken>("blood_puddles_rendered");
    return []() {};
  }));

  evt.nodes->push_back(ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.rename("gbuf_0", "gbuf_0_done").texture();
    registry.rename("gbuf_2", "gbuf_2_done").texture().optional();
    registry.rename("gbuf_depth", "gbuf_depth_done").texture();
    d3d::set_rwtex(STAGE_PS, var::packed_gbuf_1_uav_no, nullptr, 0, 0);
  }));
}
