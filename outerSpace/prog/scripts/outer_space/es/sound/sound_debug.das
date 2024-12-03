options no_aot
require soundSystem
require soundEvent
require DagorConsole

[console_cmd(name="snd.enum_events")]
def sound_debug_console_enum_events()
  sound_debug_enum_events()

[console_cmd(name="snd.play")]
def sound_debug_console_sound_play(path : string; duration = 5.)
  var handle = soundEvent::play("", path, get_listener_pos())
  if is_valid_handle_value(handle)
    sound_debug("snd.play '{path}' is3d={is_3d(handle)}; maxRange={get_max_distance(handle)}; isOneshot={is_oneshot(handle)}")
  if is_oneshot(handle)
    abandon(handle)
  else
    abandon(handle, duration)
