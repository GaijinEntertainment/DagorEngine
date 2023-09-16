from "%darg/ui_imports.nut" import *

let { mkLatestByTriggerStream, mkTriggerableLatestWatchedSetAndStorage, MK_COMBINED_STATE } = require("%sqstd/frp.nut")

let mkWatchedSetAndStorage = mkTriggerableLatestWatchedSetAndStorage(gui_scene.updateCounter)
let mkFrameIncrementObservable = mkLatestByTriggerStream(gui_scene.updateCounter)

return {
  mkWatchedSetAndStorage
  mkFrameIncrementObservable
  MK_COMBINED_STATE
}
