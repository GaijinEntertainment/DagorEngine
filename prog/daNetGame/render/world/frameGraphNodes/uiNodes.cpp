// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
#include "frameGraphNodes.h"
#include <util/dag_convar.h>
#include "ui/userUi.h"
#include "ui/uiShared.h"
#include "ui/uiRender.h"
#include <render/screencap.h>
#include <render/renderEvent.h>
#include <gui/dag_stdGuiRender.h>
#include <daECS/core/entityManager.h>
#include <render/world/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <daRg/dag_guiScene.h>
#include <gui/dag_imgui.h>
#include <gui/dag_visualLog.h>
#include <main/console.h>
#include <debug/dag_textMarks.h>
#include <render/dag_cur_view.h>
#include <render/renderer.h>
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
    prevNs.renameTexture("frame_to_present", "frame", dafg::History::No);
    return []() {};
  }));

  res.push_back(ns.registerNode("end", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::Viewport);
    registry.renameTexture("frame", "frame_done", dafg::History::No);
    return []() {};
  }));

  return res;
}

dafg::NodeHandle makeUIRenderNode(bool withHistory)
{
  return dafg::register_node("ui_render", DAFG_PP_NODE_SRC, [withHistory](dafg::Registry registry) {
    registry.requestRenderPass().color(
      {registry.createTexture2d("ui_tex", withHistory ? dafg::History::DiscardOnFirstFrame : dafg::History::No,
        {TEXFMT_A8R8G8B8 | TEXCF_RTARGET, registry.getResolution<2>("display")})});

    auto beforeUINs = registry.root() / "before_ui";
    beforeUINs.readTexture("frame_done").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE);

    return []() {
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      finish_rendering_ui();
    };
  });
}

dafg::NodeHandle makeUIBlendNode()
{
  return dafg::register_node("ui_blend", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readTexture("ui_tex").atStage(dafg::Stage::POST_RASTER).bindToShaderVar("ui_tex");
    auto beforeUINs = registry.root() / "before_ui";
    registry.requestRenderPass().color({beforeUINs.renameTexture("frame_done", "frame_ready_with_ui", dafg::History::No)});
    registry.executionHas(dafg::SideEffects::External);

    return [shader = PostFxRenderer("ui_blend")]() {
      if (!should_hide_gui())
        shader.render();
    };
  });
}
