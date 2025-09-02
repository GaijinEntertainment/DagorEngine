from "%sqstd/frp.nut" import *
import "daRg" as darg
import "daRg.behaviors" as Behaviors
from "%sqstd/functools.nut" import Set
from "%sqstd/string.nut" import tostring_r
from "math" import min
from "daRg" import sh, sw, calc_comp_size, gui_scene, Color, flex
/*
//===== DARG specific methods=====
  this function create element that has internal basic stateFlags (S_HOVER S_ACTIVE S_DRAG)
*/
function watchElemState(builder, params={}) {
  let stateFlags = params?.stateFlags ?? Watched(0)
  let onElemState = @(sf) stateFlags.set(sf)
  return function() {
    let desc = builder(stateFlags.get())
    local watch = desc?.watch ?? []
    if (type(watch) != "array")
      watch = [watch]
    desc.watch <- [stateFlags].extend(watch)
    desc.onElemState <- onElemState
    return desc
  }
}

/*
//===== DARG specific methods=====
*/
function isDargComponent(comp) {
//better to have natived daRg function to check if it is valid component!
  local c = comp
  if (type(c) == "function") {
    let info = c.getfuncinfos()
    if (info?.parameters && info.parameters.len() > 1)
      return false
    c = c()
  }
  let c_type = type(c)
  if (c_type == "null")
    return true
  if (c_type != "table" && c_type != "class")
    return false
  foreach(k, _val in c) {
    if (k in static ["size","rendObj","children","watch","behavior","halign","valign","flow","pos","hplace","vplace"].totable())
      return true
  }
  return false
}

//this function returns sh() for pixels for fullhd resolution (1080p)
//but result is not bigger than 0.75sw (for resolutions narrower than 4x3)
let hdpx = sh(100) <= sw(75)
  ? @[pure](pixels) sh(100.0 * pixels / 1080)
  : @[pure](pixels) sw(75.0 * pixels / 1080)

let hdpxi = @[pure](pixels) hdpx(pixels).tointeger()

let fsh = sh(100) <= sw(75) ? sh : @[pure](v) sw(0.75 * v)

let numerics = Set("float", "integer")

let wrapParams= static {width=0, flowElemProto={}, hGap=null, vGap=0, height=null, flow=FLOW_HORIZONTAL}
function wrap(elems, params=wrapParams) {
  //TODO: move this to native code
  let paddingLeft=params?.paddingLeft
  let paddingRight=params?.paddingRight
  let paddingTop=params?.paddingTop
  let paddingBottom=params?.paddingBottom
  let flow = params?.flow ?? FLOW_HORIZONTAL
  assert([FLOW_HORIZONTAL, FLOW_VERTICAL].indexof(flow)!=null, "flow should be FLOW_VERTICAL or FLOW_HORIZONTAL")
  let isFlowHor = flow==FLOW_HORIZONTAL
  let height = params?.height ?? SIZE_TO_CONTENT
  let width = params?.width ?? SIZE_TO_CONTENT
  let dimensionLim = isFlowHor ? width : height
  assert(type(elems)=="array", "elems should be array")
  assert(type(dimensionLim) in {float=1,integer=1}, @() "can't flow over {0} non numeric type".subst(isFlowHor ? "width" :"height"))
  let hgap = params?.hGap ?? wrapParams?.hGap
  let vgap = params?.vGap ?? wrapParams?.vGap
  local gap = isFlowHor ? hgap : vgap
  let secondaryGap = isFlowHor ? vgap : hgap
  if (type(gap) in numerics)
    gap = isFlowHor ? freeze({size=[gap,0]}) : freeze({size=[0,gap]})
  let flowElemProto = params?.flowElemProto ?? static {}
  let flowElems = []
  if (paddingTop && isFlowHor)
    flowElems.append(paddingTop)
  if (paddingLeft && !isFlowHor)
    flowElems.append(paddingLeft)
  local tail = elems
  function buildFlowElem(elems, gap, flowElemProto, dimensionLim) {  //warning disable: -ident-hides-ident -param-hides-param
    let children = []
    local curwidth=0.0
    local tailidx = 0
    let flowSizeIdx = isFlowHor ? 0 : 1
    foreach (i, elem in elems) {
      let esize = calc_comp_size(elem)[flowSizeIdx]
      let gapsize = isDargComponent(gap) ? calc_comp_size(gap)[flowSizeIdx] : gap
      if (i==0 && ((curwidth + esize) <= dimensionLim)) {
        children.append(elem)
        curwidth = curwidth + esize
        tailidx = i
      }
      else if ((curwidth + esize + gapsize) <= dimensionLim) {
        children.append(gap, elem)
        curwidth = curwidth + esize + gapsize
        tailidx = i
      }
      else {
        tail = elems.slice(tailidx+1)
        break
      }
      if (i==elems.len()-1){
        tail = []
        break
      }
    }
    flowElems.append(flowElemProto.__merge({children flow=isFlowHor ? FLOW_HORIZONTAL : FLOW_VERTICAL}))
  }

  do {
    buildFlowElem(tail, gap, flowElemProto, dimensionLim)
  } while (tail.len()>0)
  if (paddingTop && isFlowHor)
    flowElems.append(paddingBottom)
  if (paddingLeft && !isFlowHor)
    flowElems.append(paddingRight)
  return {flow=isFlowHor ? FLOW_VERTICAL : FLOW_HORIZONTAL gap=secondaryGap children=flowElems halign = params?.halign valign=params?.valign hplace=params?.hplace vplace=params?.vplace size=[width ?? SIZE_TO_CONTENT, height ?? SIZE_TO_CONTENT]}
}


function dump_observables() {
  let list = gui_scene.getAllObservables()
  print("{0} observables:".subst(list.len()))
  foreach (obs in list)
    print(tostring_r(obs))
}

let colorPart = @(value) min(255, (value + 0.5).tointeger())
function [pure] mul_color(color, mult, alpha_mult=1) {
  return Color(  colorPart(((color >> 16) & 0xff) * mult),
                 colorPart(((color >>  8) & 0xff) * mult),
                 colorPart((color & 0xff) * mult),
                 colorPart(((color >> 24) & 0xff) * mult * alpha_mult))
}

function [pure] XmbNode(params={}) {
  return clone params
}

function [pure] XmbContainer(params={}) {
  return XmbNode({
    canFocus = false
  }.__merge(params))
}

function mkWatched(persistFunc, persistKey, defVal=null, observableInitArg=null){
  let container = persistFunc(persistKey, @() {v=defVal})
  let watch = observableInitArg==null ? Watched(container.v) : Watched(container.v, observableInitArg)
  watch.subscribe(@(v) container.v=v)
  return watch
}

let FLEX_H = static [flex(), SIZE_TO_CONTENT]
let flex_h = function [pure] (val=null) {
  if (val == null)
    return FLEX_H
  assert(typeof val in numerics, @() $"val can be only numerics, got {type(val)}")
  return [flex(val), SIZE_TO_CONTENT]
}

let FLEX_V = static [SIZE_TO_CONTENT, flex()]
let flex_v = function [pure] (val=null) {
  if (val == null)
    return FLEX_H
  assert(typeof val in numerics, @() $"val can be only numerics, got {type(val)}")
  return [SIZE_TO_CONTENT, flex(val)]
}

return freeze(darg.__merge({
  mkWatched
  WatchedRo
  XmbNode
  XmbContainer
  mul_color
  wrap
  dump_observables
  hdpx
  hdpxi
  watchElemState
  isDargComponent
  fsh
  Behaviors
  getWatcheds
  Set
  FLEX_H
  FLEX_V
  flex_h
  flex_v
}))
