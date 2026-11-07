// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include <main/appProfile.h>
#include <render/world/frameGraphHelpers.h>
#include "frameGraphNodes.h"
#include <daEditorE/editorCommon/inGameEditor.h>
#include <render/daFrameGraph/daFG.h>
#include <3d/dag_render.h>
#include <EASTL/fixed_vector.h>
#include <render/cameraParams.h>
#include <render/world/shadowsManager.h>
#include <render/lights/clusteredLights.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include "render/renderEvent.h"
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <3d/dag_lowLatency.h>
#include <debug/dag_debug3d.h>
#include <drv/3d/dag_renderStates.h>
#include <shaders/dag_shaderBlock.h>
#include "game/player.h"
#include <gamePhys/phys/rendinstDestr.h>
#include <render/world/global_vars.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <gui/dag_stdGuiRender.h>
#include "render/world/wrDispatcher.h"
#include "render/debugLightProbeShapeRenderer.h"
#include "render/debugLightProbeSpheres.h"
#include <frustumCulling/frustumPlanes.h>
#include "render/indoorProbeManager.h"
#include <render/debugBoxRenderer.h>
#include <render/raytracedToroidalStaticShadow/raytracedToroidalStaticShadow_api.h>
#include <render/world/private_worldRenderer.h>
#include <rendInst/rendInstGenRender.h>
#include <ecs/render/updateStageRender.h>
#include <landMesh/lmeshRenderer.h>
#include <landMesh/lmeshManager.h>

CONSOLE_BOOL_VAL("occlusion", debug_occlusion, false);
CONSOLE_BOOL_VAL("render", debug_lights, false);
CONSOLE_BOOL_VAL("render", debug_lights_bboxes, false);
CONSOLE_BOOL_VAL("render", debug_light_probe_mirror_sphere, false);
CONSOLE_INT_VAL("render", show_hero_bbox, 0, 0, 2, "1 - weapons only, 2 - weapons+vehicle");
CONSOLE_BOOL_VAL("ridestr", show_ri_phys_bboxes, false);
CONSOLE_INT_VAL("render", show_shadow_occlusion_bboxes, -1, -1, 1);
CONSOLE_INT_VAL("render", show_static_shadow_mesh, 0, 0, 4, "rtss: 1 visibility, 2 level, 3 slot bit, 4 fallback");
CONSOLE_INT_VAL("render", show_static_shadow_frustums, -1, -1, RTSS_MAX_LEVELS - 1, "rtss: the atlas of this level");
CONSOLE_BOOL_VAL("render", show_debug_light_probe_shapes, false);
#if _TARGET_PC_TOOLS_BUILD
CONSOLE_BOOL_VAL("render", show_wireframe_overlay, false);
extern void render_late_trans(const TMatrix &view_tm);
extern void render_late_trans_cockpit(const TMatrix &view_tm);
#endif

extern void debug_draw_shadow_occlusion_bboxes(const Point3 &dir_from_sun, bool final_extend);
eastl::unique_ptr<DebugBoxRenderer> debugBoxRenderer;
eastl::unique_ptr<DebugLightProbeShapeRenderer> debugShapeRenderer;

namespace var
{
static ShaderVariableInfo source_depth_for_copy_const_no("source_depth_for_copy_const_no");
}

static dafg::NodeHandle makeUpsampleDepthForDebugNode()
{
  return dafg::register_node("upsample_depth_for_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto downsampledDepthHndl =
      registry.readTexture("depth_for_postfx").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    registry.requestRenderPass().depth(
      registry.createTexture2d("depth_for_debug", {get_gbuffer_depth_format() | TEXCF_RTARGET, registry.getResolution<2>("display")}));

    return [downsampledDepthHndl, renderer = PostFxRenderer("copy_depth")] {
      d3d::settex(var::source_depth_for_copy_const_no.get_int(), downsampledDepthHndl.get());
      d3d::set_sampler(STAGE_PS, var::source_depth_for_copy_const_no.get_int(), d3d::request_sampler({}));
      renderer.render();
      d3d::settex(var::source_depth_for_copy_const_no.get_int(), nullptr);
    };
  });
}

static const char *depthTexNameForDebug() { return WRDispatcher::isUpsampling() ? "depth_for_debug" : "depth_for_postfx"; }

static void setupTargetForDebug(dafg::Registry registry)
{
  registry.requestRenderPass().color({"target_for_debug"}).depthReadTestOnly(depthTexNameForDebug());
}

void makeDebugVisualizationNodes(eastl::vector<dafg::NodeHandle> &fg_node_handles)
{
  auto ns = dafg::root() / "debug";

  if (WRDispatcher::isUpsampling())
    fg_node_handles.emplace_back(makeUpsampleDepthForDebugNode());

#if DAGOR_DBGLEVEL > 0
  // low latency node
  fg_node_handles.emplace_back(ns.registerNode("low_latency", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("occlusion");
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] { lowlatency::render_debug_low_latency(); };
  }));
#endif

  // debug occlusion node
  fg_node_handles.emplace_back(ns.registerNode("occlusion", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    auto cameraHndl = prevNs.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [cameraHndl] {
      if (debug_occlusion.get())
      {
        CameraViewVisibilityMgr *vis_mgr = cameraHndl.ref().jobsMgr;
        Occlusion *occlusion = vis_mgr->getOcclusion();

        if (::grs_draw_wire)
          d3d::setwire(0);
        // if (occlusion_map)
        //   occlusion_map->debugRender();
        begin_draw_cached_debug_lines(false, false);
        for (int i = 0; i < occlusion->getDebugNotOccludedBoxes().size(); ++i)
        {
          BBox3 box(*(Point3 *)&occlusion->getDebugNotOccludedBoxes()[i].bmin,
            *(Point3 *)&occlusion->getDebugNotOccludedBoxes()[i].bmax);
          draw_cached_debug_box(box, 0xFF1010FF);
        }
        for (int i = 0; i < occlusion->getDebugOccludedBoxes().size(); ++i)
        {
          BBox3 box(*(Point3 *)&occlusion->getDebugOccludedBoxes()[i].bmin, *(Point3 *)&occlusion->getDebugOccludedBoxes()[i].bmax);
          draw_cached_debug_box(box, 0xFFFF1010);
        }
        end_draw_cached_debug_lines();

        IPoint2 current_block;

        current_block[0] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        current_block[1] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
        {
          StdGuiRender::ScopeStarter strt;
          StdGuiRender::goto_xy(Point2(50, 800));
          occlusion->setCountOccludees(true);
          int total =
            occlusion->getOccludedObjectsCount() + occlusion->getVisibleObjectsCount() + occlusion->getFrustumCulledObjectsCount();
          String str(128, "%s; SW_occluders: %dbox, %d quads; Occluded = %d(%f%%), culled = %d(%f%%) total=%d",
            occlusion->hasGPUFrame() ? "use gpu depth buffer" : "", occlusion->getRasterizedBoxOccluders(),
            occlusion->getRasterizedQuadOccluders(), occlusion->getOccludedObjectsCount(),
            float(occlusion->getOccludedObjectsCount() + 1) * 100.f / (total + 1), occlusion->getFrustumCulledObjectsCount(),
            float(occlusion->getFrustumCulledObjectsCount() + 1) * 100.f / (total + 1), total);
          StdGuiRender::set_color(0xFFFF00FF);
          StdGuiRender::draw_str(str.str());
        }
        ShaderGlobal::setBlock(current_block[0], ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(current_block[1], ShaderGlobal::LAYER_SCENE);
        if (::grs_draw_wire)
          d3d::setwire(1);
      }
      else
      {
        if (auto *occlusion = cameraHndl.ref().jobsMgr->getOcclusion())
          occlusion->setCountOccludees(false);
      }
    };
  }));

  // debug lights
  fg_node_handles.emplace_back(ns.registerNode("lights", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("occlusion");
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      auto &lights = WRDispatcher::getClusteredLights();
      if (debug_lights.get())
      {
        if (::grs_draw_wire)
          d3d::setwire(0);
        IPoint2 current_block;

        current_block[0] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
        current_block[1] = ShaderGlobal::getBlock(ShaderGlobal::LAYER_SCENE);
        ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

        lights.renderDebugLights();

        {
          StdGuiRender::ScopeStarter strt;
          StdGuiRender::goto_xy(60, 50);
          StdGuiRender::draw_str(
            String(64, "visibleOmniLights = %d(%d clustered) in visibleSpotLights = %d(%d clustered)", lights.getVisibleOmniCount(),
              lights.getVisibleClusteredOmniCount(), lights.getVisibleSpotsCount(), lights.getVisibleClusteredSpotsCount()));
        }
        ShaderGlobal::setBlock(current_block[0], ShaderGlobal::LAYER_FRAME);
        ShaderGlobal::setBlock(current_block[1], ShaderGlobal::LAYER_SCENE);
        if (::grs_draw_wire)
          d3d::setwire(1);
      }
    };
  }));

  // debug light bboxes node
  fg_node_handles.emplace_back(ns.registerNode("light_bboxes_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("lights");
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      auto &lights = WRDispatcher::getClusteredLights();
      if (debug_lights_bboxes)
        lights.renderDebugLightsBboxes();
    };
  }));

  // debug light probe node
  fg_node_handles.emplace_back(ns.registerNode("light_probe_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("light_bboxes_node");
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    DebugLightProbeSpheres debugMirrorSphere("debug_mirror_sphere");
    debugMirrorSphere.setSpheresCount(1);
    return [mirrorSphere = eastl::move(debugMirrorSphere)] {
      if (debug_light_probe_mirror_sphere.get())
      {
        TIME_D3D_PROFILE(debug_mirror_sphere);
        mirrorSphere.render();
      }
    };
  }));

  // debug collision & GI node
  fg_node_handles.emplace_back(ns.registerNode("collision_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("light_probe_node");
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      auto wr = static_cast<WorldRenderer *>(get_world_renderer());
      wr->renderDebugCollisionDensity();
    };
  }));

  // debug collision & GI node
  fg_node_handles.emplace_back(ns.registerNode("gi_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("collision_node");
    // the GI debug shaders sample the depth they test against; daFG allows both only through this call
    registry.requestRenderPass().color({"target_for_debug"}).depthReadTestAndSample(depthTexNameForDebug(), {"depth_gbuf"});
    registry.requestState().setFrameBlock("global_frame");

    auto prevNs = registry.root();
    auto cameraHndl = prevNs.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl] {
      auto wr = static_cast<WorldRenderer *>(get_world_renderer());
      wr->drawGIDebug(cameraHndl.ref().jitterFrustum);
    };
  }));

  // debug boxes node
  fg_node_handles.emplace_back(ns.registerNode("boxes_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("gi_node");
    setupTargetForDebug(registry);
    auto prevNs = registry.root();
    auto cameraHndl = prevNs.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [cameraHndl] {
      if (debugBoxRenderer)
      {
        CameraViewVisibilityMgr *vis_mgr = cameraHndl.ref().jobsMgr;
        debugBoxRenderer->render(vis_mgr->getRiMainVisibility(), cameraHndl.ref().cameraWorldPos);
      }
    };
  }));

  // debug light probe shapes node
  fg_node_handles.emplace_back(ns.registerNode("light_probe_shapes_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("boxes_node");
    setupTargetForDebug(registry);

    auto prevNs = registry.root();
    auto cameraHndl = prevNs.readBlob<CameraParams>("current_camera")
                        .bindAsView<&CameraParams::viewTm>()
                        .bindAsProj<&CameraParams::jitterProjTm>()
                        .handle();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [cameraHndl] {
      if (show_debug_light_probe_shapes.get())
      {
        if (!debugShapeRenderer)
        {
          auto shapesContainer = WRDispatcher::getIndoorProbeManager()->getIndoorProbeScenes();
          debugShapeRenderer = eastl::make_unique<DebugLightProbeShapeRenderer>(eastl::move(shapesContainer));
        }
        auto camera = cameraHndl.ref();
        auto visibilityMgr = camera.jobsMgr;
        debugShapeRenderer->render(visibilityMgr->getRiMainVisibility(), camera.cameraWorldPos);
      }
    };
  }));

  // show hero node
  fg_node_handles.emplace_back(ns.registerNode("show_hero_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("light_probe_shapes_node");
    setupTargetForDebug(registry);

    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      if (show_hero_bbox.get())
      {
        const bool weaponsOnly = show_hero_bbox.get() == 1;
        QueryHeroWtmAndBoxForRender hero_data(weaponsOnly);
        if (auto heroEid = game::get_controlled_hero())
          if (g_entity_mgr->getEntityTemplateId(heroEid) != ecs::INVALID_TEMPLATE_INDEX)
            g_entity_mgr->sendEventImmediate(heroEid, hero_data);
        if (hero_data.resReady)
        {
          begin_draw_cached_debug_lines(false, false);
          TMatrix wtm = hero_data.resWtm;
          wtm.col[3] += hero_data.resWofs;
          set_cached_debug_lines_wtm(wtm);
          draw_cached_debug_box(hero_data.resLbox, E3DCOLOR(255, 0, 0));

          end_draw_cached_debug_lines();
          set_cached_debug_lines_wtm(TMatrix::IDENT);
        }
      }
    };
  }));

  // debug RI bboxes node
  fg_node_handles.emplace_back(ns.registerNode("ri_bboxes_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("show_hero_node");
    setupTargetForDebug(registry);

    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      if (show_ri_phys_bboxes.get())
      {
        rendinstdestr::debug_draw_ri_phys();
      }
    };
  }));

  // shadow occlusion bboxes node
  fg_node_handles.emplace_back(ns.registerNode("shadow_occlusion_bboxes_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("ri_bboxes_node");
    setupTargetForDebug(registry);

    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      if (show_shadow_occlusion_bboxes.get() >= 0)
      {
        auto &info = WRDispatcher::getShadowInfoProvider();
        debug_draw_shadow_occlusion_bboxes(-info.getDirToSun(IShadowInfoProvider::DirToSunType::CSM),
          show_shadow_occlusion_bboxes.get() == 1);
      }
    };
  }));

  // static shadow debug node
  fg_node_handles.emplace_back(ns.registerNode("static_shadow_debug_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("shadow_occlusion_bboxes_node");
    registry.requestRenderPass().color({"target_for_debug"}).depthReadTestAndSample(depthTexNameForDebug(), {"depth_gbuf"});

    auto prevNs = registry.root();
    CameraViewShvars{prevNs.readBlob<CameraParams>("current_camera")}.bindViewVecs();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      WRDispatcher::getShadowsManager().debugRenderStaticShadows(show_static_shadow_mesh.get(), show_static_shadow_frustums.get());
    };
  }));

  // wireframe overlay nodes
#if _TARGET_PC_TOOLS_BUILD
  fg_node_handles.emplace_back(ns.registerNode("render_wireframe_overlay_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("static_shadow_debug_node");

    auto prevNs = registry.root();
    auto cameraHndl = prevNs.readBlob<CameraParams>("current_camera").handle();
    registry.createTexture2d("wireframe_tex", {TEXFMT_R8 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
      .clear(make_clear_value(1.0f, 1.0f, 1.0f, 1.0f));
    registry.requestRenderPass().color({"wireframe_tex"}).depthReadTestOnly("depth_for_postfx");
    registry.requestState().setFrameBlock("global_frame");

    shaders::OverrideState state;
    state.set(shaders::OverrideState::Z_BIAS);
    state.zBias = 0.1f;
    state.set(shaders::OverrideState::BLEND_SRC_DEST);
    state.sblend = BLEND_ZERO;
    state.dblend = BLEND_ZERO;

    return [cameraHndl, wireframeStateId = shaders::overrides::create(state),
             use_atestVarId = ::get_shader_variable_id("use_atest", true)] {
      if (!show_wireframe_overlay.get())
        return;

      TIME_D3D_PROFILE(wireframe_overlay);

      const auto &camera = cameraHndl.ref();

      d3d::setwire(1);
      shaders::overrides::set(wireframeStateId);
      const bool useDepthPrepass = rendinst::render::useRiDepthPrepass(false);
      const int useATest = ShaderGlobal::get_int(use_atestVarId);
      ShaderGlobal::set_int(use_atestVarId, 0);

      const Point3 cameraPos = camera.viewItm.getcol(3);
      auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
      wr->renderStaticSceneOpaque(RENDER_MAIN, cameraPos, camera.viewItm, camera.noJitterFrustum, nullptr);
      wr->renderDynamicOpaque(RENDER_MAIN, camera.viewItm, camera.viewTm, camera.jitterProjTm, cameraPos);
      if (auto *vis = wr->getMainCameraRiMainVisibility())
      {
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, vis, camera.viewItm, rendinst::LayerFlag::NotExtra,
          rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, wr->getTexCtx());
        rendinst::render::renderRIGen(rendinst::RenderPass::Normal, vis, camera.viewItm, rendinst::LayerFlag::Transparent,
          rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, nullptr, wr->getTexCtx());
      }
      if (auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer())
      {
        if (auto *lmeshMgr = WRDispatcher::getLandMeshManager())
        {
          LandMeshRenderDesc desc;
          desc.invGeomLodDist = lmeshRenderer->getInvGeomLodDist();
          lmeshRenderer->render(*lmeshMgr, LandMeshRenderer::RENDER_WITH_CLIPMAP, desc, cameraPos, HmapOrigin(cameraPos));
        }
      }
      if (g_entity_mgr)
      {
        g_entity_mgr->broadcastEventImmediate(UpdateStageInfoRenderTrans(camera.viewTm, camera.jitterProjTm, camera.viewItm,
          wr->getMainCameraOcclusion(), wr->getTexCtx()));
        g_entity_mgr->broadcastEventImmediate(RenderLateTransEvent(camera.viewTm, camera.cameraWorldPos, wr->getTexCtx(), nullptr));
        g_entity_mgr->broadcastEventImmediate(
          UpdateStageInfoRenderDistortion(camera.viewTm, camera.jitterProjTm, camera.viewItm, wr->getTexCtx()));
      }
      render_late_trans(camera.viewTm);
      render_late_trans_cockpit(camera.viewTm);

      ShaderGlobal::set_int(use_atestVarId, useATest);
      rendinst::render::useRiDepthPrepass(useDepthPrepass);
      shaders::overrides::reset();
      if (!::grs_draw_wire)
        d3d::setwire(0);
    };
  }));

  fg_node_handles.emplace_back(ns.registerNode("composite_wireframe_overlay_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readTexture("wireframe_tex").atStage(dafg::Stage::PS).bindToShaderVar("wireframe_tex");
    registry.requestRenderPass().color({"target_for_debug"});
    return [applyWireframe = PostFxRenderer("apply_wireframe")]() {
      if (!show_wireframe_overlay.get())
        return;
      applyWireframe.render();
    };
  }));
#endif

#if DAGOR_DBGLEVEL == 0 && _TARGET_PC
  if (has_in_game_editor())
#elif DAGOR_DBGLEVEL == 0
  if constexpr (false)
#endif
    fg_node_handles.emplace_back(makeShowSceneDebugNode());
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_debug_render_nodes_es(const OnCameraNodeConstruction &evt)
{
#if DAGOR_DBGLEVEL == 0
#if _TARGET_PC
  bool requestDebugNodes = app_profile::get().devMode;
#else
  constexpr bool requestDebugNodes = false;
#endif
#else
  constexpr bool requestDebugNodes = true;
#endif

  auto debugNs = dafg::root() / "debug";

  evt.nodes->push_back(debugNs.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto prevNs = registry.root();
    prevNs.renameTexture("frame_after_postfx", "target_for_debug");
  }));

  if (requestDebugNodes)
    makeDebugVisualizationNodes(*evt.nodes);

  evt.nodes->push_back(dafg::register_node("debug_visualization_publish_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto prevNs = registry.root() / "debug";
    prevNs.rename("target_for_debug", "target_after_debug").texture();
  }));
}
