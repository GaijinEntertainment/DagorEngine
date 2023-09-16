from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let function sampleSText(params={}) {
  return {
    rendObj = ROBJ_INSCRIPTION
    color = Color(255,255,55)
    size = [400,SIZE_TO_CONTENT]
    text = "Static Text (inscription)"
//    fontScale = 0.65 //scale just scale text, not render it in new resolution
    fontSize = hdpx(10)
  }.__update(params)
}

let function sampleDText(params={}) {
  return {
    rendObj = ROBJ_TEXT
    color = Color(105,255,155)
    size = [400,SIZE_TO_CONTENT]
    text = "Dynamic Text (font atlas based)"
  }.__update(params)
}


let function labeledElem(elem,text,vert_size=36) {
  return {
    flow = FLOW_HORIZONTAL
    gap = 20
    size = [flex(), vert_size]
    //rendObj  = ROBJ_FRAME
    valign = ALIGN_CENTER
    children = [
      {
        size = [400, vert_size]
        halign= ALIGN_CENTER
        valign =ALIGN_CENTER
        children = elem
      }
      {
        rendObj = ROBJ_TEXTAREA
        behavior = Behaviors.TextArea
        size = flex()
        text = text
      }
    ]
  }
}

let sampleSTextFxGlow = @() {
  text = "Static Text with Glow 48"
  rendObj = ROBJ_INSCRIPTION
  size = [400,SIZE_TO_CONTENT]
  fontFxColor = Color(255, 155, 0, 0)
  fontFxFactor = 128
  fontFx = FFT_GLOW
  color = Color(255,255,255)
  padding = 5
}

let function sampleHAlignedCenterDText(text) {
  return {
    rendObj = ROBJ_TEXT
    halign = ALIGN_CENTER
    text = text
    size = [flex(), flex()]
  }
}

let function sampleHAlignedRightSText(text) {
  return {
    rendObj = ROBJ_INSCRIPTION
    halign = ALIGN_RIGHT
    valign = ALIGN_CENTER
    text = text
    size = [flex(), flex()]
  }
}

let function basicsRoot() {
  return {
    rendObj = ROBJ_SOLID
    size = flex()
    color = Color(30,40,50)
    cursor = cursors.normal
    padding = 50
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_LEFT
        flow = FLOW_VERTICAL
        size = flex()
        gap = 20
        children = [
          labeledElem(sampleDText({fontFxColor = Color(255, 155, 0, 0) fontScale=1.2 color =Color(255,255,255)}), "'ROBJ_TEXT'. Dynamic text. Has 'color', 'font', 'scale' and 'text' and fx properties. Font is one font from fonts.blk")
          labeledElem(sampleDText({fontFxColor = Color(255, 155, 0, 0) fontFxFactor = 64 padding = 3 fontFx = FFT_GLOW color =Color(255,255,255)}), "!!!req more info!!! 'ROBJ_TEXT'. Dynamic text. Has 'color', 'font' and 'text' properties. Font is one font from fonts.blk")
          labeledElem(sampleSTextFxGlow, "'ROBJ_INSCRIPTION'. Static text. Has 'color', 'font' and 'text' properties. Supports hinting, pair kerning, font size, font properties, FXs")
          labeledElem(sampleSText({fontFxColor = Color(255, 155, 0, 0) fontFxFactor = 64 padding = 3 fontFx = FFT_GLOW color =Color(255,255,255)}), "'ROBJ_INSCRIPTION'. Static text. Has 'color', 'font' and 'text' properties. Supports hinting, pair kerning, font size, font properties, FXs")

          labeledElem(sampleDText({fontFxColor = Color(255, 155, 0, 0) color =Color(255,255,255) monoWidth="N"}), "'ROBJ_TEXT'. Dynamic text. monoWidth=\"N\"")
          labeledElem(sampleDText({fontFxColor = Color(255, 155, 0, 0) color =Color(255,255,255) monoWidth=sh(1)}), "'ROBJ_TEXT'. Dynamic text. monoWidth=sh(1)")
          labeledElem(sampleDText({fontFxColor = Color(255, 155, 0, 0) color =Color(255,255,255) spacing=sh(1)}), "'ROBJ_TEXT'. Dynamic text. spacing=sh(1)")
          labeledElem(sampleHAlignedCenterDText("Horiz aligned dynamic text"), "'ROBJ_TEXT'. Dynamic text. halign=ALIGN_CENTER")
          labeledElem(sampleHAlignedRightSText("Horiz aligned static text"), "'ROBJ_INSCRIPTION'. Static text. halign=ALIGN_RIGHT")
          { size=[flex(), 50] flow=FLOW_HORIZONTAL gap = 20 children = [
              {text = "Vert aligned static text" rendObj = ROBJ_INSCRIPTION valign = ALIGN_CENTER  size = [400,flex()] children={ rendObj=ROBJ_FRAME size=flex()}}
              {text="'ROBJ_TEXT'. Dynamic text. valign=ALIGN_CENTER" rendObj=ROBJ_TEXT }
            ]
          }
          { size=[flex(), 50] flow=FLOW_HORIZONTAL gap = 20 children = [
              {text = "Vert aligned dynamic text" rendObj = ROBJ_TEXT valign = ALIGN_BOTTOM  size = [400,flex()] children={ rendObj=ROBJ_FRAME size=flex()}}
              {text="'ROBJ_TEXT'. Dynamic text. valign=ALIGN_BOTTOM" rendObj=ROBJ_INSCRIPTION }
            ]
          }
        ]
      }
    ]
  }
}

return basicsRoot
