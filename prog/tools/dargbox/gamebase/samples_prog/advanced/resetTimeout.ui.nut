from "%darg/ui_imports.nut" import *

let logs = Watched("")

let addLog = @(text)logs.set($"{logs.get()}{text}\n")

let correctFunc = @() addLog("Correct func called only once")

gui_scene.resetTimeout(1, correctFunc)
gui_scene.resetTimeout(2, correctFunc)
gui_scene.resetTimeout(3, correctFunc)

gui_scene.resetTimeout(1, @() addLog("Incorrect func after 1 sec called"))
gui_scene.resetTimeout(2, @() addLog("Incorrect func after 2 sec called"))
gui_scene.resetTimeout(3, @() addLog("Incorrect func after 3 sec called"))

function autoCallback(delay) {
  let localFunc = @() addLog($"Local anonymous func after {delay} sec called")
  gui_scene.resetTimeout(delay, localFunc, localFunc)
}
autoCallback(1)
autoCallback(2)
autoCallback(3)

/*

resetTimeout can only reset the timer for a known callback function,
not for an anonymous function created each time the method is called.
all such functions are interpreted as DIFFERENT

*/

return @() {
  watch = logs
  rendObj = ROBJ_TEXTAREA
  behavior = Behaviors.TextArea
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  text = logs.get()
}