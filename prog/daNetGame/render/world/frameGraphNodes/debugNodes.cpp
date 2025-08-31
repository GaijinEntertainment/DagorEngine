// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include <render/world/frameGraphHelpers.h>
#include <render/daFrameGraph/daFG.h>
#include <3d/dag_render.h>
#include <EASTL/fixed_vector.h>
#include <render/world/cameraParams.h>
#include <render/world/shadowsManager.h>
#include <render/clusteredLights.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include "render/renderEvent.h"
#include <ecs/core/entityManager.h>
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
#include <render/world/private_worldRenderer.h>

CONSOLE_BOOL_VAL("occlusion", debug_occlusion, false);
CONSOLE_BOOL_VAL("render", debug_lights, false);
CONSOLE_BOOL_VAL("render", debug_lights_bboxes, false);
CONSOLE_BOOL_VAL("render", debug_light_probe_mirror_sphere, false);
CONSOLE_INT_VAL("render", show_hero_bbox, 0, 0, 2, "1 - weapons only, 2 - weapons+vehicle");
CONSOLE_BOOL_VAL("ridestr", show_ri_phys_bboxes, false);
CONSOLE_INT_VAL("render", show_shadow_occlusion_bboxes, -1, -1, 1);
CONSOLE_BOOL_VAL("render", show_static_shadow_mesh, false);
CONSOLE_BOOL_VAL("render", show_static_shadow_frustums, false);
CONSOLE_BOOL_VAL("render", show_debug_light_probe_shapes, false);
CONSOLE_FLOAT_VAL_MINMAX("render", static_shadow_mesh_range, 0.5f, 0.0001f, 1.0f);

extern void debug_draw_shadow_occlusion_bboxes(const Point3 &dir_from_sun, bool final_extend);
eastl::unique_ptr<DebugBoxRenderer> debugBoxRenderer;
eastl::unique_ptr<DebugLightProbeSpheres> debugLightProbeMirrorSphere;
eastl::unique_ptr<DebugLightProbeShapeRenderer> debugShapeRenderer;
DynamicShaderHelper static_shadow_debug_mesh_shader;

static void setupTargetForDebug(dafg::Registry registry)
{
  registry.requestRenderPass().color({"target_for_debug"}).depthRo("depth_for_transparency");
}

dafg::NodeHandle makeEmptyDebugVisualizationNode()
{
  return dafg::register_node("empty_debug_node", DAFG_PP_NODE_SRC,
    [from = renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING) ? "target_after_under_water_fog" : "target_for_transparency"](
      dafg::Registry registry) { registry.rename(from, "target_after_debug", dafg::History::No); });
}

void makeDebugVisualizationNodes(eastl::vector<dafg::NodeHandle> &fg_node_handles)
{
  auto ns = dafg::root() / "debug";

  fg_node_handles.emplace_back(ns.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto prevNs = registry.root();
    if (renderer_has_feature(FeatureRenderFlags::FORWARD_RENDERING))
    {
      prevNs.renameTexture("target_after_under_water_fog", "target_for_debug", dafg::History::No);
    }
    else
    {
      prevNs.renameTexture("target_for_transparency", "target_for_debug", dafg::History::No);
    }
  }));

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
        const Occlusion *occlusion = vis_mgr->getOcclusion();

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

    return [] {
      if (debug_light_probe_mirror_sphere.get())
      {
        if (!debugLightProbeMirrorSphere)
        {
          debugLightProbeMirrorSphere = eastl::make_unique<DebugLightProbeSpheres>("debug_mirror_sphere");
          debugLightProbeMirrorSphere->setSpheresCount(1);
        }
        TIME_D3D_PROFILE(debug_mirror_sphere);
        debugLightProbeMirrorSphere->render();
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
    setupTargetForDebug(registry);
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

  // static shadow mesh node
  fg_node_handles.emplace_back(ns.registerNode("static_shadow_mesh_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("shadow_occlusion_bboxes_node");
    setupTargetForDebug(registry);

    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return []() mutable {
      if (show_static_shadow_mesh.get())
      {
        float samplingRange = static_shadow_mesh_range.get();
        ShadowsManager &shadowsManager = WRDispatcher::getShadowsManager();
        FRAME_LAYER_GUARD(-1);
        if (!static_shadow_debug_mesh_shader.material)
        {
          static_shadow_debug_mesh_shader.init("static_shadow_debug_mesh_shader", nullptr, 0, "static_shadow_debug_mesh_shader");
        }

        TextureInfo ti;
        shadowsManager.getStaticShadowsTex()->getinfo(ti);
        int staticShadowSize = ti.w;
        float samplingSize = staticShadowSize * samplingRange;
        ShaderGlobal::set_int(::get_shader_variable_id("static_shadow_size", true), staticShadowSize);
        ShaderGlobal::set_real(::get_shader_variable_id("static_shadow_sampling_range", true), samplingRange);
        TMatrix4 matrixInverse = inverse(shadowsManager.getStaticShadows()->getLightViewProj(0));
        ShaderGlobal::set_float4x4(::get_shader_variable_id("static_shadow_matrix_inverse"), matrixInverse.transpose());

        int line_count = samplingSize * samplingSize * 4;
        static_shadow_debug_mesh_shader.shader->setStates();
        d3d_err(d3d::draw(PRIM_LINELIST, 0, line_count));
      }
    };
  }));

  // static shadow frustums node
  fg_node_handles.emplace_back(ns.registerNode("static_shadows_frustums", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("static_shadow_mesh_node");
    setupTargetForDebug(registry);

    auto prevNs = registry.root();
    prevNs.readBlob<CameraParams>("current_camera").bindAsView<&CameraParams::viewTm>().bindAsProj<&CameraParams::jitterProjTm>();
    prevNs.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.requestState().setFrameBlock("global_frame");

    return [] {
      if (show_static_shadow_frustums.get())
      {
        ::begin_draw_cached_debug_lines(true, false);
        ::set_cached_debug_lines_wtm(TMatrix::IDENT);

        auto &shadowsManager = WRDispatcher::getShadowsManager();
        const ToroidalStaticShadows *staticShadows = shadowsManager.getStaticShadows();
        for (int cascadeId = 0; cascadeId < staticShadows->cascadesCount(); ++cascadeId)
          draw_debug_frustum(Frustum(staticShadows->getLightViewProj(cascadeId)), E3DCOLOR(255, 0, 0));

        ::end_draw_cached_debug_lines();
      }
    };
  }));

  fg_node_handles.emplace_back(dafg::register_node("debug_visualization_publish_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto prevNs = registry.root() / "debug";
    prevNs.rename("target_for_debug", "target_after_debug", dafg::History::No).texture();
  }));
}
