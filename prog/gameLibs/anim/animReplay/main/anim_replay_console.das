options no_aot

require ecs
require DagorConsole
require %gameLibs/anim/animReplay/imports/anim_replay_events


[console_cmd(name="anim_replay.save")]
def anim_replay_save(output_filename : string; as_text : bool = false)
  if (find_query <| $ [es(REQUIRE=anim_replay__animchars)] () => true)
    broadcastEvent(AnimReplaySaveRecord(filepath = output_filename, saveAsText = as_text))
  else
    console_print("No active replay to save")
