from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let iconWidgetDef = {
  width = hdpx(64)
  height = hdpx(64)
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
}

let playPause = mkWatched(persist, "playPause", true)
let cachedPictures = mkWatched(persist, "cachedPictures",{})

function getPicture(source) {
  local pic = cachedPictures.get()?[source]
  if (pic)
    return pic
  pic = LottieAnimation(source)
  cachedPictures.get()[source] <- pic
  return pic
}

function lottieWidget(item, params=iconWidgetDef) {
  let children = params?.children
  let lottie = item?.lottie ?? ""
  let width = item?.width ?? iconWidgetDef.width
  let height = item?.height ?? iconWidgetDef.height
  let vplace = params?.vplace ?? iconWidgetDef.vplace
  let hplace = params?.hplace ?? iconWidgetDef.hplace
  let canvasHeight = item?.canvasHeight ?? height
  let canvasWidth = item?.canvasWidth ?? width
  let play = item?.play ?? true
  let loop = item?.loop ?? false
  let rate = item?.rate ?? -1
  let prerenderedFrames = item?.prerenderedFrames ?? -1

  let tbl = {
    animation = lottie
    canvasHeight = canvasHeight
    canvasWidth = canvasWidth
    loop = loop
    play = play
    rate = rate
    prerenderedFrames = prerenderedFrames
  }

  let imageSource = @"ui/atlas#render{
    lottie:t={animation}
    canvasWidth:i={canvasWidth};canvasHeight:i={canvasHeight};
    loop:b={loop}; play:b={play}; rate:i={rate}; prerenderedFrames:i={prerenderedFrames};
  }.render".subst(tbl)

  let image = getPicture(imageSource)

  return @() {
    watch = playPause
    rendObj = ROBJ_IMAGE
    image = image
    key = image
    animated = true
    vplace = vplace
    hplace = hplace
    children = children
    size = [width,height]
    keepAspect = true
    play = playPause.get()
  }.__update(params)
}

let mkLine = @() {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = SIZE_TO_CONTENT
  flow = FLOW_HORIZONTAL

  children = [
    lottieWidget({lottie="ui/lottie/glow_loading.json", width=200, height=200, loop=true, rate=1, prerenderedFrames=4})
    lottieWidget({lottie="ui/lottie/glow_loading.json", width=200, height=200, loop=true, rate=2, prerenderedFrames=8})
    lottieWidget({lottie="ui/lottie/glow_loading.json", width=200, height=200, loop=true, rate=4, prerenderedFrames=16})
    lottieWidget({lottie="ui/lottie/glow_loading.json", width=200, height=200, loop=true, rate=8, prerenderedFrames=32})
  ]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  cursor = cursors.normal
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  size = SIZE_TO_CONTENT
  flow = FLOW_VERTICAL

  children = [
    mkLine()
  ]
}

