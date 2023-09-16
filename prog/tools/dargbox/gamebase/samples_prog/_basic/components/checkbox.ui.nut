from "%darg/ui_imports.nut" import *
let {checkbox} = require("checkbox.nut")

let state0 = Watched(true)
let state1 = Watched(true)
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
      checkbox({state=state0, text="checkbox"})
      checkbox({state=state1, text= null, textCtor = function(text, state=null, stateFlags=null, _textStyle= null){
          return @(){
            text=text ?? (state?.value ? $"checked" : $"not checked"), watch=[state1, stateFlags], rendObj = ROBJ_TEXT
          }
        }
      })
    ]
  }
}

