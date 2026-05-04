from "%sqstd/frp.nut" import *
from "daRg" import *

function dtext(val, params={}, addchildren = null) {
  if (val == null)
    return null
  if (type(val)=="table") {
    params = val.__merge(params)
    val = params?.text
  }
  local children = params?.children
  if (children && type(children) !="array")
    children = [children]
  if (addchildren && children) {
    if (type(addchildren) == "array")
      children.extend(addchildren)
    else
      children.append(addchildren)
  }

  let watch = params?.watch
  local watchedtext = false
  local txt = ""
  if (type(val) == "string")  {
    txt = val
  }
  local obsVal = null
  if (type(val) == "instance" && isObservable(val)) {
    txt = val.get()
    obsVal = val
    watchedtext = true
  }
  let ret = {
    rendObj = ROBJ_TEXT
    size = SIZE_TO_CONTENT
    halign = ALIGN_LEFT
  }.__update(params, {text = txt})
  ret.__update({children=children})
  if (watchedtext) {
    let baseWatch = watch ? (type(watch) == "array" ? watch : [watch]) : []
    return function() {
      return ret.__merge({text = obsVal.get(), watch = [].extend(baseWatch, [obsVal])})
    }
  }
  if (watch)
    return @() ret
  else
    return ret
}

return {
  dtext
}
