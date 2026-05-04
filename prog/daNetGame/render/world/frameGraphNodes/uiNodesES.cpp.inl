// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include "drv/3d/dag_texFlags.h"
#include "frameGraphNodes.h"
#include <util/dag_convar.h>
#include "ui/userUi.h"
#include "ui/uiShared.h"
#include "ui/uiRender.h"
#include <render/screencap.h>
#include <render/renderEvent.h>
#include <gui/dag_stdGuiRender.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/antiAliasing_legacy.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include "render/world/uiBlurTexHelper.h"
#include <render/world/wrDispatcher.h>
#include <daRg/dag_guiScene.h>
#include <gui/dag_imgui.h>
#include <gui/dag_visualLog.h>
#include <main/console.h>
#include <debug/dag_textMarks.h>
#include <render/dag_cur_view.h>
#include <render/renderer.h>
#include <render/renderEvent.h>
#include <drv/3d/dag_renderTarget.h>

extern ConVarT<bool, false> hide_gui;

namespace
{
bool should_hide_gui() { return screencap::should_hide_gui() || hide_gui.get(); }
bool should_hide_debug() { return screencap::should_hide_debug(); }
} // namespace

eastl::fixed_vector<dafg::NodeHandle, 2> makeBeforeUIControlNodes()
{
  auto ns = dafg::root() / "before_ui";

  eastl::fixed_vector<dafg::NodeHandle, 2> res;

  res.push_back(ns.registerNode("begin", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    auto prevNs = registry.root();
    prevNs.renameTexture("frame_to_present", "frame");
    return []() {};
  }));

  res.push_back(ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    registry.renameTexture("frame", "frame_done");
    return []() {};
  }));

  return res;
}

dafg::NodeHandle makeUIRenderNode(bool withHistory)
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

dafg::NodeHandle makeUIBlendNode()
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
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_ui_nodes_es(const OnCameraNodeConstruction &evt)
{
  for (auto &&n : makeBeforeUIControlNodes())
    evt.nodes->push_back(eastl::move(n));

  if (WRDispatcher::needSeparatedUI())
  {
    AntiAliasing *antiAliasing = WRDispatcher::getAntialiasing();
    // this is currently only used for frame generation, so currently always true in this branch
    const bool needHistory = true;
    evt.nodes->push_back(makeUIRenderNode(needHistory));

    if (antiAliasing && antiAliasing->needsUIBlending())
      evt.nodes->push_back(makeUIBlendNode());
  }
}