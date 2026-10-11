from "%sqstd/frp.nut" import *
from "daRg" import *
from "types" import Table, Array, String

function dtext(val, params={}) {
  if (val == null)
    return null
  if (val instanceof Table) {
    params = val.__merge(params)
    val = params?.text
  }

  let watch = params?.watch
  local watchedtext = false
  local txt = ""
  if (val instanceof String)  {
    txt = val
  }
  local obsVal = null
  if (isObservable(val)) {
    txt = val.get()
    obsVal = val
    watchedtext = true
  }
  let ret = {
    rendObj = ROBJ_TEXT
    size = SIZE_TO_CONTENT
    halign = ALIGN_LEFT
  }.__update(params, {text = txt})
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
