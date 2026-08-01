options stack = 102400 // render_anim_graph is actaully huge and we run out of stack otherwise
options persistent_heap
require ecs
require DagorSystem
require DagorDataBlock
require imgui
require AnimV20
require AssetViewer
require gameLibs.imgui_daeditor.imgui.animgraph.anim_graph_common

require daslib/static_let

[cpp_event(broadcast)]
struct ImGuiStage {}

[init] // So that after das hot reload vars are still set
def set_animchar_vars_view()
  using() <| $(var appBlk : DataBlock)
    if appBlk |> datablock_load(get_application_blk_path())
      appBlk |> datablock_get_block("animCharView") <| $(animCharViewBlk)
        animCharViewBlk |> datablock_get_block("vars") <| $(varsBlk)
          set_anim_char_view_vars(varsBlk)

[es(REQUIRE=animTreeViewer, on_appear)]
def anim_tree_load_anim_char_view_settings(evt : Event)
  set_animchar_vars_view()

[es(REQUIRE=animTreeViewer)]
def update_anim_state_history(event : AnimGraphViewerSetAnimState)
  add_anim_state_to_history(event.state_idx)

[es(REQUIRE=animTreeViewer)]
def select_asset(event : AnimGraphViewerSelectAssetResource)
  select_asset(event.res_name)

[es(REQUIRE=animTreeViewer)]
def anim_tree_ui(event : ImGuiStage)
  if imgui::Begin("anim tree")

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
      for ch, i in chars, range(length(chars))
        imgui::SameLine()
        if imgui::Button("{ch._1} {i == selectedChar ? "(selected)" : ""}")
          selectedChar = i
        if i == selectedChar
          animchar = ch._0

      var resetState = false
      render_anim_graph(*animchar, resetState)

  imgui::End()