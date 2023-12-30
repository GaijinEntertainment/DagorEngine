from "%darg/ui_imports.nut" import *

function elem() {
  return {
    flow  = FLOW_VERTICAL
    hplace = ALIGN_CENTER
    vplace = ALIGN_TOP
    halign = ALIGN_CENTER
    pos = [0, 100]
    size = [sh(22),300]
    children =
      {
        flow = FLOW_HORIZONTAL
        valign= ALIGN_CENTER
        gap = sh(1)
        size = [flex(), SIZE_TO_CONTENT] //todo - min-height should be SIZE_TO_CONTENT, height - flex
        children = [
          {
            rendObj = ROBJ_TEXT
            color = Color(128,128,128,60)
            text = "PERKS AVAILABLE:"
            transform = {pivot = [0.5,0.5]}
            animations = [
              { prop=AnimProp.translate, from=[-sh(20),0], to=[0,0], duration=0.35, play=true, easing=OutQuart}
              { prop=AnimProp.opacity, from=0, to=1, duration=0.75, play=true, easing=OutCubic}
            ]

          }
          {size=[flex(0.01),0]}
          {
            rendObj = ROBJ_TEXT
            color = Color(255,255,255,60)
            text = 66
            transform = {pivot = [0.5,0.5]}
            font = 2
            animations = [{ prop=AnimProp.scale, from=[2.7,2.7], to=[1,1], duration=0.75, play=true, easing=OutCubic }]
          }
        ]
      }
  }
}
/* possible easings
Linear,
InQuad, OutQuad, InOutQuad,
InCubic, OutCubic, InOutCubic,
InQuart, OutQuart, InOutQuart,
inQuintic, outQuintic, inOutQuintic
inElastic, outElastic, inOutElastic
inCirc, outCirc, inOutCirc
inSine, outSine, inOutSine
inExp, outExp, inOutExp
inBack, outBack, inOutBack,
outBounce, inBounce, inOutBounce
InOutBezier, CosineFull, InStep, OutStep,
Descrete8
*/

function root() {
  return {
    valign = ALIGN_CENTER
    flow = FLOW_VERTICAL
    size=flex()
    children = elem
  }
}


return root
