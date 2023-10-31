from "dagor.workcycle" import setTimeout, clearTimer
from "dagor.random" import frnd
let math = require("math")

/*
Creates and returns a new debounced version of the passed function which will postpone its execution until
after wait milliseconds have elapsed since the last time it was invoked.
when delay_s_max is not a null, real delay will become random value between delay_s and delay_s_max

Useful for implementing behavior that should only happen after the input has stopped arriving.
For example: rendering a preview of a Markdown comment, recalculating a layout after the window has stopped being resized, and so on.
*/


let function debounce(func, delay_s, delay_s_max = null){
  let storage = { func = @() null }
  let action = @() storage.func()
  let function debounced(...) {
    storage.func <- @() func.acall([null].extend(vargv))
    clearTimer(action)
    let time = delay_s_max == null
      ? delay_s
      : math.min(delay_s, delay_s_max) + frnd() * math.abs(delay_s_max - delay_s)
    setTimeout(time, action)
  }
  return debounced
}

/*
Same as debounce but trigger the function on the leading instead of the trailing edge of the wait interval.
Useful in circumstances like preventing accidental double-clicks on a "submit" button from firing a second time.
*/
let function debounceImmediate(func, delay_s){
  local isActionAllowed = true
  let function allowAction() { isActionAllowed = true }
  let function debounced(...) {
    if (!isActionAllowed)
      return
    isActionAllowed = false
    func.acall([null].extend(vargv))
    setTimeout(delay_s, allowAction)
  }
  return debounced
}

/*
  Creates and returns a new, throttled version of the passed function, that, when invoked repeatedly, will only actually call the original function at most once per every wait milliseconds.
  Useful for rate-limiting events that occur faster than you can keep up with.

  By default, throttle will execute the function as soon as you call it for the first time, and, if you call it again any number of times during the wait period, as soon as that period is over.
  If you'd like to disable the leading-edge call, pass {leading: false}, and if you'd like to disable the execution on the trailing-edge, pass {trailing: false}.
*/
let defThrottleOptions = {leading = true, trailing=false}
let function throttle(func, delay_s, options=defThrottleOptions){
  let leading = options?.leading ?? defThrottleOptions.leading
  let trailing = options?.trailing ?? defThrottleOptions.trailing
  local needCallByTimer = false //only for !trailing version
  assert(leading || trailing, "throttle should be called with at least one front call leading or trailing")
  local curAction = null
  let function throttled(...){
    let doWait = curAction != null
    curAction = @() func.acall([null].extend(vargv))
    if (doWait) {
      needCallByTimer = !trailing
      return
    }
    let function clearThrottled(){
      if (trailing)
        curAction()
      else if (needCallByTimer) {
        needCallByTimer = false
        curAction()
        setTimeout(delay_s, clearThrottled)
        return
      }
      curAction = null
    }
    if (leading){
      curAction()
    }
    setTimeout(delay_s, clearThrottled)
  }
  return throttled
}

return {
  throttle
  debounce
  debounceImmediate
}