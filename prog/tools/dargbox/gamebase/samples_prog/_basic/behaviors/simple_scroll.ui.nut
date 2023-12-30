from "%darg/ui_imports.nut" import *
from "%darg/laconic.nut" import *
from "math" import max

require("daRg").gui_scene.config.kbCursorControl = true
let fa = require("samples_prog/_basic/goodies/fontawesome.map.nut")

let mkDummy = @(_, i) comp(Size(sh(10)), Button, RendObj(ROBJ_BOX), FillColr(50,60,60), txt(i), HACenter, VACenter,
Style({xmbNode = XmbNode({})}))

/*
todo:
  animated scroll?
  selectXmbNode and scroll to it?
*/
let DEF_SIDE_SCROLL_OPTIONS = freeze({
  orientation = O_VERTICAL
  size = flex()
  maxWidth = null
  maxHeight = null
  needReservePlace = true //need reserve place for scrollbar when it not visible
  clipChildren  = true
  joystickScroll = true
  rootStyle = null
  percentToScroll = 10
})

let defFontStyle = freeze({font = Fonts?.fontawesome, fontSize = sh(5)})

function mkBtn(handler, style = null){
  let {up=true, fontStyle = defFontStyle, trigger= {}} = style
  let animations = [
    { prop=AnimProp.translate, from=[0, up ? fsh(1) : -fsh(1)], to=[0,0], duration=0.25, trigger, easing=InOutCubic}
    { prop=AnimProp.translate, from=[0, up ? fsh(3) : -fsh(3)], to=[0,0], duration=0.35, play=true, easing=InOutCubic}
    { prop=AnimProp.opacity, from=0, to=1, duration=0.25, play=true, easing=OutCubic}
  ]
  let children = {
    key = up
    animations
    transform = {}
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    children = [
      txt(fa["circle"], fontStyle.__merge({
        color = Color(80,80,80, 250)
        fontFxColor = Color(0,0,0,60)
        fontFxFactor = max(64, hdpx(64))
        fontFxOffsY = hdpx(1)
        fontFx = FFT_GLOW
      }))
      txt(fa[up ? "arrow-up" : "arrow-down"], fontStyle.__merge({color = Color(230,230,230) fontSize = fontStyle.fontSize*0.66}))
    ]
  }
  return {
    key = up
    onClick = handler
    behavior = Behaviors.Button
    halign = ALIGN_CENTER
    valign = ALIGN_CENTER
    skipDirPadNav=true
    transform = {}
    children
    hplace = ALIGN_CENTER
  }.__update(style ?? {})
}

function makeScroll(content, options = DEF_SIDE_SCROLL_OPTIONS) {
  options = DEF_SIDE_SCROLL_OPTIONS.__merge(options)
  let {percentToScroll} = options
  let scrollHandler = options?.scrollHandler ?? ScrollHandler()
  let scrollVal = Watched(0)
  let elemSizeV = Watched(0)
  let contentSizeVal = Watched(0)
  let showUpBtn = Computed(@() scrollVal.value > 0 )
  let showDnBtn = Computed(@() contentSizeVal.value - (scrollVal.value + elemSizeV.value) > 0 )
  let triggerUp = {}
  let triggerDn = {}
  let scrollStep = Computed(@() elemSizeV.value/percentToScroll)
  let upBtn = mkBtn(@() scrollHandler.scrollToY(scrollVal.value - scrollStep.value), {pos = [0, fsh(0.5)] trigger = triggerUp})
  let dnBtn = mkBtn(@() scrollHandler.scrollToY(scrollVal.value + scrollStep.value), {pos = [0, -fsh(0.5)] up=false, vplace = ALIGN_BOTTOM trigger = triggerDn})
  return @() {
    size = options.size
    clipChildren = true
    watch = [showUpBtn,showDnBtn]
    children = [
      {
        rendObj = ROBJ_MASK
        children = content
        maxWidth = options.maxWidth
        maxHeight = options.maxHeight
        size = options.size
        xmbNode = XmbContainer()
        orientation = options.orientation
        wheelStep = 0.8
        joystickScroll = options.joystickScroll
        scrollHandler
        behavior = [Behaviors.WheelScroll, Behaviors.ScrollEvent, Behaviors.Pannable]
        onScroll = function(elem) {
          let nScroll = elem?.getScrollOffsY() ?? 0
          if (showDnBtn.value && nScroll > scrollVal.value)
            anim_start(triggerDn)
          if (showUpBtn.value && nScroll < scrollVal.value)
            anim_start(triggerUp)
          scrollVal(nScroll)
          elemSizeV(elem?.getHeight() ?? 0)
          contentSizeVal(elem?.getContentHeight() ?? 0)
        }
      }.__update(options.rootStyle ?? {}),
      showUpBtn.value ? upBtn : null,
      showDnBtn.value ? dnBtn : null
    ]
  }
}


return comp(
  Size(sh(20), sh(60))
  HCenter
  VCenter
  RendObj(ROBJ_FRAME)
  makeScroll(
    vflow(
      RendObj(ROBJ_FRAME)
      Colr(120,200,120)
      Padding(hdpx(10))
      Gap(hdpx(10))
      array(15).map(mkDummy)
    )
    {rootStyle = comp( HACenter, RendObj(ROBJ_SOLID), Colr(60,20,20))}
  )
)