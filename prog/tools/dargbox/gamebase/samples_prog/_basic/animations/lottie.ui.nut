from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

let iconWidgetDef = {
  width = hdpx(64)
  height = hdpx(64)
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
}

let playPause = persist("playPause", @() Watched(true)) //or model
let cachedPictures = persist("cachedPictures", @() Watched({}))

function getPicture(source) {
  local pic = cachedPictures.value?[source]
  if (pic)
    return pic
  pic = LottieAnimation(source)
  cachedPictures.value[source] <- pic
  return pic
}

function button() {
  return watchElemState(function(sf) {
    return{
      watch = playPause
      rendObj = ROBJ_BOX
      size = [sh(20),SIZE_TO_CONTENT]
      padding = sh(2)
      fillColor = (sf & S_ACTIVE) ? Color(0,0,0) : Color(200,200,200)
      borderWidth = (sf & S_HOVER) ? 2 : 0
      behavior = [Behaviors.Button]
      halign = ALIGN_CENTER
      onClick = @() playPause.update(!playPause.value)
      hotkeys = [["^Space", @() playPause.update(!playPause.value)]]
      children = {
        rendObj = ROBJ_TEXT
        text = playPause.value ? "Play" : "Paused"
        color = (sf & S_ACTIVE) ? Color(255,255,255): Color(0,0,0)
        pos = (sf & S_ACTIVE) ? [0,2] : [0,0] }
    }
  })
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

  let tbl = {
    animation = lottie
    canvasHeight = canvasHeight
    canvasWidth = canvasWidth
    loop = loop
    play = play
  }

  let imageSource = @"ui/atlas#render{
    lottie:t={animation}
    canvasWidth:i={canvasWidth};canvasHeight:i={canvasHeight};
    loop:b={loop}; play:b={play};
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
    play = playPause.value
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
    lottieWidget({lottie="ui/lottie/abstract_circle.json", width=hdpx(128), height=hdpx(128), loop=true})
    lottieWidget({lottie="ui/lottie/cuisine.json", width=11, height=11, loop=true})
    lottieWidget({lottie="ui/lottie/acrobatics.json", loop=true})
    lottieWidget({lottie="ui/lottie/bell.json", loop=true})
    lottieWidget({lottie="ui/lottie/birth_stone_logo.json", loop=true})
    lottieWidget({lottie="ui/lottie/done.json", loop=true})
    lottieWidget({lottie="ui/lottie/emoji_wink.json", loop=true})
    lottieWidget({lottie="ui/lottie/fingerprint_success.json", loop=true})
    lottieWidget({lottie="ui/lottie/gears_or_settings.json", loop=true})
    lottieWidget({lottie="ui/lottie/glow_loading.json", loop=true})
    lottieWidget({lottie="ui/lottie/gradient_sleepy_loader.json", loop=true})
    lottieWidget({lottie="ui/lottie/Indicators1.json", loop=true})
    lottieWidget({lottie="ui/lottie/loader.json", loop=true})
    lottieWidget({lottie="ui/lottie/loading_animation.json", loop=true})
    lottieWidget({lottie="ui/lottie/movie_loading.json", loop=true})
    lottieWidget({lottie="ui/lottie/ripple_loading_animation.json", loop=true})
    lottieWidget({lottie="ui/lottie/telegram.json", loop=true})
    lottieWidget({lottie="ui/lottie/triib_manage.json", loop=true})
    lottieWidget({lottie="ui/lottie/alarm.json", loop=true})
    lottieWidget({lottie="ui/lottie/anubis.json", loop=true})
    lottieWidget({lottie="ui/lottie/firework.json", loop=true})
    lottieWidget({lottie="ui/lottie/360degree.json", loop=true})
    lottieWidget({lottie="ui/lottie/honey.json", loop=true})
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
    mkLine()
    mkLine()
    mkLine()
    mkLine()
    mkLine()
    mkLine()
    button()
  ]
}

