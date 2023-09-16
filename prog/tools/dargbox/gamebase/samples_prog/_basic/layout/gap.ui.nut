from "%darg/ui_imports.nut" import *

let box = {
  rendObj = ROBJ_SOLID
  color = Color(250,230,100)
  size = flex(1)
}

let panel1 = {
  rendObj = ROBJ_BOX
  fillColor = Color(50,0,0)
  borderColor = Color(180,180,180)
  size = flex()

  padding = sh(5)
  gap = sh(5) // fixed gap

  flow = FLOW_VERTICAL
  children = [box, box, box, box]
}

let panel2 = {
  rendObj = ROBJ_BOX
  fillColor = Color(0,50,0)
  borderColor = Color(180,180,180)
  size = flex()

  padding = sh(5)

  // custom component gap
  gap = {
    rendObj = ROBJ_SOLID
    color = Color(50,50,20,200)
    size = flex(1) // it is the same-class child, so all its layout is customizable
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    // it can even have children!
    children = {
      size = [flex(), 5]
      rendObj = ROBJ_SOLID
      color = Color(0,0,0)
    }
  }

  flow = FLOW_VERTICAL
  children = [box, box, box, box]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()

  flow = FLOW_HORIZONTAL
  gap = sh(10) // fixed gap
  padding = sh(10)
  children = [
    panel1
    panel2
  ]
}
