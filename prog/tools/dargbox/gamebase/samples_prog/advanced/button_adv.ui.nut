from "%darg/ui_imports.nut" import *
let cursors = require("samples_prog/_cursors.nut")

//[WIP] !very experimental component
// the idea here is to recursively through components and updated them with provided table of components with the same tree structure


let function __update_r(target, source, deepLevel = -1) {
  let function sub_update_r(target, source, deepLevel = -1) {
    let res = {}.__update(target)
    foreach (key,value in source) {
      if (type(value) =="table" && deepLevel != 0 && key in target) {
        res[key] = sub_update_r({}.__update(target[key]), {}.__update(value), deepLevel - 1)
      } else {
        res[key] <- source[key]
      }
    }
    return res
  }
  return sub_update_r(target, source, deepLevel)
}

let function copy_component(comp) {
  if (type(comp) == "table")
    return {}.__update(comp)
  if (type(comp) == "null")
    return {}
  return comp
}

let function __update_darg_r(target, source, deepLevel = -1) {

  let function isUpdatable(component) {
    if (type(component) == "class" || type(component) == "table")
      return true
    return false
  }

  let function sub_update_r(target, source, deepLevel = -1) {
    let res = {}.__update(target)
    foreach (key, value in source) {
      if (key == "children" && key in target && type(value) == "array" && type(target[key]) == "array") {
        let newValue = []
        foreach (idx, val in target[key]) {
          if (idx < value.len()) {
            if (isUpdatable(val) && isUpdatable(value[idx])) {
              newValue.append(sub_update_r(copy_component(val), copy_component(value[idx])))
            }
            else {
              newValue.append(copy_component(value[idx]))
            }
          }
          else
            newValue.append(val)
        }
        if (value.len() > target[key].len()) {
          newValue.extend(value.slice(target[key].len()))
        }
        res[key] = newValue
      }
      else if (type(value) =="table" && deepLevel != 0 && key in target) {
        res[key] = sub_update_r(copy_component(target[key]), copy_component(value), deepLevel - 1)
      }
      else {
        res[key] <- source[key]
      }
    }
    return res
  }
  return sub_update_r(target, source, deepLevel)
}

let button = watchElemState(
  function(sf) {
    let normalBtn = {
      rendObj = ROBJ_BOX
      size = [sh(20),SIZE_TO_CONTENT]
      padding = sh(2)
      fillColor = Color(0,128,0)
      borderWidth = 0
      behavior = [Behaviors.Button]
      halign = ALIGN_CENTER
      onClick = function() {
        vlog("clicked")
      }
      onHover = function(hover) {
        vlog($"hover: {hover}")
      }
      //borderColor = Color(255,255,255)
      children = [{rendObj = ROBJ_TEXT text = "my button" } ]
    }
    let hoverBtn = {
      fillColor = Color(128,128,0)
      borderWidth = 2
      borderColor = Color(255,255,255)
      children = [{text = "hovered" color=Color(255,255,0)} {size=flex() rendObj = ROBJ_FRAME}]
    }

    let activeBtn = {
      fillColor = Color(255,255,255)
      children = [{text = "pressed" color=Color(0,0,0) pos =[0,3]} null]
    }

    local btn = normalBtn
    if (sf & S_HOVER) {
      btn = __update_darg_r(btn, hoverBtn)
    }

    if (sf & S_ACTIVE) {
      btn = __update_darg_r(btn, activeBtn)
    }
    return btn
  }
)


return {
  rendObj = ROBJ_SOLID
  color = Color(30,40,50)
  size = flex()
  gap = sh(5)
  cursor = cursors.normal
  flow = FLOW_HORIZONTAL
  halign = ALIGN_CENTER
  valign = ALIGN_CENTER
  children = [
    button
  ]
}
