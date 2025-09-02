from "%sqstd/frp.nut" import mkLatestByTriggerStream, mkTriggerableLatestWatchedSetAndStorage, MK_COMBINED_STATE
from "%darg/ui_imports.nut" import *
let mkWatchedSetAndStorage = mkTriggerableLatestWatchedSetAndStorage(gui_scene.updateCounter)
let mkFrameIncrementObservable = mkLatestByTriggerStream(gui_scene.updateCounter)

return {
  mkWatchedSetAndStorage
  mkFrameIncrementObservable
  MK_COMBINED_STATE
}
