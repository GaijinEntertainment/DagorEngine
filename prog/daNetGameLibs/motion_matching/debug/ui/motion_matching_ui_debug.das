options no_aot
options persistent_heap
require ecs
require DagorImgui
require danetlibs.motion_matching.debug.modules.mm_ui_common
require danetlibs.motion_matching.debug.modules.mm_ui_play_clips
require danetlibs.motion_matching.es.mm_events
require danetlibs.renderer.includes.pufd_events
require imgui

require imgui/imgui_node_editor_boost
require daslib/safe_addr

[imgui_window(name="motion matching", group="Anim")]
def motion_matching_imgui_es()
  if IsWindowCollapsed()
    return

  if getSingletonEntity("motion_matching_imgui_state") == INVALID_ENTITY_ID
    createEntity("motion_matching_imgui_state")

  query() <| $ [es] (eid : EntityId; mm_imguiAnimcharEid : EntityId)
    if mm_imguiAnimcharEid != INVALID_ENTITY_ID
      motion_matching_imgui(mm_imguiAnimcharEid, eid)
    else
      imgui::Text("Select any entity with MM component")

[es(tag=dev)]
def mm_choose_imgui_selected_entity(act : ParallelUpdateFrameDelayed;
                                    var mm_imguiAnimcharEid : EntityId&)
  var mmEntity = INVALID_ENTITY_ID
  query() <| $ [es(REQUIRE=(watchedByPlr, motion_matching__controller))] (eid : EntityId)
    mmEntity = eid
  //higher priority
  query() <| $ [es(REQUIRE=(daeditor__selected, motion_matching__controller))] (eid : EntityId)
    mmEntity = eid
  mm_imguiAnimcharEid = mmEntity

[es(tag=dev, before=motion_matching_job_es)]
def mm_enable_for_animation_player(act : ParallelUpdateFrameDelayed;
                                   mm_imguiAnimcharEid : EntityId;
                                   mm_clips_animationPlayMode : int)
  if mm_clips_animationPlayMode != int(AnimationPlayMode.DISABLED)
    query(mm_imguiAnimcharEid) <| $ [es] (var motion_matching__enabled : bool&;
                                          var motion_matching__presetIdx : int&)
      motion_matching__enabled = true
      motion_matching__presetIdx = 0 //default

[es(tag=dev, on_event=InvalidateAnimationDataBase)]
def mm_clear_clips_progress(evt : Event; var mm_clips_clipProgress : IntList)
  resize(mm_clips_clipProgress, 0)
