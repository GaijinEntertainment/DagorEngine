from "%darg/ui_imports.nut" import *

let defControlHeight = calc_str_box("A")[1]

return freeze({
  HOVER_TXT = Color(255,200,100)
  SELECTED_TXT = Color(255,255,255)
  NORMAL_TXT = Color(150,150,150)
  C_FONT_SIZE = hdpx(50)
  H_FONT_SIZE = hdpx(70)
  NORMAL_FONT_SIZE = hdpx(30)
  backgroundColor = Color(30,40,60)
  BTN_BD_NORMAL = Color(100,100,100,255)
  BTN_BG_NORMAL = Color(20,20,20,255)
  BTN_BG_HOVER = Color(250,200,150,255)
  BTN_TXT_NORMAL = Color(150,150,150,255)
  BTN_TXT_HOVER = Color(0,0,0,255)
  BTN_TXT_DISABLED = Color(90,80,80)

  LABEL_TXT = Color(100,100,100,100)
  defControlHeight
  SelectWidth = sw(15)
})
