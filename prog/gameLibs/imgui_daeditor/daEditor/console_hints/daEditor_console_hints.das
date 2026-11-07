options no_aot

require math
require math.base
require daslib/strings_boost
require DagorConsole
require gameLibs.console_commands.main.console_common


[console_processor]
def daeditor_select_processor(args : array<string>;
                              collect : bool;
                              var hints : ConsoleProcessorHints)
  if !collect
    return false

  let cmd = "daEd4.select"
  let inputs <- args[0] |> split(" ")
  let hintsCount = 20

  if length(inputs) >= 2 && inputs[0] == cmd
    let entities <- find_console_hint_entities(inputs[1], /*threshold_cam_distance_sq*/ square(50.0), hintsCount)

    for i in range(min(length(entities), hintsCount))
      hints |> add_hint("{cmd} {entities[i].eid}", 0, 1, "[auto_zoom]", build_entity_hint_desc(entities[i]))
  return false
