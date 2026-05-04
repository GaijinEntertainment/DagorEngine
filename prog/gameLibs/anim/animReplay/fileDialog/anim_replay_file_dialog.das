require ecs
require stddlg
require %gameLibs/anim/animReplay/imports/anim_replay_events

let SAVE_AS_TEXT = false

[es]
def anim_replay_save_file_dialog(var evt : AnimReplayOpenSaveFileDialog)
  let savePath = get_dlg_save_file("animReplay", "", SAVE_AS_TEXT ? "txt" : "arpl")
  if !empty(savePath)
    broadcastEvent(AnimReplaySaveRecord(filepath = savePath, saveAsText = SAVE_AS_TEXT))

[es]
def anim_replay_load_file_dialog(var evt : AnimReplayOpenLoadFileDialog)
  let loadPath = get_dlg_open_file("", "arpl")
  if !empty(loadPath)
    broadcastEvent(AnimReplayLoadRecord(filepath = loadPath))
