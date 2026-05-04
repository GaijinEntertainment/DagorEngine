options persistent_heap
require ecs
require ecs.common
require DagorImgui
require gameLibs.imgui_daeditor.imgui.animgraph.anim_graph_common
require AnimV20
require imgui

require imgui/imgui_node_editor_boost
require daslib/static_let
require daslib/safe_addr



def copy_button(text : string)
  if imgui::Button("copy##copy_{text}")
    imgui::CloseCurrentPopup()
    imgui::SetClipboardText(text)
  imgui::SameLine()
  imgui::Text(text)


[imgui_window(name="Anim graph", group="Anim")]
def ecs_anim_graph2()
  if IsWindowCollapsed()
    return

  static_let_finalize <|
    var paramsFilter = new ImGuiTextFilter
    var statesFilter = new ImGuiTextFilter

  static_let <|
    var selectedCharEid = INVALID_ENTITY_ID
    var useDaeditorSelected = true

  let size = imgui::GetWindowSize()
  if size.x <= 500f || size.y <= 500f
    imgui::SetWindowSize(float2(500))

  if imgui::Button("Daeditor selected{useDaeditorSelected ? "(on)" : "(off)"}")
    useDaeditorSelected = true

  if selectedCharEid == INVALID_ENTITY_ID || useDaeditorSelected
    query() <| $ [es(REQUIRE=daeditor__selected)] (eid : EntityId)
      selectedCharEid = eid

  query(selectedCharEid) <| $ [es] (animchar_attach__attachedTo : EntityId)
    if animchar_attach__attachedTo != INVALID_ENTITY_ID
      imgui::SameLine()
      imgui::Text("Attached to:")
      imgui::SameLine()

      if imgui::Button("{split_template(getEntityTemplateName(animchar_attach__attachedTo))[0]} ({animchar_attach__attachedTo})")
        selectedCharEid = animchar_attach__attachedTo
        useDaeditorSelected = false

  imgui::SameLine()
  imgui::Text("Attachments: ")
  query() <| $ [es] (eid : EntityId; animchar_attach__attachedTo : EntityId; var animchar aka attach_animchar : AnimcharBaseComponent&)
    if attach_animchar.animGraph == null || attach_animchar.animState == null
      return

    if animchar_attach__attachedTo != selectedCharEid || selectedCharEid == INVALID_ENTITY_ID
      return

    imgui::SameLine()
    if imgui::Button("{split_template(getEntityTemplateName(eid))[0]} ({eid})")
      selectedCharEid = eid
      useDaeditorSelected = false

  query(selectedCharEid) <| $ [es] (eid : EntityId; var animchar__animState : Object?; var animchar : AnimcharBaseComponent)
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
    if animchar.animGraph == null || animchar.animState == null
      return

    var resetState = false
    render_anim_graph2(animchar, *animchar.animGraph, *animchar.animState, resetState, *paramsFilter, *statesFilter)
    if resetState
      eid |> addSubTemplate("animchar_lock_anim_state_change")
      if animchar__animState != null
        *animchar__animState |> clear()
    else
      eid |> removeSubTemplate("animchar_lock_anim_state_change")

  if selectedCharEid == INVALID_ENTITY_ID
    imgui::Text("Select any entity")

[es(tag=render, on_event=EventComponentsDisappear, REQUIRE=daeditor__selected)]
def animgraph_restore_animations(evt : Event; eid : EntityId)
  if eid |> has("animchar__lockAnimStateChange")
    eid |> removeSubTemplate("animchar_lock_anim_state_change")
