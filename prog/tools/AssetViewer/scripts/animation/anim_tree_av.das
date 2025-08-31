options persistent_heap
require ecs
require DagorSystem
require imgui
require AnimV20
require AssetViewer
require danetlibs.imgui_daeditor.imgui.animgraph.anim_graph_common

require daslib/static_let

[cpp_event(broadcast)]
struct ImGuiStage {}

[es(tag=tools, REQUIRE=animTreeViewer)]
def update_anim_state_history(event : AnimGraphViewerSetAnimState)
  add_anim_state_to_history(event.state_idx)

[es(tag=tools, REQUIRE=animTreeViewer)]
def anim_tree_ui(event : ImGuiStage)
  if imgui::Begin("anim tree")
    static_let_finalize <|
      var paramsFilter = new ImGuiTextFilter
      var statesFilter = new ImGuiTextFilter
    static_let <|
      var selectedChar = 0

    var animchar = try_get_entity_animchar_base_comp()

    if animchar != null && (*animchar).animGraph != null && (*animchar).animState != null
      // Collect attached chars to allow easy inspection of those with a button press
      var chars : array<tuple<AnimcharBaseComponent?, string>>
      chars.reserve(*animchar |> animchar_getAttachmentSlotsCount())
      chars |> emplace((animchar, *animchar |> animchar_getResName))

      for i in range(*animchar |> animchar_getAttachmentSlotsCount())
        var attach = *animchar |> animchar_getAttachedChar(i)
        if attach != null && (*attach).animGraph != null && (*attach).animState != null
          chars |> emplace((attach, *attach |> animchar_getResName))

      imgui::Text("Chars: ")
      for char, i in chars, range(length(chars))
        imgui::SameLine()
        if imgui::Button("{char._1} {i == selectedChar ? "(selected)" : ""}")
          selectedChar = i
        if i == selectedChar
          animchar = char._0

      var resetState = false
      render_anim_graph2(*animchar, *animchar.animGraph, *animchar.animState, resetState, *paramsFilter, *statesFilter)

  imgui::End()