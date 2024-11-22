from "%darg/ui_imports.nut" import *

let textInput = require("samples_prog/_basic/components/textInput.nut")
let {makeVertScroll} = require("samples_prog/_basic/components/scrollbar.nut")

function messageInLog(entry) {
  return {
    rendObj = ROBJ_TEXTAREA
    behavior = Behaviors.TextArea
    preformatted = FMT_NO_WRAP | FMT_KEEP_SPACES
    margin = fsh(0.5)
    size = [flex(), SIZE_TO_CONTENT]
  }.__merge(entry)
}

let scrollHandler = ScrollHandler()

function logContent(log_state) {
  return function() {
    return {
      size = [flex(), SIZE_TO_CONTENT]
      flow = FLOW_VERTICAL
      children = log_state.value.map(messageInLog)
      watch = log_state
    }
  }
}

function textLog(log_state) {
  return {
    size = [flex(), hdpx(300)]
    borderColor = Color(120, 120, 120)
    fillColor = Color(0,0,0,120)
    rendObj = ROBJ_BOX
    borderWidth = [hdpx(1), 0]
    padding = [hdpx(1), 0]

    children = makeVertScroll(logContent(log_state), {scrollHandler})
  }
}

let consoleLog = mkWatched(persist, "consoleLog", [])
let textSt = mkWatched(persist, "textSt", "")
let history = mkWatched(persist, "history", [])
local key = 0

local historyCursorPos = 0

let execCode = function(){
  local s = textSt.value
  if (s=="")
    return
  historyCursorPos = 0
  consoleLog.mutate(@(v) v.insert(0, {text = textSt.value, key, color = Color(160,160,160)}))
  history.mutate(@(v) v.append(s))
  try{
    let wrapped = $"return (@()({s}))()"
    let res = compilestring(wrapped, "_", require("%darg/ui_imports.nut").__merge({require}))()
    consoleLog.mutate(@(v) v.insert(0, {text = $"SUCCESS: {res}", key, color = Color(190,230,230)}))
  }
  catch(e){
    consoleLog.mutate(@(v) v.insert(0, {text = $"<color=#FF2222>error</color>, {e}", key, color = Color(200,120,120)}))
  }
  textSt("")
  key++
}
let conTextInput = textInput(textSt, {
  onReturn = execCode
  margin=0
  textmargin = hdpx(4)
  onBlur = @() textSt("")
})

let hotkeys = {
  hotkeys = [
    ["Tab", function() {
      let hs = history.value
      if (historyCursorPos>=hs.len())
        historyCursorPos=0
      historyCursorPos++
      let found = hs?[hs.len()-historyCursorPos]
      textSt(found)
    }]
  ]
}
return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = [flex(), ph(100)]
  flow = FLOW_VERTICAL
  children = [
    {rendObj = ROBJ_TEXTAREA behavior=Behaviors.TextArea size  = [flex(), SIZE_TO_CONTENT]
      text = "Very fast Proof of concept console. TODO: better to_string, better hotkeys, independet scroll and textInput"},
    conTextInput, hotkeys, textLog(consoleLog)
  ]
}
