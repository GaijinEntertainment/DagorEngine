let { get_time_msec } = require("dagor.time")
let { log } = require("log.nut")()

let timers = []

let start = @() timers.append(get_time_msec())

let show = @(msg = "show", printFunc = log) timers.len() > 0
  ? printFunc($"dbg_timer: {msg}: {get_time_msec() - timers.top()}")
  : printFunc($"dbg_timer: not found timer for {msg}")

function stop(msg = "stop", printFunc = log) {
  show(msg, printFunc)
  if (timers.len())
    timers.pop()
}

let timerFunc = @(func, msg = "func time", printFunc = log) function(...) {
  start()
  let res = func.acall([this].extend(vargv))
  stop(msg, printFunc)
  return res
}

return {
  timerStart = start
  timerShow = show
  timerStop = stop
  timerFunc = timerFunc
}