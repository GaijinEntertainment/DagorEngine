//expect:w257
//-file:undefined-global

local OutCubic = 2
local AnimProp = ::aaa

local numAnimations = [
  { prop=AnimProp.opacity, from=0.0, to=0.0, delay=0.0, duration=0.1, play=true easing=OutCubic }
  { prop=AnimProp.scale, from=[2,2], to=[1,1], delay=0.1, duration=0.5, play=true easing=OutCubic }
  { prop=AnimProp.opacity, from=0.0, to=1.0, delay=0.1, duration=0.5, play=true easing=OutCubic }
  { prop=AnimProp.scale, from=[1,1], to=[2,2], delay=0.0, duration=0.1, playFadeOut=true easing=OutCubic }
  { prop=AnimProp.opacity, from=1.0, to=0.0, delay=0.1, duration=0.1, playFadeOut=true easing=OutCubic }
]
local numTextAnimations = [
  { prop=AnimProp.opacity, from=0.0, to=0.0, delay=0.0, duration=0.1, play=true easing=OutCubic }
  { prop=AnimProp.scale, from=[2,2], to=[1,1], delay=0.1, duration=0.5, play=true easing=OutCubic }
  { prop=AnimProp.opacity, from=0.0, to=1.0, delay=0.1, duration=0.5, play=true easing=OutCubic }
  { prop=AnimProp.scale, from=[1,1], to=[2,2], delay=0.0, duration=0.1, playFadeOut=true easing=OutCubic }
  { prop=AnimProp.opacity, from=1.0, to=0.0, delay=0.1, duration=0.1, playFadeOut=true easing=OutCubic }
]

return [numAnimations, numTextAnimations]
