// Behaviors.ImageLoadState / mkAsyncImage: animated placeholder while a picture loads.
//
// To slow the load down enough to see it, add a picMgr block to the dargbox settings blk. It
// REPLACES the built-in defaults, so repeat them:
//   picMgr { debugAsyncSleep:i=1500; createAsyncLoadJobMgr:b=yes; dynAtlasLazyAllocDef:b=yes;
//            fatalOnPicLoadFailed:b=no }
// A resident atlas item resolves synchronously and must show no placeholder at all; to get the
// loading case back, change the size suffix to one that has not been rasterized yet.
from "%darg/ui_imports.nut" import *

let cursors = require("samples_prog/_cursors.nut")

// dynamic-atlas item: plain textures in dargbox are resident and resolve synchronously
let imageName = "!ui/atlas#chat.svg:178:178:K"

let imageProps = {
  size = [200, 200]
  keepAspect = KEEP_ASPECT_FIT
  halign = ALIGN_CENTER // centers the placeholder, it lives in this element's children
  valign = ALIGN_CENTER
}

function mkSpinner() {
  return {
    rendObj = ROBJ_SOLID
    size = [30, 30]
    color = Color(120, 170, 220)
    transform = { pivot = [0.5, 0.5] }
    animations = [
      { prop = AnimProp.rotate, from = 0, to = 360, duration = 1.2, play = true, loop = true }
      { prop = AnimProp.opacity, from = 0, to = 1, duration = 0.15, play = true }
      { prop = AnimProp.opacity, from = 1, to = 0, duration = 0.15, playFadeOut = true }
    ]
  }
}

let label = @(text) { rendObj = ROBJ_TEXT, text }

// once scope, never inside a builder: mkAsyncImage owns a Watched
let asyncImage = mkAsyncImage(Picture(imageName), mkSpinner(), imageProps)

// for comparison: draws nothing until the load completes
let plain = imageProps.__merge({ rendObj = ROBJ_IMAGE, image = Picture(imageName) })

let column = @(text, comp) {
  flow = FLOW_VERTICAL
  gap = 10
  halign = ALIGN_CENTER
  children = [label(text), comp]
}

return {
  rendObj = ROBJ_SOLID
  color = Color(30, 40, 50)
  size = flex()
  cursor = cursors.normal
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  gap = 40
  children = [
    column("mkAsyncImage", asyncImage)
    column("plain ROBJ_IMAGE", plain)
  ]
}
