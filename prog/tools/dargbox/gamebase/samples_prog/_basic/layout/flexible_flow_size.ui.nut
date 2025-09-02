from "%darg/ui_imports.nut" import *

let txt = @(text) {
  rendObj = ROBJ_TEXT
  text
}

let txtFlex = @(text) {
  size = FLEX_H
  halign = ALIGN_CENTER
  rendObj = ROBJ_TEXT
  text
}

let txtMin = @(text) {
  size = FLEX_H
  minWidth = SIZE_TO_CONTENT
  halign = ALIGN_CENTER
  rendObj = ROBJ_TEXT
  text
}

let fixedGap = {
  size = static [sh(2), flex()]
  halign = ALIGN_CENTER
  children = { rendObj = ROBJ_SOLID size = static [1, flex()] }
}

let flexGap = {
  size = flex()
  halign = ALIGN_CENTER
  children = { rendObj = ROBJ_SOLID size = static [1, flex()] }
}

let halfGap = {
  size = flex(0.5)
}

return {
  rendObj = ROBJ_FRAME
  size = static [sw(50), SIZE_TO_CONTENT]
  hplace = ALIGN_CENTER
  vplace = ALIGN_CENTER
  flow = FLOW_VERTICAL
  gap = sh(1)
  padding = sh(1)
  borderWidth = 1
  children = [
    {
      rendObj = ROBJ_FRAME
      size = FLEX_H
      flow = FLOW_HORIZONTAL
      gap = fixedGap
      borderWidth = 1
      color = 0xFFFF3399
      children = [
        txt("one")
        txt("two")
        txt("long three")
        txt("the longest four")
        txt("five")
        txt("6")
      ]
    }

    {
      rendObj = ROBJ_FRAME
      size = FLEX_H
      flow = FLOW_HORIZONTAL
      gap = fixedGap
      borderWidth = 1
      color = 0xFFAADD66
      children = [
        txtFlex("one")
        txtFlex("two")
        txtFlex("long three")
        txtFlex("the longest four")
        txtFlex("five")
        txtFlex("6")
      ]
    }

    {
      rendObj = ROBJ_FRAME
      size = FLEX_H
      flow = FLOW_HORIZONTAL
      gap = fixedGap
      borderWidth = 1
      color = 0xFF3366DD
      children = [
        txtMin("one")
        txtMin("two")
        txtMin("long three")
        txtMin("the longest four")
        txtMin("five")
        txtMin("6")
      ]
    }

    {
      rendObj = ROBJ_FRAME
      size = FLEX_H
      flow = FLOW_HORIZONTAL
      gap = flexGap
      borderWidth = 1
      color = 0xFF9966EE
      children = [
        txt("one")
        txt("two")
        txt("long three")
        txt("the longest four")
        txt("five")
        txt("6")
      ]
    }

    {
      rendObj = ROBJ_FRAME
      size = FLEX_H
      flow = FLOW_HORIZONTAL
      borderWidth = 1
      color = 0xFFFFDD66
      children = [
        halfGap
        txt("one")
        flexGap
        txt("two")
        flexGap
        txt("long three")
        flexGap
        txt("the longest four")
        flexGap
        txt("five")
        flexGap
        txt("6")
        halfGap
      ]
    }
  ]
}