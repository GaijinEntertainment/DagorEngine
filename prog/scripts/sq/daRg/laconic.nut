from "daRg" import *
from "%sqstd/underscore.nut" import partition, flatten


/*
laconic framework
make layout less nested and allow short way to setup properties (like flow, flex, and so on)

compare
let foo = {
  flow = FLOW_VERTICAL
  children = [
    {rendObj = ...}
    {...}
    {
      flow = FLOW_HORIZONTAL
      children = [child1, child2]
    }
  ]
}
and
let foo = comp(
  FlowV,
  {rendObj = ...},
  {...}
  comp(FlowV, child1, child2)
)
or
let foo = comp(Style({flow = FLOW_HORIZONTAL}, child1, child2))

todo:
  correct combine watch and behaviors (do not override)
*/

let Style = class { //to be able distinguish style elements from components
  value = null
  constructor(...) {
    //I do not know the way to return instance of the same class if it is passed here
    let val = {}
    foreach(v in vargv){
      if (type(v) == "instance" && "value" in v) { //to create Style from Styles
        val.__update(v.value)
      }
      else {
        assert(type(v) == "table")
        val.__update(v)
      }
    }
    this.value = freeze(val)
  }
}

let extendable = ["watch", "behavior"]
let toArray = @(v) type(v) == "array" ? v : [v]

let function comp(...) {
  local [styles, children] = partition(flatten(vargv), @(v) v instanceof Style)
  local ret = styles.reduce(function(a,b) {
    foreach (k, v in b.value){
      local found = false
      foreach (e in extendable) {
        if (k != e)
          continue
        found = true
        if (k not in a) {
          a[k] <- toArray(v)
          break
        }
        else {
          a[k] = toArray(a[k]).extend(toArray(v))
          break
        }
      }
      if (!found) {
        assert(k not in a, @() $"property {k} already exist")
        a[k] <- v
      }
    }
    a.__update(b.value)
    return a
  }, {})

  children = flatten(children)
  if (ret?.children)
    children = flatten([ret?.children]).extend(children)
  if (children.len()>1){
    ret = ret.__update({children})
  }
  else if (children.len()==1) {
    children = children[0]
    ret = ret.__update({children})
  }
  if (ret.len()==1 && "children" in ret)
    ret = type(ret.children) != "array" || ret.children.len()==1 ? ret.children?[0] : ret
  return ("watch" in ret) ? @() ret : ret
}

let FlowV = Style({flow = FLOW_VERTICAL})
let FlowH = Style({flow = FLOW_HORIZONTAL})
let Flex = @(p=null) Style({size = p!=null ? flex(p) : flex()})
let vflow = @(...) comp(FlowV, vargv)
let hflow = @(...) comp(FlowH, vargv)
let Text = Style({rendObj = ROBJ_TEXT})
let Image = Style({rendObj = ROBJ_IMAGE})
let TextArea = Style({rendObj = ROBJ_TEXTAREA behavior=Behaviors.TextArea})
let Gap = @(gap) Style({gap})
let FillColr = @(...) Style({fillColor = Color.acall([null].extend(vargv))})
let BorderColr = @(...) Style({borderColor = Color.acall([null].extend(vargv))})
let BorderWidth = @(...) Style({borderWidth = vargv})
let BorderRadius = @(...) Style({borderRadius = vargv})
let ClipChildren = Style({clipChildren = true})
let Bhv = @(...) Style({behaviors = flatten(vargv)})
let Watch = @(...) Style({watch = flatten(vargv)})
let OnClick = @(func) Style({onClick = func})
let Button = Style({behavior = Behaviors.Button})

let function Size(...) {
  assert(vargv.len()<3)
  local size = vargv
  if (size.len()==1 && type(size?[0]) != "array")
    size = [size[0], size[0]]
  else if (size.len()==0)
    size = null
  return Style({size})
}

let Padding = @(...) Style({padding = vargv.len() > 1 ? vargv : vargv[0]})
let Margin = @(...) Style({margin = vargv.len() > 1 ? vargv : vargv[0]})
let Pos = @(...) Style({pos=vargv})
let YOfs = @(y) Style({pos=[0,y]})
let XOfs = @(x) Style({pos=[0,x]})

let function updateWithStyle(obj, style){
  if (type(style) == "table") {
    foreach (k in style)
      assert(k not in obj)
    obj.__update(style)
  }
  else if (style instanceof Style) {
    foreach (k in style.value)
      assert(k not in obj)
    obj.__update(style.value)
  }
  if ("watch" in obj)
    return @() obj
  else
    return obj
}

let function txt(text, style = null) {
  let obj = (type(text) == "table")
    ? text.__merge({rendObj = ROBJ_TEXT})
    : {rendObj = ROBJ_TEXT text}
  return updateWithStyle(obj, style)
}

let function img(image, style = null) {
  let obj = (type(image) == "table") ? image.__merge({rendObj = ROBJ_IMAGE}) : {rendObj = ROBJ_IMAGE image}
  return updateWithStyle(obj, style)
}

let RendObj = @(rendObj) Style({rendObj})
let Colr = @(...) Style({color=Color.acall([null].extend(vargv))})

let HALeft = Style({halign = ALIGN_LEFT})
let HARight = Style({halign = ALIGN_RIGHT})
let HACenter = Style({halign = ALIGN_CENTER})
let VATop = Style({valign = ALIGN_TOP})
let VABottom = Style({valign = ALIGN_BOTTOM})
let VACenter = Style({valign = ALIGN_CENTER})

let Left = Style({hplace = ALIGN_LEFT})
let Right = Style({hplace = ALIGN_RIGHT})
let HCenter = Style({hplace = ALIGN_CENTER})
let Top = Style({vplace = ALIGN_TOP})
let Bottom = Style({vplace = ALIGN_BOTTOM})
let VCenter = Style({vplace = ALIGN_CENTER})

return {
  Style
  comp
  vflow
  hflow
  txt
  img

  FlowV, FlowH, Flex, Text, Image, TextArea,
  Size, Padding, Margin, Gap, Colr, RendObj, Pos, XOfs, YOfs,
  VCenter, Top, Bottom, VABottom, VATop, VACenter,
  HCenter, Left, Right, HACenter, HALeft, HARight,
  BorderColr, BorderWidth, BorderRadius, FillColr,
  Bhv, ClipChildren, Watch, Button, OnClick
}