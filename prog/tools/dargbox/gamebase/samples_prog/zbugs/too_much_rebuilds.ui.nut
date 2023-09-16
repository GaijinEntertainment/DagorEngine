from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let comps = {
  a = @() {rendObj = dlog("rebuild", 1) ?? ROBJ_TEXT text=1}
  b = @() {rendObj = dlog("rebuild", 2) ?? ROBJ_TEXT text=2}
  c = freeze({rendObj = dlog("rebuild", 3) ?? ROBJ_TEXT text=3})
  d = @() {rendObj = dlog("rebuild", 4) ?? ROBJ_TEXT text=4}
}
let s = ["a", "b", "c","d"]
let gen = Watched(s)
let click = @() gen.value.len()==1 ? gen(s) : gen(gen.value.slice(0,-1))

let buttonC = watchElemState( @(sf) {
    rendObj = ROBJ_BOX
    size = [sh(20),SIZE_TO_CONTENT]
    padding = sh(2)
    fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
    borderWidth = (sf & S_HOVER) ? 2 : 0
    behavior = [Behaviors.Button]
    halign = ALIGN_CENTER
    onClick = click
    children = {rendObj = ROBJ_TEXT text = "test" color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0)
      pos = (sf & S_ACTIVE) ? [0,2] : [0,0]
    }
})

return @(){
  rendObj = ROBJ_SOLID
  watch = gen
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  vplace = ALIGN_CENTER
  children = [buttonC].extend(gen.value.map(@(i) comps[i]))
}
