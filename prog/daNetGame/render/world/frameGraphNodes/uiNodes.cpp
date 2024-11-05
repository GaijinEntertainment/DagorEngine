// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
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

eastl::fixed_vector<dabfg::NodeHandle, 2> makeBeforeUIControlNodes()
{
  auto ns = dabfg::root() / "before_ui";

  eastl::fixed_vector<dabfg::NodeHandle, 2> res;

  res.push_back(ns.registerNode("begin", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.multiplex(dabfg::multiplexing::Mode::Viewport);
    auto prevNs = registry.root();
    prevNs.renameTexture("frame_to_present", "frame", dabfg::History::No);
    return []() {};
  }));

  res.push_back(ns.registerNode("end", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.multiplex(dabfg::multiplexing::Mode::Viewport);
    registry.renameTexture("frame", "frame_done", dabfg::History::No);
    return []() {};
  }));

  return res;
}

dabfg::NodeHandle makeUIRenderNode(bool withHistory)
{
  return dabfg::register_node("ui_render", DABFG_PP_NODE_SRC, [withHistory](dabfg::Registry registry) {
    registry.requestRenderPass().color(
      {registry.createTexture2d("ui_tex", withHistory ? dabfg::History::DiscardOnFirstFrame : dabfg::History::No,
        {TEXFMT_A8R8G8B8 | TEXCF_RTARGET, registry.getResolution<2>("display")})});

    auto beforeUINs = registry.root() / "before_ui";
    beforeUINs.readTexture("frame_done").atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE);

    return []() {
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      finish_rendering_ui();
    };
  });
}

dabfg::NodeHandle makeUIBlendNode()
{
  return dabfg::register_node("ui_blend", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.readTexture("ui_tex").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("ui_tex");
    auto beforeUINs = registry.root() / "before_ui";
    registry.requestRenderPass().color({beforeUINs.renameTexture("frame_done", "frame_ready_with_ui", dabfg::History::No)});
    registry.executionHas(dabfg::SideEffects::External);

    return [shader = PostFxRenderer("ui_blend")]() {
      if (!should_hide_gui())
        shader.render();
    };
  });
}
