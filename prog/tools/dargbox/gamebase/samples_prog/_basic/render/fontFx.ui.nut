from "%darg/ui_imports.nut" import *

let variants = [
  {
    text = "FFT_NONE"
    font = 2
    color = 0x0000FF
  }
  {
    text = "FFT_SHADOW, 24"
    font = 2
    color = 0x0000FF
    fontFxColor = 0x00FF00
    fontFxFactor = 24
    fontFx = FFT_SHADOW
    fontFxOffsX = 1
    fontFxOffsY = 1
  }
  {
    text = "FFT_GLOW, 64"
    font = 2
    color = 0x0000FF
    fontFxColor = 0x00FF00
    fontFxFactor = 64
    fontFx = FFT_GLOW
  }
  {
    text = "FFT_BLUR, 64"
    font = 2
    color = 0x0000FF
    fontFxColor = 0x00FF00
    fontFxFactor = 64
    fontFx = FFT_BLUR
  }
  {
    text = "FFT_OUTLINE, 32"
    font = 2
    color = 0x0000FF
    fontFxColor = 0x00FF00
    fontFxFactor = 64
    fontFx = FFT_OUTLINE
  }
]

let children = []
foreach(i, v in variants) {
  if (i)
    children.append({ size = [pw(70), 1], rendObj = ROBJ_SOLID, color = Color(110, 110, 1100, 150), margin = 1 })
  children.append(v.__merge({ rendObj=ROBJ_TEXT, text = $"text: {v.text}" }))
  children.append(v.__merge({ rendObj=ROBJ_TEXTAREA, behavior = Behaviors.TextArea, halign = ALIGN_CENTER, text = $"textarea: {v.text}"}))
}

return {
  size = SIZE_TO_CONTENT
  rendObj = ROBJ_SOLID
  color = 0x808080
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  halign = ALIGN_CENTER
  flow = FLOW_VERTICAL
  padding = 10
  gap = 5
  children = children
}
