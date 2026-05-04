options stack = 102400 // render_anim_graph2 is actaully huge and we run out of stack otherwise
options persistent_heap
require ecs
require DagorSystem
require imgui
require AnimV20
require AssetViewer
require gameLibs.imgui_daeditor.imgui.animgraph.anim_graph_common

require daslib/static_let

[cpp_event(broadcast)]
struct ImGuiStage {}

[es(REQUIRE=animTreeViewer)]
def update_anim_state_history(event : AnimGraphViewerSetAnimState)
  add_anim_state_to_history(event.state_idx)

[es(REQUIRE=animTreeViewer)]
def select_asset(event : AnimGraphViewerSelectAssetResource)
  select_asset(event.res_name)

[es(REQUIRE=animTreeViewer)]
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

      // Collect chars which are actually attached to the main char.
      for i in range(*animchar |> animchar_getAttachmentSlotsCount())
        var attach = *animchar |> animchar_getAttachedChar(i)
        if attach != null && (*attach).animGraph != null && (*attach).animState != null
          chars |> emplace((attach, *attach |> animchar_getResName))

      // For some reason in dng based animchar update (animchar.cpp.inl animchar__updater_es)
      // attachment chars are connected to the char only for the duration of animchar update and
      // attachments are released right after. This means we can't get them through
      // animchar_getAttachedChar call.
      // Since we know that anim_tree_ui is only used in Asset Viewer and the only time this code
      // runs is when animchar is viewed we can assume that all animchar entities that exists are
      // of interest to the user. So just add all those animchars to view buttons too unless they
      // are already present there.
      query() <| $ [es] (var animchar aka attach_animchar : AnimcharBaseComponent&)
        if attach_animchar.animGraph == null || attach_animchar.animState == null
          return

        let exists = find_index_if(chars) <| $ (element)
          return element._0 == unsafe(addr(attach_animchar))
        if exists == -1
          chars |> emplace((unsafe(addr(attach_animchar)), attach_animchar |> animchar_getResName))

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