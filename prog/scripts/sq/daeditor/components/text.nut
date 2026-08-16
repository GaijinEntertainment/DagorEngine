from "%sqstd/frp.nut" import *
from "daRg" import *
from "types" import Table, Array, String

function dtext(val, params={}, addchildren = null) {
  if (val == null)
    return null
  if (val instanceof Table) {
    params = val.__merge(params)
    val = params?.text
  }
  local children = params?.children
  if (children && !(children instanceof Array))
    children = [children]
  if (addchildren) {
    children = children ?? []
    if (addchildren instanceof Array)
      children.extend(addchildren)
    else
      children.append(addchildren)
  }

  let watch = params?.watch
  local watchedtext = false
  local txt = ""
  if (val instanceof String)  {
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
    let baseWatch = watch ? (watch instanceof Array ? watch : [watch]) : []
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
