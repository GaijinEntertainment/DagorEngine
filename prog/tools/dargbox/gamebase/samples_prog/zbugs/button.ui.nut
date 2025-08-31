from "%darg/ui_imports.nut" import *

let sf = Watched(0)

let onClick = @() dlog("click")

return @(){
  watch = sf
  size = hdpx(150)
  rendObj = ROBJ_SOLID
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  color = sf.get() & S_HOVER ? 0xDD52C4E4 : 0xFF000000
  onClick
  pos = [hdpx(50), 0]
  onElemState = @(s) sf.set(s)
  behavior = Behaviors.Button
  transform = {
    scale = sf.get() & S_ACTIVE ? [0.5, 0.5] : [1, 1]
  }
}
