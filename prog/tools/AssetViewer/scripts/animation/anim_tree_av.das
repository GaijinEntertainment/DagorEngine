require ecs
require DagorSystem
require imgui
require AnimV20
require danetlibs.imgui_daeditor.imgui.anim_graph_common

[cpp_event(broadcast)]
struct ImGuiStage {}

[es(tag=tools, REQUIRE=animTreeViewer)]
def anim_tree_ui(event : ImGuiStage; eid : EntityId; var animchar : AnimcharBaseComponent)
  if imgui::Begin("anim tree")
    imgui::Text(getEntityTemplateName(eid))

    if animchar.animGraph != null && animchar.animState != null
      var resetState = false
      render_anim_graph2(*animchar.animGraph, *animchar.animState, resetState)

  imgui::End()