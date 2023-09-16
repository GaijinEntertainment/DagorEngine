from "%darg/ui_imports.nut" import *
let txt = @(text) {rendObj = ROBJ_TEXT text margin = hdpx(5)}
return {
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  rendObj = ROBJ_SOLID
  color = Color(0,0,0)
  size = flex()
  children = [
    txt("Russian: РУССКИЙ")
    txt("Japanese: 日本")
    txt("Sunshine: 𝕊𝕦𝕟𝕤𝕙𝕚𝕟𝕖")
    txt("Something: ஆஇண ⶼⶩⶹ థ፼ ఋఋ")
    txt("Roman numerals: VIII Ⅶ ⅶ" )
    txt("Japanese numerals: 3 三" )
  ]
}