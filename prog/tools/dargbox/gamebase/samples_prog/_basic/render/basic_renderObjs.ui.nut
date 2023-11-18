from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let cup = Picture("!ui/ca_cup1.png")
let button_image = Picture("!ui/button.png")

/*
  - circular progress
*/


let function sampleDebug() {
  return {
    rendObj = ROBJ_DEBUG
    color = Color(255,230,200)
    size = [100,30]
  }
}


let function sampleSolid() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(250,25,205)
    size = [100,30]
  }
}


let function sampleFrameP(borderWidth=1, color=Color(200,200,200)) {
  return {
    rendObj = ROBJ_FRAME
    color = color
    size = [40,30]
    borderWidth = borderWidth
  }
}

let function sampleBox() {
  return {
    rendObj = ROBJ_BOX
    fillColor = Color(100,50,50)
    borderColor = Color(100,100,200)
    borderWidth = [4,1,4,1]
    size = [100,30]
  }
}

let function labeledElem(elem,text) {
  return {
    flow = FLOW_HORIZONTAL
    gap = 20
    size = flex()
//    rendObj  = ROBJ_FRAME
    valign = ALIGN_CENTER
    children = [
      {
        size = [200, 30]
        halign= ALIGN_CENTER
        valign =ALIGN_CENTER
        children = elem
      }
      {
        rendObj = ROBJ_TEXTAREA
        behavior = Behaviors.TextArea
        size = [flex(),SIZE_TO_CONTENT]
        text = text
        valign=ALIGN_CENTER
      }
    ]
  }
}

let function sampleImageP(size, image = cup) {
  return {
    size = size
    rendObj = ROBJ_IMAGE
    image
  }
}

let images = {
  flow = FLOW_HORIZONTAL
  size = flex()
  halign = ALIGN_CENTER
  gap = 10
  children = [
    sampleImageP([30,30])
    sampleImageP([30,SIZE_TO_CONTENT])
    sampleImageP([SIZE_TO_CONTENT, 30])
  ]

}

let vector_canvas = {
  flow = FLOW_HORIZONTAL
  size = flex()
  halign = ALIGN_CENTER
  gap = 10
  children = [
    {
      rendObj = ROBJ_VECTOR_CANVAS
      size = [50, 50]
      lineWidth = 2.5
      color = Color(50, 200, 255)
      fillColor = Color(122, 1, 0, 0)
      commands = [
        [VECTOR_WIDTH, 2.2],                   // set current line width
        [VECTOR_LINE, 0, 0, 100, 25],          // fromX, fromY, toX, toY
        [VECTOR_WIDTH, 2.2],                   // set current line width
        [VECTOR_LINE, 100, 25, 0, 50],          // fromX, fromY, toX, toY
        [VECTOR_WIDTH, 1.2],                   // set current line width
        [VECTOR_LINE, 0, 50, 100, 75],          // fromX, fromY, toX, toY
        [VECTOR_WIDTH, 2.2],                   // set current line width
        [VECTOR_LINE, 100, 75, 0, 100],          // fromX, fromY, toX, toY
      ]
    }
    {
      rendObj = ROBJ_VECTOR_CANVAS
      size = [50, 50]
      lineWidth = 2.0
      color = Color(50, 200, 155)
      fillColor = Color(122, 105, 0, 0)
      commands = [
        [VECTOR_ELLIPSE, 50, 50, 60, 40],      // centerX, centerY, radiusX, radiusY
      ]
    }
    {
      rendObj = ROBJ_VECTOR_CANVAS
      size = [50, 50]
      lineWidth = 0.5
      color = Color(50, 200, 255)
      fillColor = Color(122, 1, 0, 0)
      commands = [
        [VECTOR_RECTANGLE, 0, 25, 100, 50],      // minX, minY, maxX, maxY
      ]
    }
  ]
}


let frames = {
  flow = FLOW_HORIZONTAL
  size = flex()
  halign = ALIGN_CENTER
  gap = 10
  children = [
    sampleFrameP(3, Color(0,205,205))
    sampleFrameP([2,0], Color(0,205,205))
    sampleFrameP([0,0,3,0], Color(100,25,205))
  ]

}
let nine_rect = {
  size = [100, 50]
  rendObj = ROBJ_9RECT
  image = button_image
  screenOffs = [hdpx(4),hdpx(4),hdpx(5),hdpx(5)]
  texOffs = [4,4,5,4]
}

let sampleRoundedBox = {
  rendObj = ROBJ_BOX
  fillColor = Color(100,50,50)
  borderColor = Color(100,100,200)
  borderWidth = 2
  borderRadius = [2,2,2,2]

  size = [100,30]
}

let sampleRoundedImage = {
  rendObj = ROBJ_BOX
  image = cup
  fillColor = Color(100,50,50)
  borderColor = Color(100,100,200)
  borderWidth = 2
  borderRadius = [8,4,5,1]

  size = [100,30]
}


let function basicsRoot() {
  return {
    rendObj = ROBJ_SOLID
    color = Color(30,40,50)
    cursor = cursors.normal
    size = flex()
    padding = 50
    children = [
      {
        valign = ALIGN_CENTER
        halign = ALIGN_LEFT
        flow = FLOW_VERTICAL
        size = flex()
        gap = 40
        children = [
          labeledElem(sampleDebug, "'ROBJ_DEBUG' rendObj. Has just one 'color' as render properties")
          labeledElem(sampleSolid, "'ROBJ_SOLID' rendObj. Has just one 'color' as render properties")
          labeledElem(frames, "'ROBJ_FRAME' rendObj. Has 'color' as border color and 'borderWidth' (1=2 ; 2=[2,0], 3=[0,0,2,0] )")
          labeledElem(sampleBox, "'ROBJ_BOX'. Has 'fillColor', 'borderColor' and 'borderWidth' (= [2,1,2,1] here)")
          labeledElem(sampleRoundedBox, "'ROBJ_BOX'. Has 'fillColor', 'borderColor', 'borderWidth' (= 2 here), borderRadius (= [2,2,2,2] here)")
          labeledElem(sampleRoundedImage, "'ROBJ_BOX'. Has 'fillColor', 'borderColor', 'borderWidth' (= 2 here), borderRadius (= [8, 4, 5, 1] and image here)")
          labeledElem(images, "'ROBJ_IMAGE'. 1: size =[30, 30], 2: [30, SIZE_TO_CONTENT] (width=30, height=aspect_ratio*30), 3: [SIZE_TO_CONTENT, 30] (height=30, width=30/aspect_ratio) ")
          labeledElem(nine_rect, "ROBJ_9RECT. Has 'image', 'screenOffs' and 'texOffs' properties")
          labeledElem(vector_canvas, "ROBJ_VECTOR_CANVAS. Has 'lineWidth', 'color', 'fillColor' and 'commands' properties (like VECTOR_LINE, VECTOR_ELLIPSE and VECTOR_RECTANGL ")
        ]
      }
    ]
  }
}

return basicsRoot
