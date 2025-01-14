// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1
#include "nodes.h"

#include <render/world/frameGraphHelpers.h>
#include <render/world/gbufferConsts.h>
#include <render/world/hideNodesEvent.h>
#include <render/world/private_worldRenderer.h>
#include <render/world/aimRender.h>

#include <daECS/core/entityManager.h>
#include <frustumCulling/frustumPlanes.h>
#include <render/debugMesh.h>
#include <render/deferredRenderer.h>
#include <render/daBfg/bfg.h>
#include <render/renderEvent.h>
#include <ecs/render/updateStageRender.h>
#include <render/viewVecs.h>

#include <3d/dag_quadIndexBuffer.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <shaders/dag_shaderBlock.h>
#include <util/dag_convar.h>

#include <gui/dag_imgui.h>
#include <imgui/imgui.h>
#include <imgui/implot.h>
#include <render/vertexDensityOverlay.h>

#include <render/world/global_vars.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_renderPass.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>

extern int globalFrameBlockId;
extern ConVarT<bool, false> async_animchars_main;

void start_async_animchar_main_render(const Frustum &fr, uint32_t hints, TexStreamingContext texCtx);

#if DAGOR_DBGLEVEL > 0

struct TileRenderAreaDebugger
{
  int tileSize[2];
  int tileId;
  bool enabled = false;
  bool active = false;
  IPoint2 targetSize;
  IPoint2 tileCount;
  IPoint2 extTileCount;
  DynamicShaderHelper zFill;
  shaders::UniqueOverrideStateId zFuncAlwaysStateId;
  UniqueBufHolder tileInstBuf;
  int tileSizeVarId = -1;
  const char *detectMsg = "unknown";

  struct TileInfo
  {
    vec4f pos;
  };

  eastl::vector<TileInfo> perTileInfos;

  enum
  {
    SEQ_ORIGIN_MIN = 0,
    SEQ_ORIGIN_MAX = 1
  };

  int xOrigin = SEQ_ORIGIN_MIN;
  int yOrigin = SEQ_ORIGIN_MIN;
  bool seqConnect = false;
  bool seqLineMajor = true;
  bool seqSeparateLeftouts = false;

  void resetTileInfo()
  {
    IPoint2 totalTiles = tileCount + extTileCount;
    perTileInfos.clear();
    perTileInfos.resize(totalTiles.x * totalTiles.y);
    tileInstBuf.close();
    tileInstBuf = UniqueBufHolder(
      dag::create_sbuffer(16, perTileInfos.size(), SBCF_MISC_STRUCTURED | SBCF_BIND_SHADER_RES, 0, "debug_tile_positions"),
      "debug_tile_positions");
    tileInstBuf.setVar();

    if (tileSizeVarId < 0)
      tileSizeVarId = get_shader_variable_id("debug_tile_size");

    ShaderGlobal::set_color4(tileSizeVarId,
      Point4(float(tileSize[0]) / float(targetSize.x), float(tileSize[1]) / float(targetSize.y), 0, 0));
  }

  void setTileInfo(int idx, IPoint2 pos)
  {
    IPoint2 iPos = IPoint2(pos.x * tileSize[0], pos.y * tileSize[1]);
    float fPos[4];
    fPos[0] = float(iPos.x) / float(targetSize.x);
    fPos[1] = float(iPos.y) / float(targetSize.y);
    fPos[2] = 1;
    fPos[3] = 1;
    perTileInfos[idx].pos = v_ldu(fPos);
  }

  void fillTilePos()
  {
    IPoint2 dlt(seqLineMajor ? 1 : 0, seqLineMajor ? 0 : 1);
    IPoint2 restartDlt(seqLineMajor ? 0 : 1, seqLineMajor ? 1 : 0);
    dlt.x *= xOrigin == SEQ_ORIGIN_MAX ? -1 : 1;
    dlt.y *= yOrigin == SEQ_ORIGIN_MAX ? -1 : 1;

    restartDlt.x *= xOrigin == SEQ_ORIGIN_MAX ? -1 : 1;
    restartDlt.y *= yOrigin == SEQ_ORIGIN_MAX ? -1 : 1;

    IPoint2 primaryTiles = tileCount + (seqSeparateLeftouts ? IPoint2(0, 0) : extTileCount);
    IPoint2 secondaryTiles = seqSeparateLeftouts ? extTileCount : IPoint2(0, 0);
    IPoint2 totalTiles = primaryTiles + secondaryTiles;
    int restartPoint = seqLineMajor ? primaryTiles.x : primaryTiles.y;
    int primaryTilesCount = primaryTiles.x * primaryTiles.y;

    IPoint2 zeroPos(xOrigin == SEQ_ORIGIN_MAX ? primaryTiles.x - 1 : 0, yOrigin == SEQ_ORIGIN_MAX ? primaryTiles.y - 1 : 0);
    IPoint2 zeroPosSec(totalTiles.x - 1, totalTiles.y - 1);

    IPoint2 pos = zeroPos;
    for (int i = 0; i < primaryTilesCount; i++)
    {
      setTileInfo(i, pos);
      if ((i % restartPoint) == (restartPoint - 1))
      {
        pos += restartDlt;
        if (seqConnect)
          dlt *= -1;
        else
        {
          if (seqLineMajor)
            pos.x = zeroPos.x;
          else
            pos.y = zeroPos.y;
        }
        continue;
      }
      pos += dlt;
    }

    // Y scan
    if (secondaryTiles.x)
    {
      pos = IPoint2(zeroPosSec.x, yOrigin == SEQ_ORIGIN_MAX ? totalTiles.y - 1 : 0);
      dlt = IPoint2(0, yOrigin == SEQ_ORIGIN_MAX ? -1 : 1);
      for (int i = 0; i < totalTiles.y; i++)
      {
        setTileInfo(i + primaryTilesCount, pos);
        pos += dlt;
      }
    }

    // X scan
    if (secondaryTiles.y)
    {
      pos = IPoint2(xOrigin == SEQ_ORIGIN_MAX ? totalTiles.x - 1 : 0, zeroPosSec.y);
      dlt = IPoint2(xOrigin == SEQ_ORIGIN_MAX ? -1 : 1, 0);
      for (int i = 0; i < totalTiles.x - (secondaryTiles.x ? 1 : 0); i++)
      {
        setTileInfo(i + primaryTilesCount + (secondaryTiles.x ? totalTiles.y : 0), pos);
        pos += dlt;
      }
    }
  }

  void updateTileInstBuf() { tileInstBuf->updateData(0, perTileInfos.size() * 16, (void *)perTileInfos.data(), 0); }

  void overrideRpArea(RenderPassArea v, const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    active = true;
    targetSize = IPoint2(v.width, v.height);

    if ((tileSize[0] > 0) && (tileSize[1] > 0))
    {
      tileCount = IPoint2(targetSize.x / tileSize[0], targetSize.y / tileSize[1]);
      extTileCount = min(IPoint2(targetSize.x % tileSize[0], targetSize.y % tileSize[1]), IPoint2(1, 1));
    }

    if (enabled)
    {
      IPoint2 totalTiles = tileCount + extTileCount;
      if (perTileInfos.size() != totalTiles.x * totalTiles.y)
        resetTileInfo();

      if (!zFill.shader)
      {
        zFill.init("mobile_deferred_debug_tile_zfill", nullptr, 0, "mobile_deferred_debug_tile_zfill", false);
        index_buffer::init_quads_16bit();

        shaders::OverrideState state = shaders::OverrideState();
        state.set(shaders::OverrideState::Z_FUNC);
        state.zFunc = CMPF_ALWAYS;
        zFuncAlwaysStateId = shaders::overrides::create(state);
      }

      index_buffer::use_quads_16bit();
      set_viewvecs_to_shader(view_tm, proj_tm);
      zFill.shader->setStates();
      d3d::setvsrc(0, 0, 0);
      shaders::overrides::set(zFuncAlwaysStateId);
      d3d::drawind_instanced(PRIM_TRILIST, 0, 2, 0, perTileInfos.size());
      shaders::overrides::reset();
    }
  }

  void apply()
  {
    if ((tileSize[0] < 0) && (tileSize[1] < 0))
      return;

    if ((targetSize.x < 0) && (targetSize.y < 0))
      return;

    if (tileId >= perTileInfos.size())
      return;

    fillTilePos();

    perTileInfos[tileId].pos = v_add(perTileInfos[tileId].pos, v_make_vec4f(0, 0, -1, 0));

    updateTileInstBuf();
  }

  static void imgui_window();

  void detectTileSize()
  {
    // TODO: route info from vendor vulkan exts
    detectMsg = "debug val";
    tileSize[0] = 256;
    tileSize[1] = 256;
  }

  void imguiWindow()
  {
    // avoid drawing window if actual render that is supporting this debug approach is not active
    if (!active)
      return;

    const char *xOriginTx[] = {"Left", "Right"};
    const char *yOriginTx[] = {"Top", "Bottom"};
    bool resetSq = false;

    resetSq |= ImGui::InputInt2("Tile size", tileSize);

    if (ImGui::Button("Detect tile size"))
    {
      detectTileSize();
      resetSq = true;
    }
    ImGui::SameLine();
    ImGui::Text("Detected by: %s", detectMsg);

    resetSq |= ImGui::Combo("X Origin", &xOrigin, xOriginTx, sizeof(xOriginTx) / sizeof(const char *));
    resetSq |= ImGui::Combo("Y Origin", &yOrigin, yOriginTx, sizeof(yOriginTx) / sizeof(const char *));

    resetSq |= ImGui::Checkbox("Connected pattern", &seqConnect);
    resetSq |= ImGui::Checkbox("Line major", &seqLineMajor);
    resetSq |= ImGui::Checkbox("Separate leftouts", &seqSeparateLeftouts);

    if ((tileSize[0] <= 0) || (tileSize[1] <= 0))
      ImGui::Text("Change tile size to valid values to enable");
    else
      resetSq |= ImGui::Checkbox("Enable", &enabled);

    if (resetSq)
    {
      tileId = 0;
      resetTileInfo();
    }

    if (ImGui::InputInt("Tile id", &tileId) || resetSq)
    {
      apply();
    }
    ImGui::Text("Tile id is %s", tileId < perTileInfos.size() ? "valid" : "invalid");
  }
};

#else

struct TileRenderAreaDebugger
{
  void overrideRpArea(RenderPassArea, const TMatrix &, const TMatrix4 &){};
  static void imgui_window(){};
};

#endif

TileRenderAreaDebugger tileRenderAreaDebugger;

#if DAGOR_DBGLEVEL > 0
void TileRenderAreaDebugger::imgui_window() { tileRenderAreaDebugger.imguiWindow(); }
#endif

dabfg::NodeHandle mk_opaque_setup_mobile_node()
{
  return dabfg::register_node("opaque_setup_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("panorama_prepare_mobile");

    const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);
    const size_t gbufCount = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT : MOBILE_GBUFFER_RT_COUNT;
    const auto *gbufNames = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES.data() : MOBILE_GBUFFER_RT_NAMES.data();
    const auto *formats = simplified ? MOBILE_SIMPLIFIED_GBUFFER_FORMATS.data() : MOBILE_GBUFFER_FORMATS.data();

    const auto gbufferResolution = registry.getResolution<2>("main_view");
    for (size_t i = 0; i < gbufCount; ++i)
      registry.create(gbufNames[i], dabfg::History::No).texture({formats[i], gbufferResolution});

    registry.create("target_for_resolve", dabfg::History::No)
      .texture({get_frame_render_target_format() | TEXCF_RTARGET, gbufferResolution});

    registry.create("gbuf_depth_for_opaque", dabfg::History::No)
      .texture({get_gbuffer_depth_format() | TEXCF_RTARGET, gbufferResolution});

    registry.requestState().setFrameBlock("global_frame");

    auto cameraHandle = registry.readBlob<CameraParams>("current_camera").handle();
    use_jitter_frustum_plane_shader_vars(registry);
    return [cameraHandle] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHandle.ref();

      set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

#if DAGOR_DBGLEVEL > 0
      debug_mesh::activate_mesh_coloring_master_override();
#endif

      G_ASSERTF(wr.currentAntiAliasingMode != AntiAliasingMode::MSAA, "mobile deferred doesn't support msaa yet");

      wr.target->setVar();

      const Point3 viewPos = camera.viewItm.getcol(3);
      g_entity_mgr->broadcastEventImmediate(HideNodesEvent(viewPos));
    };
  });
}

dabfg::NodeHandle mk_opaque_begin_rp_node()
{
  return dabfg::register_node("opaque_begin_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);
    const size_t gbufCount = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT : MOBILE_GBUFFER_RT_COUNT;
    const auto *gbufNames = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES.data() : MOBILE_GBUFFER_RT_NAMES.data();
    bool lensRendererEnabledGlobally = lens_renderer_enabled_globally();

    dag::Vector<dabfg::VirtualResourceHandle<BaseTexture, true, false>> renderPassTex;

    auto addToRenderPass = [&renderPassTex, &registry](const char *tex_name) {
      renderPassTex.push_back(
        registry.modify(tex_name).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle());
    };

    for (size_t i = 0; i < gbufCount; ++i)
      addToRenderPass(gbufNames[i]);

    addToRenderPass("gbuf_depth_for_opaque");
    addToRenderPass("target_for_resolve");
    registry.requestState().setFrameBlock("global_frame");

    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    const auto gbufferResolution = registry.getResolution<2>("main_view");
    return [renderPassTex, aimDataHndl, cameraHndl, gbufferResolution, lensRendererEnabledGlobally] {
      const auto &aimData = aimDataHndl.ref();
      if (aimData.lensRenderEnabled && lensRendererEnabledGlobally)
        return;

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      const IPoint2 res = gbufferResolution.get();
      RenderPassArea area = {0, 0, (uint32_t)res.x, (uint32_t)res.y, 0.0f, 1.0f};

      VertexDensityOverlay::bind_UAV();

      eastl::fixed_vector<RenderPassTarget, 5> targets;
      for (const auto handle : renderPassTex)
        targets.push_back({{handle.get(), 0, 0}, make_clear_value(0, 0, 0, 0)});

      d3d::begin_render_pass(wr.mobileRp.opaque, area, targets.data());

      tileRenderAreaDebugger.overrideRpArea(area, cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
    };
  });
}

dabfg::NodeHandle mk_opaque_mobile_node()
{
  return dabfg::register_node("opaque_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);
    const size_t gbufCount = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT : MOBILE_GBUFFER_RT_COUNT;
    const auto *gbufNames = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES.data() : MOBILE_GBUFFER_RT_NAMES.data();
    bool lensRendererEnabledGlobally = lens_renderer_enabled_globally();

    for (size_t i = 0; i < gbufCount; ++i)
      registry.modify(gbufNames[i]).texture().atStage(dabfg::Stage::UNKNOWN).useAs(dabfg::Usage::UNKNOWN);

    registry.rename("gbuf_depth_for_opaque", "gbuf_depth_for_decals", dabfg::History::No)
      .texture()
      .atStage(dabfg::Stage::UNKNOWN)
      .useAs(dabfg::Usage::UNKNOWN);

    registry.requestState().setFrameBlock("global_frame").allowWireframe();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto texCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    auto aimDataHndl = registry.readBlob<AimRenderingData>("aim_render_data").handle();
    use_jitter_frustum_plane_shader_vars(registry);
    return [cameraHndl, texCtxHndl, aimDataHndl, lensRendererEnabledGlobally] {
      const auto &aimData = aimDataHndl.ref();
      if (aimData.lensRenderEnabled && lensRendererEnabledGlobally)
        d3d::next_subpass();

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      if (async_animchars_main.get())
      {
        uint8_t mainHints =
          UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH | UpdateStageInfoRender::RENDER_MAIN;
        start_async_animchar_main_render(camera.noJitterFrustum, mainHints, texCtxHndl.ref());
      }

      wr.renderStaticOpaqueForward(camera.viewItm);
      wr.renderDynamicOpaqueForward(camera.viewItm);
    };
  });
}

dabfg::NodeHandle mk_opaque_resolve_mobile_node()
{
  return dabfg::register_node("opaque_resolve", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    const bool simplified = renderer_has_feature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS);
    const size_t gbufCount = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_COUNT : MOBILE_GBUFFER_RT_COUNT;
    const auto *gbufNames = simplified ? MOBILE_SIMPLIFIED_GBUFFER_RT_NAMES.data() : MOBILE_GBUFFER_RT_NAMES.data();

    for (size_t i = 0; i < gbufCount; ++i)
      registry.read(gbufNames[i]).texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::UNKNOWN);

    registry.rename("target_for_resolve", "target_after_resolve", dabfg::History::No)
      .texture()
      .atStage(dabfg::Stage::PS)
      .useAs(dabfg::Usage::COLOR_ATTACHMENT);

    registry.rename("gbuf_depth_for_resolve", "depth_after_opaque", dabfg::History::No)
      .texture()
      .atStage(dabfg::Stage::PS)
      .useAs(dabfg::Usage::DEPTH_ATTACHMENT);

    registry.requestState().setFrameBlock("global_frame");

    return [] {
      d3d::next_subpass();
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.mobileRp.deferredResolve->render();
    };
  });
}

dabfg::NodeHandle mk_opaque_end_rp_mobile_node()
{
  return dabfg::register_node("native_rp_end_mobile", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto depthHndl = registry.rename("depth_after_opaque", "depth_for_transparency_setup", dabfg::History::No)
                       .texture()
                       .atStage(dabfg::Stage::PS)
                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                       .handle();
    auto colorHndl = registry.rename("target_after_water", "target_for_transparency_setup", dabfg::History::No)
                       .texture()
                       .atStage(dabfg::Stage::PS)
                       .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                       .handle();
    return [depthHndl, colorHndl] {
      d3d::end_render_pass();

      VertexDensityOverlay::unbind_UAV();

      d3d::set_render_target(colorHndl.get(), 0);
      d3d::set_depth(depthHndl.get(), DepthAccess::RW);
    };
  });
}

dabfg::NodeHandle mk_opaque_setup_forward_node()
{
  return dabfg::register_node("opaque_setup_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("panorama_prepare_mobile");

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    const uint32_t depthFlags = get_gbuffer_depth_format() | TEXCF_RTARGET | (wr.isMSAAEnabled() ? wr.msaaQuality : 0);

    auto depthHndl = registry.create("depth_for_opaque", dabfg::History::No)
                       .texture({depthFlags, registry.getResolution<2>("main_view")})
                       .atStage(dabfg::Stage::PS)
                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                       .handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto targetHndl =
      registry
        .registerTexture2d("target_for_opaque",
          [](const dabfg::multiplexing::Index &) {
            ManagedTexView target;

            auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

            if (!shouldRenderGbufferDebug() && !wr.hasFeature(FeatureRenderFlags::POSTFX))
            {
              G_ASSERTF(!wr.isFsrEnabled(), "FSR make no sense without postfx");
              G_ASSERTF(wr.currentAntiAliasingMode != AntiAliasingMode::MSAA && wr.currentAntiAliasingMode != AntiAliasingMode::OFF,
                "Only MSAA or no-AA will work without postfx: %d", int(wr.currentAntiAliasingMode));

              if (wr.currentAntiAliasingMode == AntiAliasingMode::MSAA)
              {
                ExternalTex bb = dag::get_backbuffer();
                target = ManagedTexView{bb};
              }
              else
                target = ManagedTexView{*wr.finalTargetFrame};
            }
            else
            {
              ResizableRTarget::Ptr frameHolder;
              int renderingWidth, renderingHeight;
              wr.getRenderingResolution(renderingWidth, renderingHeight);
              frameHolder = wr.renderFramePool->acquire();
              frameHolder->resize(renderingWidth, renderingHeight);
              target = ManagedTexView{*frameHolder};
            }

            return target;
          })
        .atStage(dabfg::Stage::PS)
        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
        .handle();
    use_jitter_frustum_plane_shader_vars(registry);
    return [targetHndl, depthHndl, cameraHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      ManagedTexView frame = targetHndl.view();
      ManagedTexView depth = depthHndl.view();
      const auto &camera = cameraHndl.ref();

      if (!frame.getTex2D())
        return;

      set_viewvecs_to_shader(camera.viewTm, camera.jitterProjTm);

#if DAGOR_DBGLEVEL > 0
      debug_mesh::activate_mesh_coloring_master_override();
#endif

      wr.target->setVar();
      ShaderGlobal::set_texture(depth_gbufVarId, depth);

      if (wr.currentAntiAliasingMode == AntiAliasingMode::OFF)
      {
        d3d::set_render_target(frame.getTex2D(), 0);
        d3d::set_depth(depth.getTex2D(), DepthAccess::RW);
      }
      else if (wr.currentAntiAliasingMode == AntiAliasingMode::MSAA)
      {
        wr.target->setRt();
        d3d::set_render_target(1, frame.getTex2D(), 0);
      }
      else
        G_ASSERTF(false, "Invalid antialiasing mode");

      {
        TIME_D3D_PROFILE(clearGbuffer)
        d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0x00000000, 0, 0);
      }

      const Point3 viewPos = camera.viewItm.getcol(3);
      g_entity_mgr->broadcastEventImmediate(HideNodesEvent(viewPos));
    };
  });
}

dabfg::NodeHandle mk_rename_depth_opaque_forward_node()
{
  return dabfg::register_node("rename_depth_opaque_forward", DABFG_PP_NODE_SRC,
    [](dabfg::Registry registry) { registry.renameTexture("depth_for_opaque", "depth_opaque_static", dabfg::History::No); });
}

dabfg::NodeHandle mk_static_opaque_forward_node()
{
  return dabfg::register_node("static_opaque_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass()
      .color({registry.renameTexture("target_for_opaque", "target_opaque_static", dabfg::History::No)})
      .depthRw("depth_opaque_static");

    auto depthHndl =
      registry.modifyTexture("depth_opaque_static").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto strmCtxHndl = registry.readBlob<TexStreamingContext>("tex_ctx").handle();
    use_jitter_frustum_plane_shader_vars(registry);

    return [cameraHndl, strmCtxHndl, depthHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      const auto &camera = cameraHndl.ref();

      if (async_animchars_main.get())
      {
        uint8_t mainHints =
          UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH | UpdateStageInfoRender::RENDER_MAIN;
        start_async_animchar_main_render(camera.jitterFrustum, mainHints, strmCtxHndl.ref());
      }

      wr.renderStaticOpaqueForward(camera.viewItm);
      wr.renderStaticDecalsForward(depthHndl.view(), camera.viewTm, camera.jitterProjTm, camera.cameraWorldPos);
    };
  });
}

dabfg::NodeHandle mk_dynamic_opaque_forward_node()
{
  return dabfg::register_node("dynamic_opaque_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass()
      .color({registry.renameTexture("target_opaque_static", "target_opaque_dynamic", dabfg::History::No)})
      .depthRw(registry.renameTexture("depth_opaque_static", "depth_opaque_dynamic", dabfg::History::No));

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    use_jitter_frustum_plane_shader_vars(registry);

    return [cameraHndl] {
      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.renderDynamicOpaqueForward(cameraHndl.ref().viewItm);
    };
  });
}

dabfg::NodeHandle mk_decals_on_dynamic_forward_node()
{
  return dabfg::register_node("decals_on_dynamic_forward", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().allowWireframe().setFrameBlock("global_frame");
    registry.requestRenderPass().depthRoAndBindToShaderVars("depth_opaque_dynamic", {"depth_gbuf"});
    registry.orderMeBefore("panorama_apply_mobile");

    return []() {
      if (!renderer_has_feature(FeatureRenderFlags::DECALS))
        return;
      g_entity_mgr->broadcastEventImmediate(RenderDecalsOnDynamic());
    };
  });
}

REGISTER_IMGUI_WINDOW("Render", "TileRenderAreaDbg##Tile-Render-Area-Dbg", TileRenderAreaDebugger::imgui_window);