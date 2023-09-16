from "%darg/ui_imports.nut" import *

let utf8 = require("utf8")

let mkAnimation = function(delay) {
  let trigger = {}
  let totalD = delay+0.3
  return [
    { prop=AnimProp.translate, from=[0,-sh(30)], to=[0,0], duration=0.2, easing=InCubic, play=true, delay}
//    { prop=AnimProp.opacity, from=0, to=1, duration=0.3, easing=InCubic, trigger}
    { prop=AnimProp.opacity, from=0, to=1, duration=totalD, easing=function(t) {
        let ct = t*totalD
        if (ct<delay)
          return 0
        local s = (ct-delay)/0.3
        s = s*s
        return s*s
      }, play=true, onFinish = trigger}

    { prop=AnimProp.translate, from=[0,0], to=[0,sh(20)], duration=0.2, easing=InCubic, playFadeOut=true, delay=delay/2}
    { prop=AnimProp.opacity, from=1, to=0, duration=0.18, easing=InCubic, playFadeOut=true, delay=delay/2}
  //  { prop=AnimProp.opacity, from=1, to=1, duration=delay/2, easing=OutCubic, playFadeOut=true}
  ]
}
let function mkAnim(ch, i, total){
  let l = total.len()
  let delay = i*max(0.1, min(2.0/l, 0.3))
  return {
    rendObj = ROBJ_TEXT
    text = ch
    key = {}
    animations = mkAnimation(delay)
    fontSize = hdpx(100)
    transform = {}
  }
}
let show = Watched(true)
let function mkAnimText(txt) {
  let ut = utf8(txt)
  let chars = []
  for(local i=1; i <= ut.charCount(); i++){
    chars.append(ut.slice(i-1, i))
  }
  return @(){
    flow  = FLOW_HORIZONTAL
    watch = show
    children = show.value ? chars.map(mkAnim) : null
  }
}

let function root() {
  return {
    valign = ALIGN_CENTER
    halign= ALIGN_CENTER
    flow = FLOW_VERTICAL
    size=flex()
    behavior = Behaviors.Button
    onClick = @() dlog(show.value) ?? show(!show.value)
    children = mkAnimText("Выживи. Убей. Прославься!")
  }
}


return root
