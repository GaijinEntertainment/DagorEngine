from "%darg/ui_imports.nut" import *

function input(text_state, state_flags, group, options = {}) {
  let sf = state_flags.get()
  return @() {
    rendObj = ROBJ_BOX
    fillColor = (sf & S_KB_FOCUS) ? Color(10,10,10) : Color(50,50,50)
    border = 1
    borderColor = (sf & S_HOVER) ? Color(195,195,195) : (sf & S_KB_FOCUS) ? Color(155,155,155) : Color(140,40,40)
    group = group
    size = flex()
    padding = 5
    color = (sf & S_KB_FOCUS) ? Color(200, 150, 0) : Color(255, 255, 255, 200)
    watch = [state_flags, text_state]

    children = {
      rendObj = ROBJ_TEXT
      vplace = ALIGN_CENTER
      hplace = ALIGN_CENTER
      size = [flex(), fontH(100)]
      group = group
      behavior = Behaviors.TextInput
      text = text_state.get()
      password = options?.password
      key = text_state
      function onChange(val) {
        text_state.set(val)
        vlog(val)
      }
      function onReturn() {
        vlog($"entered text: {text_state.get()}")
        text_state.set("")
      }
      function onEscape() {
        vlog("cleared text!")
        text_state.set("")
      }
      color = (sf & S_KB_FOCUS) ? Color(255, 255, 255) : Color(160, 160, 160)
    }
  }
}

let textMsg = Watched("")
let textPwd = Watched("")

function inputElem(state, params={options={}, title=null}){
  let stateFlags = Watched(0)
  let group = ElemGroup()
  let title = params?.title
  let options = params?.options ?? {}
  let size = params?.size ?? [hdpx(300), fontH(150)]
  local titleCmp
  if (title)
   titleCmp = {rendObj = ROBJ_TEXT text=title}
  return function(){
    return {
      watch   = stateFlags
      group   = group
      gap  = sh(2)
      size = size
      flow = FLOW_HORIZONTAL
      valign = ALIGN_CENTER
      onElemState = @(sf) stateFlags.set(sf)
      children = [ titleCmp input(state, stateFlags, group, options) ]
    }
  }
}

return {
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  cursor = Cursor({ rendObj = ROBJ_IMAGE size = 32 image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32))) })
  children = @() {
    valign  = ALIGN_CENTER
    halign  = ALIGN_CENTER
    flow    = FLOW_VERTICAL
    gap  = sh(2)
    size = flex()
    children = [
      inputElem(textMsg, {title="text input"})
      inputElem(textPwd, {title="password input", options= {password=true}})
    ]
  }
}
