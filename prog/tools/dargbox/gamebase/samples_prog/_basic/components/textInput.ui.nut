from "%darg/ui_imports.nut" import *
let input = require("textInput.nut")

let textState = Watched("")
let textPwdState = Watched("")
let mailState = persist("mailState", @() Watched(""))
let numState = persist("numState", @() Watched(""))
let intState = persist("intState", @() Watched(""))
let floatState = persist("floatState", @() Watched(""))

return {
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  cursor = Cursor({ rendObj = ROBJ_IMAGE size = [32, 32] image = Picture("!ui/atlas#cursor.svg:{0}:{0}:K".subst(hdpx(32))) })
  children = @() {
    valign  = ALIGN_CENTER
    halign  = ALIGN_CENTER
    flow    = FLOW_VERTICAL
    gap  = sh(2)
    size = flex()
    padding= sh(5)
    children = [
      input(textState, {placeholder="some text"})
      input(textPwdState, {title="password", password=true, placeholder="password"})
      input(mailState,{inputType="mail", placeholder="e-mail"})
      input(intState,{inputType="integer", placeholder="integer"})
      input(floatState,{inputType="float", placeholder="float"})
      input(numState,{inputType="num", placeholder="numeric"})
//      input(textState, options={}, handlers={}, frameCtor=defaultFrame) textInput(text_state, options, handlers, frameCtor)
    ]
  }
}

