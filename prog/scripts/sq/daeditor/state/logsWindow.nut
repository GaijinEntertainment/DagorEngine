from "%darg/ui_imports.nut" import *

let showLogsWindow = mkWatched(persist, "showLogsWindow", false)
let hasNewLogerr = Watched(false)

return {
  showLogsWindow
  hasNewLogerr
}
