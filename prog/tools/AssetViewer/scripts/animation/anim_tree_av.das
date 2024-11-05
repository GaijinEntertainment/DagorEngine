options persistent_heap
require ecs
require DagorSystem
require imgui
require AnimV20
require danetlibs.imgui_daeditor.imgui.anim_graph_common

require daslib/static_let

[cpp_event(broadcast)]
struct ImGuiStage {}

[es(tag=tools, REQUIRE=animTreeViewer)]
def anim_tree_ui(event : ImGuiStage; eid : EntityId; var animchar : AnimcharBaseComponent)
  if imgui::Begin("anim tree")
    imgui::Text(getEntityTemplateName(eid))

    static_let_finalize <|
      var paramsFilter = new ImGuiTextFilter

    if animchar.animGraph != null && animchar.animState != null
      var resetState = false
      render_anim_graph2(*animchar.animGraph, *animchar.animState, resetState, *paramsFilter)

  imgui::End()