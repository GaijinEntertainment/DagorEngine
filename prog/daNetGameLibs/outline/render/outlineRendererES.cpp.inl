// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <outline/outlineCommon.h>

#include <EASTL/sort.h>
#include <EASTL/vector.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/anim/anim.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_postFxRenderer.h>
#include <math/dag_hlsl_floatx.h>
#include <rendInst/rendInstExtra.h>
#include <gameRes/dag_collisionResource.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstCollision.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>

#include <scene/dag_occlusion.h>
#include <perfMon/dag_statDrv.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/rendInst/riExtra.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/dynModelRenderer.h>
#include <render/renderEvent.h>
#include <game/gameEvents.h>

#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <render/resolution.h>

#include <daECS/core/utility/enumRegistration.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_shaderConstants.h>

extern ShaderBlockIdHolder dynamicDepthSceneBlockId;
extern ShaderBlockIdHolder rendinstDepthSceneBlockId;
using namespace dynmodel_renderer;

static void create_outline_rendrerer_locally_es(const BeforeLoadLevel &, ecs::EntityManager &manager)
{
  manager.getOrCreateSingletonEntity(ECS_HASH("outline_renderer"));
}

ECS_ON_EVENT(on_appear)
static void outline_renderer_create_es_event_handler(
  const ecs::Event &, OutlineRenderer &outline_renderer, float outline_blur_width, float outline_brightness)
{
  outline_renderer.init(outline_brightness);

  int targetWidth, targetHeight;
  get_rendering_resolution(targetWidth, targetHeight);
  outline_renderer.changeResolution(targetWidth, targetHeight);

  int displayWidth, displayHeight;
  get_display_resolution(displayWidth, displayHeight);
  outline_renderer.initBlurWidth(outline_blur_width, displayWidth, displayHeight);
}

// ri destruction is synced earlier than entity destruction
// as a result the game tries to render outline of a destroyed ri and crashes
// So prevent outline rendering as soon as the client learns that ri is destroyed.
ECS_TAG(render)
static inline void disable_outline_on_ri_destroyed_es_event_handler(const EventRiExtraDestroyed &, bool &outline__enabled)
{
  outline__enabled = false;
}

ECS_AFTER(animchar_before_render_es) // require for execute animchar_before_render_es as early as possible
static void outline_prepare_es(const UpdateStageInfoBeforeRender &stg, OutlineContexts &outline_ctxs)
{
  outline_update_contexts(outline_ctxs, stg.globTm, stg.mainOcclusion);
}

static void rendist_rendering(UniqueBuf &rendinstMatrixBuffer, int res_idx, const TMatrix &transform)
{
  Point4 data[4];
  data[0] = Point4(transform.getcol(0).x, transform.getcol(1).x, transform.getcol(2).x, transform.getcol(3).x),
  data[1] = Point4(transform.getcol(0).y, transform.getcol(1).y, transform.getcol(2).y, transform.getcol(3).y),
  data[2] = Point4(transform.getcol(0).z, transform.getcol(1).z, transform.getcol(2).z, transform.getcol(3).z);
  data[3] = Point4(0, 0, 0, 0); // we need zero vector to avoid executing this branch if (useCbufferParams)

  rendinstMatrixBuffer->updateData(0, sizeof(Point4) * 4U, &data, VBLOCK_WRITEONLY | VBLOCK_DISCARD);

  IPoint2 offsAndCnt(0, 1);
  uint16_t riIdx = res_idx;
  uint32_t zeroLodOffset = 0;
  SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);

  rendinst::render::renderRIGenExtraFromBuffer(rendinstMatrixBuffer.getBuf(), dag::ConstSpan<IPoint2>(&offsAndCnt, 1),
    dag::ConstSpan<uint16_t>(&riIdx, 1), dag::ConstSpan<uint32_t>(&zeroLodOffset, 1), rendinst::RenderPass::Depth,
    rendinst::OptimizeDepthPass::No, rendinst::OptimizeDepthPrepass::No, rendinst::IgnoreOptimizationLimits::No,
    rendinst::LayerFlag::Stages);
}

static void render_outline_stencil(const TMatrix &view_tm,
  dag::Vector<OutlinedAnimchar> &animcharElements,
  DynModelRenderingState &state,
  StencilColorState &stencil,
  ColorBuffer &colors)
{
  if (animcharElements.empty())
    return;
  TIME_D3D_PROFILE(outlined_animchars);
  TMatrix vtm = view_tm;
  vtm.col[3] = Point3(0, 0, 0);
  d3d::settm(TM_VIEW, vtm);

  render_outline_elements(
    animcharElements, stencil, colors,
    [&state](const OutlinedAnimchar &e) {
      state.process_animchar(0, ShaderMesh::STG_imm_decal, e.animchar->getSceneInstance(),
        animchar_additional_data::get_optional_data(e.additionalData), false);
    },
    [&state]() {
      state.prepareForRender();
      const DynamicBufferHolder *buffer = state.requestBuffer(BufferType::OTHER);
      if (buffer)
      {
        state.setVars(buffer->buffer.getBufId());
        SCENE_LAYER_GUARD(dynamicDepthSceneBlockId);
        state.render(buffer->curOffset);
      }
      state.clear();
    });
}

static void render_outline_stencil(const TMatrix &view_tm,
  dag::Vector<OutlinedRendinst> &riElements,
  UniqueBuf &rend_inst_transforms,
  StencilColorState &stencil_state,
  ColorBuffer &colors)
{
  if (riElements.empty())
    return;

  // We can optimize RI rendering with instancing
  // Need to gather all RI with same color in buffer and slices and pass to renderRIGenExtraFromBuffer
  TIME_D3D_PROFILE(outlined_rendinst);
  d3d::settm(TM_VIEW, view_tm);

  render_outline_elements(
    riElements, stencil_state, colors,
    [&rend_inst_transforms](const OutlinedRendinst &ri) { rendist_rendering(rend_inst_transforms, ri.riIdx, ri.transform); }, []() {});
}

void outline_render_elements(
  OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs, const TMatrix &view_tm, const TMatrix4_vec4 &proj_tm)
{
  StencilColorState stencilState;
  DynModelRenderingState &state = dynmodel_renderer::get_immediate_state();
  auto renderOutlineType = [&](int override_type) {
    auto &context = outline_ctxs.context[override_type];
    if (context.empty())
      return;
    shaders::overrides::set(outline_renderer.zTestOverrides[override_type]);
    render_outline_stencil(view_tm, context.animcharElements, state, stencilState, outline_renderer.colors);
    render_outline_stencil(view_tm, context.riElements, outline_renderer.rendInstTransforms, stencilState, outline_renderer.colors);
    shaders::overrides::reset();
  };

  {
    SCOPE_VIEW_PROJ_MATRIX;
    TMatrix4 nproj = proj_tm * outline_renderer.tm;
    d3d::settm(TM_PROJ, &nproj);

    outline_renderer.resetColorBuffer();
    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

    renderOutlineType(NO_ZTEST);
    renderOutlineType(ZTEST_PASS);
    renderOutlineType(ZTEST_FAIL); // If self-occluded regions of an object pass back-face culling we can have bad outlines. We can
                                   // render Z-fail context with Z-pass override and a "magic" stencil value before this, in order to
                                   // discard outline on those regions.

    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
  }
}

template <typename Callable>
static inline void render_outline_ecs_query(Callable fn);

ECS_TAG(render)
static void outline_render_resolution_es_event_handler(const SetResolutionEvent &event, OutlineRenderer &outline_renderer)
{
  int width = event.renderingResolution.x, height = event.renderingResolution.y;
  outline_renderer.changeResolution(width, height);
}

#define TRANSPARENCY_NODE_PRIORITY_OUTLINE_APPLY 5
ECS_ON_EVENT(on_appear, ChangeRenderFeatures)
static void create_outline_node_es(const ecs::Event &evt, dafg::NodeHandle &outline_prepare_node, dafg::NodeHandle &outline_apply_node)
{
  if (auto *changedFeatures = evt.cast<ChangeRenderFeatures>())
  {
    if (!changedFeatures->isFeatureChanged(CAMERA_IN_CAMERA))
      return;
  }

  auto nodeNs = dafg::root() / "transparent" / "close";
  outline_prepare_node = nodeNs.registerNode("outline_prepare_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    bool isForward = renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING);
    registry.read(isForward ? "srv_depth_for_transparency" : "opaque_depth_with_water")
      .texture()
      .atStage(dafg::Stage::PS)
      .bindToShaderVar("depth_gbuf");

    auto outlineDepth = registry.create("outline_depth", dafg::History::No)
                          .texture({TEXFMT_DEPTH24 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                          .clear(make_clear_value(0.f, 0));

    registry.requestRenderPass().depthRw(outlineDepth);

    auto outlineDepthHndl = eastl::move(outlineDepth).atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::DEPTH_ATTACHMENT).handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");


    return [cameraHndl, outlineDepthHndl] {
      render_outline_ecs_query([&](OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs) {
        if (!outline_ctxs.anyVisible())
          return;

        const auto &camera = cameraHndl.ref();
        shaders::overrides::reset();

        outline_prepare(outline_renderer, outline_ctxs, outlineDepthHndl.view().getTex2D(), camera.viewTm, camera.jitterProjTm);
      });
    };
  });

  outline_apply_node = nodeNs.registerNode("outline_apply_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    request_common_transparent_state(registry);
    registry.setPriority(TRANSPARENCY_NODE_PRIORITY_OUTLINE_APPLY);

    auto outlineDepthHndl =
      registry.read("outline_depth").texture().atStage(dafg::Stage::PS).bindToShaderVar("outline_depth").handle();
    registry.create("outline_depth_sampler", dafg::History::No)
      .blob(d3d::request_sampler({}))
      .bindToShaderVar("outline_depth_samplerstate");

    read_gbuffer_material_only(registry);

    return [outlineDepthHndl](const dafg::multiplexing::Index &multiplexing_index) {
      if (multiplexing_index.subCamera > 0)
        return;

      camera_in_camera::RenderMainViewOnly camcam{{}};

      render_outline_ecs_query([&](OutlineRenderer &outline_renderer, OutlineContexts &outline_ctxs) {
        if (!outline_ctxs.anyVisible())
          return;

        SCOPE_VIEWPORT; // Workaround for FG not tracking viewport
        outline_apply(outline_renderer, outlineDepthHndl.view().getTex2D());
      });
    };
  });
}
