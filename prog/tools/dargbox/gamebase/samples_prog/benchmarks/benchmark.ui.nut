import "math" as math
import "string" as string
from "%darg/ui_imports.nut" import *
const componentsNum = 2000 //amount of components
const rt_update = true //use realtimeUpdate behaviour
const rtRecalcLayout = true
const borders = true //show borders on each element
const texts = false //show timers on each element
const useFlow = false
const rtTransform = false
const calcLayout = false

let cursors = require("samples_prog/_cursors.nut")

let timer =  mkWatched(persist, "timer", 0)

local timerFloat = 0
let showChild = []

gui_scene.setUpdateHandler(function(dt) {
  timerFloat += dt
  timer.set(math.floor(timerFloat).tointeger())
})


function simpleComponent(val){
  let pos = useFlow ? null : [sw(math.rand()*80/math.RAND_MAX), sh(math.rand()*80/math.RAND_MAX)]
  let size = [sh(math.rand()*15/math.RAND_MAX+2), sh(math.rand()*15/math.RAND_MAX+2) / (useFlow ? 10 : 1)]
  let color = Color(math.rand()*255/math.RAND_MAX, math.rand()*255/math.RAND_MAX, math.rand()*255/math.RAND_MAX)
  function frc() {return (math.rand()*255/math.RAND_MAX).tointeger()}
  let children = []

  if (borders && showChild[val].get()) {
    children.append(@() {
                 //color = Color(rc+timer.get()*57,rc+state.timer.get()*31,rc+state.timer.get()*89)
                 //color = Color(frc(),frc(),frc())
                 borderWidth = 1
                 rendObj=ROBJ_FRAME size=flex()
                 behavior = rt_update ? [Behaviors.RtPropUpdate] : null
                 update = @() {
                   color=Color(frc(),frc(),frc())
                 }
                 rtRecalcLayout = false
               })
  }
  if (texts && showChild[val].get()) {
    children.append( @() {
                 rendObj=ROBJ_TEXT
                 //text = string.format("%02d", timer.get()%60)
                 behavior = rt_update ? [Behaviors.RtPropUpdate] : null
                 update = @() {text = string.format("%02d", timer.get()%60)}
                 rtRecalcLayout = rtRecalcLayout
               })
   }
  return @() {
    rendObj = ROBJ_SOLID
    size = size
    pos = pos
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    color = showChild[val].get() ? color : Color(0,0,0)

    onClick = function() {
      showChild[val].set(!showChild[val].get())
    }
    watch = showChild[val]
    children = showChild[val].get() ? children : null
    rtRecalcLayout = rtRecalcLayout
    behavior =  rt_update ? [Behaviors.RtPropUpdate, Behaviors.Button] : [Behaviors.Button]
    update = @() {
      size = calcLayout ? [sh(math.rand()*15/math.RAND_MAX+2), sh(math.rand()*15/math.RAND_MAX+2) / (useFlow ? 10 : 1)] : size
      pos = calcLayout
        ? useFlow
          ? null
          : [sw(math.rand()*80/math.RAND_MAX), sh(math.rand()*80/math.RAND_MAX)]
        : pos
      transform = rtTransform ? {
        translate = [math.rand()*10/math.RAND_MAX, math.rand()*10/math.RAND_MAX]
      }: null
    }
  }
}

function benchmark() {
  local children = null
  let flow = useFlow ? FLOW_VERTICAL : null
  children = []

  for (local i=0; i<componentsNum; ++i) {
    showChild.append(Watched(true))
    children.append(simpleComponent(i))
  }

  let desc = {
    size = flex()
    flow = flow
    children = children
    cursor = cursors.active
  }

  return desc
}

return benchmark
