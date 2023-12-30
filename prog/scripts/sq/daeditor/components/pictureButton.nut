from "%darg/ui_imports.nut" import *
let {colors} = require("style.nut")
let cursors = require("cursors.nut")
let {endswith} = require("string")

let defSize = [hdpx(21), hdpx(21)]

function imagePic(params){
  local image = params?.image
  let size = params?.size ?? defSize

  if (endswith(image, ".svg"))
    image = $"{image}:{size[0]}:{size[1]}"
  image = Picture(image)
  return{
    rendObj = ROBJ_IMAGE
    image
    size
    margin = params?.imageMargin
  }
}

function canvasPic(params){
  return{
    rendObj = ROBJ_VECTOR_CANVAS
    image = params?.image
    size = params?.size ?? defSize
    margin = params?.imageMargin
  }.__update(params?.canvasObj ?? {})
}

function picCmp(params){
  if (params?.canvasObj)
    return canvasPic(params)
  return imagePic(params)
}

function pictureButton(params) {
  let stateFlags = Watched(0)

  return function() {
    let color = (params?.checked || (stateFlags.value & S_ACTIVE))
                  ? colors.Active
                  : (stateFlags.value & S_HOVER)
                      ? colors.Hover
                      : params?.styles?.defColor ?? Color(130,130,130,250)

    return {
      rendObj = ROBJ_SOLID
      size = SIZE_TO_CONTENT
      margin = hdpx(2)
      behavior = Behaviors.Button
      color = color
      watch = stateFlags
      onHover = params?.onHover
      cursor = cursors.normal

      children = picCmp(params)
      onClick = params?.action
      onElemState = @(sf) stateFlags.update(sf)
    }
  }
}


return pictureButton
