from "%darg/ui_imports.nut" import *

let imgSize = hdpxi(22)

let images = {
  folder = "browser/images/folder.svg"
  file = "browser/images/file.svg"
  image = "browser/images/image-file.png"
  video = "browser/images/video-file.avif"
  ui = "browser/images/srccode-file.png"
  shader = "browser/images/srccode-file.png"
}
  .map(@(path) Picture($"{path}:{imgSize}:{imgSize}:P"))

let imageExt = ["dds","tga","png","svg","psd","jpg", "avif"]
  .reduce(@(res, v) res.rawset(v, true), {})
let videoExt = ["ogg", "ivf"]
  .reduce(@(res, v) res.rawset(v, true), {})
let shaderExt = ["hlsl"]
  .reduce(@(res, v) res.rawset(v, true), {})

function get_image(filename, isDir=false, image=null) {
  if (image != null) {
    if (type(image)=="string" && image.indexof("/") != null) {
      return Picture(image)
    }
    if (type(image) == "instance")
      return image
    return images?[image] ?? images.file
  }

  if (isDir)
    return images.folder

  let ext = filename.split(".").top()
  if (ext in imageExt)
    return images.image
  if (ext in videoExt)
    return images.video
  if (ext in shaderExt)
    return images.shader

  return images.file
}

function button(filename, params) {
  let {onclick=null, isDir=null} = params
  let stateFlags = Watched(0)
  let image = get_image(filename, isDir, params?.image)
  function color(sf, {current=null}) {
    if (current == true)
      return Color(255,205,255)
    if (sf & S_ACTIVE)
      return Color(255,255,255)
    return Color(125,125,125)
  }
  function component() {
    let sf = stateFlags.get()
    let onClick = onclick ? onclick : function() { vlog("file:", filename)}
    return {
      watch = [stateFlags]
      onElemState = @(s) stateFlags.set(s)
      rendObj = ROBJ_BOX
      fillColor = isDir ? Color(40,40,40) : Color(0,0,0,0)
      borderWidth = (sf & S_HOVER) ? 1 : 0
      borderColor = Color(120,120,120,0)
      size = [flex(),SIZE_TO_CONTENT]
      padding = 1
      margin = 1
      gap = 4
      behavior = [Behaviors.Button]
      onClick = onClick
      flow = FLOW_HORIZONTAL
      valign = ALIGN_CENTER
      children = [
        {
          size = imgSize
          rendObj = ROBJ_IMAGE
          color = isDir ? Color(215,185,150) : Color(180,180,180)
          image
        }
        {
          rendObj = ROBJ_TEXT text = filename
          key = filename
          color = color(sf, params)
          tint = (sf & S_HOVER) ? Color(185,255,255, 150) : Color(0,0,0,0)
        }
      ]
    }
  }
  return component
}

return button
