from "%darg/ui_imports.nut" import *

let scrollbar = require("samples_prog/_basic/components/scrollbar.nut")

let function messageInLog(entry) {
  return {
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    preformatted = FMT_NO_WRAP | FMT_KEEP_SPACES
    margin = fsh(0.5)
    size = [flex(), SIZE_TO_CONTENT]
  }.__merge(entry)
}
let function logContent(log_state, scrollHandler) {
  local lastScrolledTo = null
  let function scroll(val){
    local scrollTo = val.len() > 0 ? val.top()?.key : null
    if (scrollTo==null || scrollTo == lastScrolledTo)
      return
    lastScrolledTo = scrollTo
    scrollHandler.scrollToChildren(@(desc) ("key" in desc) ? desc.key == scrollTo : false, 2, false, true)
  }
  return function() {
    return {
      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_VERTICAL
      behavior = Behaviors.RecalcHandler
      children = log_state.value.map(messageInLog)
      watch = log_state
      onRecalcLayout = @(_initial) scroll(log_state.value)
    }
  }
}
let defSz = [flex(), hdpx(300)]
let defColor = Color(120, 120, 120)

let function textLog(log_state, options) {
  let {
    size = defSz,
    color = defColor
  } = options

  let scrollHandler = ScrollHandler()
  return {
    size
    color
    rendObj = ROBJ_FRAME
    borderWidth = [hdpx(1), 0]
    padding = [hdpx(1), 0]

    children = [
      scrollbar.makeVertScroll(logContent(log_state, scrollHandler), {scrollHandler = scrollHandler})
    ]
  }
}

return @(log_state, options = {}) textLog(log_state, options)
