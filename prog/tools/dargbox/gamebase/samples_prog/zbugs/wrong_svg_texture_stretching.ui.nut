from "%darg/ui_imports.nut" import *
let mkBanner = @(cfg) {
  rendObj = ROBJ_IMAGE
  size = cfg.size
  image = Picture("!ui/base_header_bar1.svg:{0}:{1}:K".subst(cfg.imgSize[0].tointeger(), cfg.imgSize[1].tointeger()))
  color = cfg.color
  children = {
    pos = [0, -hdpx(20)]
    rendObj = ROBJ_TEXT
    text = cfg.text
  }
}

let list = [
  {
    size = static [hdpx(150), hdpx(50)]
    imgSize = [hdpx(150), hdpx(50)]
    text = "Pixel perfect"
    color = 0xFFFF0000
  }
  {
    size = static [hdpx(700), hdpx(50)]
    imgSize = [hdpx(700), hdpx(50)]
    text = "Pixel perfect"
    color = 0xFFFF0000
  }
  {
    size = static [hdpx(250), hdpx(50)]
    imgSize = [hdpx(150), hdpx(4)]
    text = "Stretched"
    color = 0xFF00FF00
  }
  {
    size = static [hdpx(700), hdpx(100)]
    imgSize = [hdpx(150), hdpx(4)]
    text = "Stretched"
    color = 0xFF00FF00
  }
  {
    size = static [hdpx(270), hdpx(40)]
    imgSize = [hdpx(300), hdpx(200)]
    text = "Gripped"
    color = 0xFF00FF00
  }
  {
    size = static [hdpx(500), hdpx(120)]
    imgSize = [hdpx(700), hdpx(300)]
    text = "Gripped"
    color = 0xFF00FF00
  }
]

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(3)
  flow = FLOW_VERTICAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = list.map(mkBanner)
}

