from "%darg/ui_imports.nut" import *

let margin = fsh(0.3)
let height = calc_str_box({text="A"})[1] + 2*margin

return {
  gridMargin = margin
  gridHeight = height
  colors = {
    HighlightSuccess = Color(100,255,100)
    HighlightFailure = Color(255,100,100)

    ControlBg = Color(10, 10, 10, 160)
    ReadOnly = Color(80, 80, 80, 80)
    Interactive = Color(80, 120, 200, 80)
    Hover = Color(110, 110, 150, 50)
    Active = Color(100, 120, 200, 120)

    FrameActive = Color(250,200,50,200)
    FrameDefault =  Color(100,100,100,100)

    TextReadOnly = Color(100,100,100)
    TextDefault = Color(255,255,255)
    TextError = Color(255, 20, 20)

    GridBg = [
      Color(15,15,15,140)
      Color(10,10,10,160)
    ]
    GridRowHover = Color(50,50,50,160)

    comboboxBorderColor = Color(60,60,60,255)
    ControlBgOpaque     = Color(28,28,28,240)
    TextHighlight       = Color(220,220,220,160)
    TextHover           = Color(0,0,0)
    TextActive          = Color(120,120,120,120)
  }
}