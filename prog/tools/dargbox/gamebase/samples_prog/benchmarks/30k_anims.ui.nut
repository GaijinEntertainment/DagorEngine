from "%darg/ui_imports.nut" import *
let cursors = require("samples_prog/_cursors.nut")

//inspired by https://www.youtube.com/watch?v=yy8jQgmhbAU
let maxX=400

let animations = [
  { prop=AnimProp.fillColor, from=Color(255,255,255) to=Color(255,0,0) duration = 2, easing=Linear, play=true, loop=true }
  { prop=AnimProp.translate, from=[0,0] to= [-hdpx(100),0], duration = 1, easing=Linear, play=true, loop=true }
  { prop=AnimProp.scale, from=[1,1] to= [1.1,0.9], duration = 1, easing=Linear, play=true, loop=true }
]
let white =Color(255,255,255)
let size = [hdpx(10), hdpx(10)]
function mkRect(i){
  let x = i%maxX*size[0]*1.4
  let y = i/maxX*size[0]*1.4

  return {
    rendObj=ROBJ_BOX
    borderWidth=0
    fillColor=white
    size = size
    pos = [x,y]
    transform = {}
    animations= animations
  }
}

let isStarted = Watched(false)

function root() {
  local children
  if (isStarted.value)
    children = array(30000).map(@(_,i) mkRect(i))
  else {
    children = {
      rendObj = ROBJ_TEXT
      size = flex()
      valign = ALIGN_CENTER
      halign = ALIGN_CENTER
      text = "Click to start"
    }
  }

  return {
    watch = isStarted
    size = flex()
    children
    behavior = Behaviors.Button
    cursor = cursors.normal

    function onClick() {
      isStarted(true)
    }
  }
}

return root
