from "%darg/ui_imports.nut" import *

// Scene root
return {
  // Solid color box
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  // Filling entire parent
  size = flex()

  // Horizontally flowing children aligned to the center
  flow = FLOW_HORIZONTAL
  gap = sh(2)
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER

  // A picture and a text
  children = [
    {
      size = sh(5) // fixed square size
      rendObj = ROBJ_IMAGE
      image= Picture("ui/image.png")
    }
    {
      // sizes to contents by default
      rendObj = ROBJ_TEXT
      text = "Hello World!"
    }
  ]
}
