from "%darg/ui_imports.nut" import *
from "datetime" import clock
from "math" import sin

let cursors = require("samples_prog/_cursors.nut")

let canvas = {
  rendObj = ROBJ_VECTOR_CANVAS
  size = [sh(80), sh(80)]

  function draw(ctx, rect) {
    let t = clock()
    let d = sh(5)
    let {w,h} = rect

    ctx.fill_poly([-d, -d, w-d, d, w+d, h+d, d, h-d],
      3, Color(100,220,160), Color(10,50,10,50))

    ctx.rect(d, d, w-d, h-d,
      5, Color(200,20,20), Color(200,0,0), Color(180+30*sin(t),180,180))

    ctx.ellipse(w-d, h-d, sh(7.5), sh(7.5),
      10, Color(200,20,20), Color(200,20,20), Color(180,180,120))

    ctx.sector(w*0.4, h*0.4, sh(20), sh(10), t, 2*t,
      3, Color(120, 200, 80), Color(100, 180, 70), Color(20,80,20))

    ctx.line([20,20, 30,30, 40,20, 50,30, 60,20, 70, 30],
      5, Color(255,255,20))

    ctx.line_dashed([20,rect.h-20, rect.w-20, 20],
      sh(5), sh(2.5),
      5, Color(255,255,20))
  }
}



let root = {
  rendObj = ROBJ_SOLID
  color = Color(30, 40, 50)
  cursor = cursors.normal
  size = flex()
  valign = ALIGN_CENTER
  halign = ALIGN_CENTER
  children = {
    padding = sh(5)
    rendObj = ROBJ_SOLID
    color = Color(0,0,0)
    children = canvas
  }
}

return root
