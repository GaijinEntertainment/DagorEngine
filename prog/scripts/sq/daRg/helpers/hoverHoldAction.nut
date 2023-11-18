from "daRg" import gui_scene

const REPAY_TIME = 0.3

let allTimers = {}

function mkOnHover(groupId, itemId, action, repayTime = REPAY_TIME) {
  if (!(groupId in allTimers))
    allTimers[groupId] <- {}
  let groupTimers = allTimers[groupId]
  return function(on) {
    if (groupTimers?[itemId]) {
      gui_scene.clearTimer(groupTimers[itemId])
      groupTimers.$rawdelete(itemId)
    }
    if (!on)
      return
    groupTimers[itemId] <- function() {
      groupTimers.$rawdelete(itemId)
      action(itemId)
    }
    gui_scene.setTimeout(repayTime, groupTimers[itemId])
  }
}

return mkOnHover