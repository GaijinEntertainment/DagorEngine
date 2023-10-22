from "%sqstd/frp.nut" import *
from "daRg" import *

let function dtext(val, params={}, addchildren = null) {
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
  if (type(val) == "instance" && val instanceof Watched) {
    txt = val.value
    watchedtext = true
  }
  let ret = {
    rendObj = ROBJ_TEXT
    size = SIZE_TO_CONTENT
    halign = ALIGN_LEFT
  }.__update(params, {text = txt})
  ret.__update({children=children})
  if (watch || watchedtext)
    return @() ret
  else
    return ret
}

return {
  dtext
}
