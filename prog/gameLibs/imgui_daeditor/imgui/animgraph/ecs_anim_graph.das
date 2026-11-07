options persistent_heap
options no_global_variables = false
require ecs
require ecs.common
require DagorImgui
require gameLibs.imgui_daeditor.imgui.animgraph.anim_graph_common
require AnimV20
require imgui

require imgui/imgui_node_editor_boost
require daslib/safe_addr



def copy_button(text : string)
  if imgui::Button("copy##copy_{text}")
    imgui::CloseCurrentPopup()
    imgui::SetClipboardText(text)
  imgui::SameLine()
  imgui::Text(text)

// Shared with the window body: Begin returns false when collapsed, so the body never runs then.
// ES below still publishes while the window is opened (incl. collapsed).
var private animgraphSelectedCharEid = INVALID_ENTITY_ID
var private animgraphUseDaeditorSelected = true

def private resolve_daeditor_selected_char(var selected_char_eid : EntityId&)
  query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
    selected_char_eid = eid

// Write-if-changed so the bridge singleton is not dirtied every frame.
def private publish_animgraph_prop_panel_focus(selected_char_eid : EntityId)
  query() <| $ [es] (var animgraph_prop_panel__focus_eid : EntityId&)
    if animgraph_prop_panel__focus_eid != selected_char_eid
      animgraph_prop_panel__focus_eid = selected_char_eid

// The window body draws the graph under this same rule; reject must stay in
// step or it pends a request the viewer would consume (or vice versa).
def private char_has_anim_graph(animchar : AnimcharBaseComponent&)
  return animchar.animGraph != null && animchar.animState != null

// The in-graph consume runs only when a graph is drawn; reject a graphless focus
// here so a pending select request cannot wait forever.
def private reject_pending_select_without_graph(selected_char_eid : EntityId)
  var pending = false
  query() <| $ [es] (animgraph_prop_panel__select_request : StringList)
    pending = !empty(animgraph_prop_panel__select_request)
  if !pending
    return
  var hasGraph = false
  query(selected_char_eid) <| $ [es] (animchar : AnimcharBaseComponent&)
    hasGraph = char_has_anim_graph(animchar)
  if hasGraph
    return
  query() <| $ [es] (var animgraph_prop_panel__select_request : StringList&; var animgraph_prop_panel__select_status : das_string&)
    animgraph_prop_panel__select_status := "Select failed: no anim graph is drawn in the viewer"
    clear(animgraph_prop_panel__select_request)

// imgui_perform_registered skips the window function when Begin is false (collapsed).
// Keep focus_eid fresh for daeditor follow / last pin while the viewer stays opened.
[es(tag=render, no_order)]
def animgraph_prop_panel_publish_focus(info : UpdateStageInfoAct)
  if imgui_get_state() == ImGuiState.OFF
    return
  if !imgui_window_is_visible("Anim", "Anim graph")
    return
  if animgraphSelectedCharEid == INVALID_ENTITY_ID || animgraphUseDaeditorSelected
    resolve_daeditor_selected_char(animgraphSelectedCharEid)
  publish_animgraph_prop_panel_focus(animgraphSelectedCharEid)
  reject_pending_select_without_graph(animgraphSelectedCharEid)

[imgui_window(name="Anim graph", group="Anim")]
def ecs_anim_graph2()
  let size = imgui::GetWindowSize()
  if size.x <= 500f || size.y <= 500f
    imgui::SetWindowSize(float2(500))

  if imgui::Button("Daeditor selected {animgraphUseDaeditorSelected ? "(on)" : "(off)"}")
    animgraphUseDaeditorSelected = true

  if !doesEntityExist(animgraphSelectedCharEid)
    animgraphSelectedCharEid = INVALID_ENTITY_ID

  if animgraphSelectedCharEid == INVALID_ENTITY_ID || animgraphUseDaeditorSelected
    resolve_daeditor_selected_char(animgraphSelectedCharEid)

  if animgraphSelectedCharEid == INVALID_ENTITY_ID
    query() <| $ [es(REQUIRE=hero)] (eid : EntityId)
      imgui::SameLine()
      if imgui::Button("Hero ({split_template(getEntityTemplateName(eid))[0]})")
        animgraphSelectedCharEid = eid
        animgraphUseDaeditorSelected = false

  query(animgraphSelectedCharEid) <| $ [es] (animchar_attach__attachedTo : EntityId)
    if animchar_attach__attachedTo != INVALID_ENTITY_ID
      imgui::SameLine()
      imgui::Text("Attached to:")
      imgui::SameLine()

      if imgui::Button("{split_template(getEntityTemplateName(animchar_attach__attachedTo))[0]} ({animchar_attach__attachedTo})")
        animgraphSelectedCharEid = animchar_attach__attachedTo
        animgraphUseDaeditorSelected = false

  imgui::SameLine()
  imgui::Text("Attachments: ")
  query() <| $ [es] (eid : EntityId; animchar_attach__attachedTo : EntityId; var animchar aka attach_animchar : AnimcharBaseComponent&)
    if attach_animchar.animGraph == null || attach_animchar.animState == null
      return

    if animchar_attach__attachedTo != animgraphSelectedCharEid || animgraphSelectedCharEid == INVALID_ENTITY_ID
      return

    imgui::SameLine()
    if imgui::Button("{split_template(getEntityTemplateName(eid))[0]} ({eid})")
      animgraphSelectedCharEid = eid
      animgraphUseDaeditorSelected = false

  // Same-frame publish after pin UI (ES may have run earlier this frame with the old pin).
  publish_animgraph_prop_panel_focus(animgraphSelectedCharEid)

  query(animgraphSelectedCharEid) <| $ [es] (eid : EntityId; var animchar__animState : Object?; var animchar : AnimcharBaseComponent)
    if imgui::Button("*")
      imgui::OpenPopup("anim_graph_popup_{eid}")

    if imgui::BeginPopup("anim_graph_popup_{eid}")
      copy_button("{eid}")
      copy_button("{animchar |> animchar_getResName()}")
      copy_button("{getEntityTemplateName(eid)}")
      copy_button("{split_template(getEntityTemplateName(eid))[0]}")

      imgui::EndPopup()
    imgui::SameLine()
    imgui::Text("Selected Currently: {animchar |> animchar_getResName()}  |  ({eid})<{getEntityTemplateName(eid)}>")
    if !char_has_anim_graph(animchar)
      return

    var resetState = false
    render_anim_graph(animchar, resetState)
    if resetState
      eid |> addSubTemplate("animchar_lock_anim_state_change")
      if animchar__animState != null
        *animchar__animState |> clear()
    else
      eid |> removeSubTemplate("animchar_lock_anim_state_change")

  if animgraphSelectedCharEid == INVALID_ENTITY_ID
    imgui::Text("Select any entity in editor or click hero button above.")

[es(tag=render, on_event=EventComponentsDisappear, REQUIRE=daeditor__selected)]
def animgraph_restore_animations(evt : Event; eid : EntityId)
  if eid |> has("animchar__lockAnimStateChange")
    eid |> removeSubTemplate("animchar_lock_anim_state_change")
