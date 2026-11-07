options no_aot

require app // set_timespeed, get_timespeed
require ecs
require gameLibs.imgui_daeditor.daEditor.daEditor_common
require DagorConsole
require DagorEditor


[console_cmd(name="daEd4.select")]
def daeditor_select(eid : uint; auto_zoom : bool = true)
  if doesEntityExist(EntityId(eid))
    editor_open()
    editor_select_eid(EntityId(eid))
    if auto_zoom
      entity_object_editor_zoomAndCenter()
  else
    console_print("{eid}: non-existent entity")


[init]
def init_ecs_inspector_shortcut()
  console_command("consoleKeybindings.bind ctrl_- time.decrease_speed")
  console_command("consoleKeybindings.bind ctrl_= time.increase_speed")


[console_cmd(name="time.increase_speed")]
def time_increase_scale()
  set_timespeed(get_timespeed() * 2.)
  visual_log("time speed: {get_timespeed():.3f}")


[console_cmd(name="time.decrease_speed")]
def time_decrease_scale()
  set_timespeed(max(0.0, get_timespeed() * 0.5))
  visual_log("time speed: {get_timespeed():.3f}")
