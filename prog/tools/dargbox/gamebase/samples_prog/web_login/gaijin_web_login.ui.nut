from "%sqstd/string.nut" import tostring_r
from "%darg/ui_imports.nut" import *
let { loginData, gsidData, requestLogin } = require("web_login.nut")


let txt = @(text, style = null) {rendObj = ROBJ_TEXT text}.__update(style ?? {})
let txtBtn = function(text, handler=null, opts = null) {
  let stateFlags=Watched(0)
  return @() {
    rendObj = ROBJ_BOX
    watch = stateFlags
    padding = static [hdpx(5), hdpx(10)]
    children = txt(text)
    onClick = handler
    onElemState = @(s) stateFlags.set(s)
    borderRadius = hdpx(5)
    behavior = Behaviors.Button
    fillColor = stateFlags.get() & S_HOVER ? Color(60,80,80) : Color(40,40,40)
  }.__update(opts ?? {})
}

let textArea = @(text) {rendObj = ROBJ_TEXTAREA behavior = Behaviors.TextArea text size = FLEX_H, preformatted=true}

let loginInfo = @() {
  size = flex()
  watch = [loginData, gsidData]
  rendObj = ROBJ_SOLID
  color = Color(30,30,30)
  padding = sh(2)
  children = textArea(
    gsidData.get()==null
      ? "press login button"
      : loginData.get() != null ? tostring_r(loginData.get()) : "waiting for login data ..."
  )

}

let loginBtn = txtBtn("login via web", @() requestLogin(null, dlog))

return {
  flow = FLOW_VERTICAL
  padding = sh(5)
  size = flex()
  halign = ALIGN_CENTER
  children = [
    loginBtn
    loginInfo
  ]
}