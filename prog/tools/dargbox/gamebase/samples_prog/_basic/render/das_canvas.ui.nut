from "%darg/ui_imports.nut" import *

import "samples_prog/_cursors.nut" as cursors

let dasScript = load_das("samples_prog/_basic/render/das_canvas_demo.das")

let testContent = {
  size = SIZE_TO_CONTENT
  rendObj = ROBJ_SOLID
  color = Color(0,0,50,100)
  padding = sh(5)
  gap = sh(5)
  flow = FLOW_HORIZONTAL

  children = [
    {
      size = [sh(40), sh(40)]
      rendObj = ROBJ_DAS_CANVAS
      script = dasScript
      drawFunc = "draw_with_params"
      setupFunc = "setup_data"
      dasTextColor = Color(200,250,50)
    }

    {
      size = [sh(40), sh(40)]
      rendObj = ROBJ_DAS_CANVAS
      script = dasScript
      drawFunc = "draw_without_params"
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
