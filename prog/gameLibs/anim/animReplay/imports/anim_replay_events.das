module anim_replay_events shared
require ecs

enum AnimcharsRecordingFilter
  WATCHED_BY_PLAYER
  DAEDITOR_SELECTED
  ALL_MM_CHARACTERS

[event(broadcast)]
struct AnimReplayStartRecord
  sampleRate : int
  animcharsFilter : AnimcharsRecordingFilter

[event(broadcast)]
struct AnimReplayStopRecord

[event(broadcast)]
struct AnimReplaySaveRecord
  filepath : string
  saveAsText : bool

[event(broadcast)]
struct AnimReplayLoadRecord
  filepath : string

[event(broadcast)]
struct AnimReplayOpenSaveFileDialog

[event(broadcast)]
struct AnimReplayOpenLoadFileDialog

[event(broadcast)]
struct AnimReplayAnimcharsCreated
