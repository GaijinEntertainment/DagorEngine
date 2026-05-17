// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <daECS/core/entitySystem.h>
#include <drv/3d/dag_texFlags.h>
#include <drv/3d/dag_renderTarget.h>
#include <shaders/dag_postFxRenderer.h>
#include <util/dag_convar.h>
#include <render/screencap.h>
#include <render/renderEvent.h>
#include <render/antiAliasing_legacy.h>
#include "render/world/uiBlurTexHelper.h"
#include <render/world/wrDispatcher.h>
#include <render/renderer.h>

extern ConVarT<bool, false> hide_gui;

static inline bool should_hide_gui() { return screencap::should_hide_gui() || hide_gui.get(); }
static inline bool should_hide_debug() { return screencap::should_hide_debug(); }

static inline void makeBeforeUIControlNodes(dafg::NodeHandle &before_ui_begin, dafg::NodeHandle &before_ui_end)
{
  auto ns = dafg::root() / "before_ui";

  before_ui_begin = ns.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    auto prevNs = registry.root();
    prevNs.renameTexture("frame_to_present", "frame");
    return []() {};
  });

  before_ui_end = ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    registry.renameTexture("frame", "frame_done");
    return []() {};
  });
}

static inline dafg::NodeHandle makeUIRenderNode(bool withHistory)
{
  return dafg::register_node("ui_render", DAFG_PP_NODE_SRC, [withHistory](dafg::Registry registry) {
    auto targetHndl =
      registry.createTexture2d("ui_tex", {TEXFMT_A8R8G8B8 | TEXCF_RTARGET | TEXCF_UNORDERED, registry.getResolution<2>("display")})
        .withHistory(withHistory ? dafg::History::DiscardOnFirstFrame : dafg::History::No)
        .atStage(dafg::Stage::POST_RASTER)
        .useAs(dafg::Usage::COLOR_ATTACHMENT)
        .handle();

    auto beforeUINs = registry.root() / "before_ui";
    beforeUINs.readTexture("frame_done").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);

    return [targetHndl]() {
      BaseTexture *uiTex = targetHndl.view().getBaseTex();
      d3d::set_render_target({}, DepthAccess::RW, {{uiTex, 0, 0}});
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      if (!screencap::is_screenshot_scheduled() || !get_world_renderer()->needUIBlendingForScreenshot())
      {
        set_ui_tex_for_blur(uiTex);
        finish_rendering_ui();
        set_ui_tex_for_blur(nullptr);
      }
    };
  });
}

static inline dafg::NodeHandle makeUIBlendNode()
{
  return dafg::register_node("ui_blend", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readTexture("ui_tex").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("ui_tex");
    auto beforeUINs = registry.root() / "before_ui";
    registry.requestRenderPass().color({beforeUINs.renameTexture("frame_done", "frame_ready_with_ui")});
    registry.executionHas(dafg::SideEffects::External);

    return [shader = PostFxRenderer("ui_blend")]() {
      if (!should_hide_gui())
        shader.render();
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction, SetAntialiasing)
static inline void create_ui_nodes_es(const ecs::Event &,
  dafg::NodeHandle &before_ui_begin,
  dafg::NodeHandle &before_ui_end,
  dafg::NodeHandle &ui_render,
  dafg::NodeHandle &ui_blend)
{
  if (!get_world_renderer())
    return;

  makeBeforeUIControlNodes(before_ui_begin, before_ui_end);

  ui_render = {};
  ui_blend = {};
  if (WRDispatcher::needSeparatedUI())
  {
    AntiAliasing *antiAliasing = WRDispatcher::getAntialiasing();
    // this is currently only used for frame generation, so currently always true in this branch
    const bool needHistory = true;
    ui_render = makeUIRenderNode(needHistory);

    if (antiAliasing && antiAliasing->needsUIBlending())
      ui_blend = makeUIBlendNode();
  }
}