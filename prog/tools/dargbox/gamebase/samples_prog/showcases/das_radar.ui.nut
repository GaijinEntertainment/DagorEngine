from "%darg/ui_imports.nut" import *

import "samples_prog/_cursors.nut" as cursors

let dasScript = load_das("samples_prog/showcases/das_radar.das")

let testContent = {
  size = flex()
  padding = sh(5)

  children = [
    {
      size = [sh(40), sh(40)]
      pos = [sh(10), sh(10)]
      rendObj = ROBJ_DAS_CANVAS
      script = dasScript
      drawFunc = "draw_with_params"
      setupFunc = "setup_data"
      dasTextColor = Color(200,250,50)
    }

    {
      hplace = ALIGN_RIGHT
      vplace = ALIGN_BOTTOM
      size = [sh(40), sh(22)]
      rendObj = ROBJ_DAS_CANVAS
      script = dasScript
      drawFunc = "draw_radar_b_scope"
    }
  ]
}


return function() {
  return {
    size = flex()
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    children = testContent
    cursor = cursors.normal
  }
}
