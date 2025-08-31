options persistent_heap
require ecs
require DagorImgui
require danetlibs.imgui_daeditor.imgui.animgraph.anim_graph_common
require AnimV20
require imgui

require imgui/imgui_node_editor_boost
require daslib/static_let
require daslib/safe_addr


[imgui_window(name="Anim graph", group="Anim")]
def ecs_anim_graph2()
  if IsWindowCollapsed()
    return

  static_let_finalize <|
    var paramsFilter = new ImGuiTextFilter
    var statesFilter = new ImGuiTextFilter

  let size = imgui::GetWindowSize()
  if size.x <= 500f || size.y <= 500f
    imgui::SetWindowSize(float2(500))
  let found = find_query <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId; var animchar__animState : Object?; var animchar : AnimcharBaseComponent)
    imgui::Text("{animchar |> animchar_getResName()}  |  ({eid})<{getEntityTemplateName(eid)}>")
    var graph = animchar.animGraph
    if graph != null
      var resetState = false
      render_anim_graph2(animchar, *graph, *animchar.animState, resetState, *paramsFilter, *statesFilter)
      if resetState
        eid |> addSubTemplate("animchar_lock_anim_state_change")
        if animchar__animState != null
          *animchar__animState |> clear()
      else
        eid |> removeSubTemplate("animchar_lock_anim_state_change")
    return true

  if !found
    imgui::Text("Select any entity")

[es(tag=render, on_event=EventComponentsDisappear, REQUIRE=daeditor__selected)]
def animgraph_restore_animations(evt : Event; eid : EntityId)
  if eid |> has("animchar__lockAnimStateChange")
    eid |> removeSubTemplate("animchar_lock_anim_state_change")
