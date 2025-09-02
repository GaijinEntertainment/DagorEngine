from "%darg/ui_imports.nut" import *
let {textButton} = require("textButton.nut")

let counter = Watched(0)
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
    padding= sh(5)
    children = [
      textButton("sampleButton", @() dlog("pressed"))
      textButton("increment", @() counter.modify(@(v)v+1))
      @(){text = $"counter: {counter.get()}" watch = counter rendObj=ROBJ_TEXT}
      textButton("decrement", @() counter.modify(@(v)v-1))
    ]
  }
}

