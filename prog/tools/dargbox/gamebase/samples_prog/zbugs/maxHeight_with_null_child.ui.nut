from "%darg/ui_imports.nut" import *

let child0 = @() {
	minHeight = hdpx(220)
	children = null
}

let child1 = @() {
	minHeight = hdpx(220)
	children = { size = flex() }
}

let box = @(child, text) {
  rendObj = ROBJ_BOX
  fillColor = Color(40,40,40)
  borderWidth = hdpx(1)
  borderColor = Color(100,100,100)
  gap = hdpx(12)
  padding = hdpx(12)
  flow = FLOW_VERTICAL
  children = [
    child
    {
      rendObj = ROBJ_TEXT
      color = Color(240,240,240)
      text = $"{text} 1"
    }
    {
      rendObj = ROBJ_TEXT
      color = Color(240,240,240)
      text = $"{text} 2"
    }
  ]
}

return {
  size = flex()
  flow = FLOW_HORIZONTAL
  gap = hdpx(100)
  halign = ALIGN_CENTER
  children = [
    box(child0, "Sibling of null")
    box(child1, "Sibling of {}")
  ]
}
