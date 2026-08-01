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
  mkKeyedList: dynamic list that builds each item's component (and the observables it owns)
  once, keyed by a stable id, instead of reconstructing every item on each rebuild.

  The cache is synced off the build path (in a subscription wired at onAttach), so the container
  builder is a pure id -> cached-component lookup; it constructs nothing per rebuild. Removing an
  item from the source evicts and GCs its component. Because cached identity is stable per id,
  daRg reuses the item elements across reorders.

  params:
    source         - Watched/Computed holding the array of item models
    mkItem         - @(item) -> component; called once per new id, off the build path. May freely
                     construct observables/subscriptions - that is where per-item state belongs.
    keyOf          - @(item) -> stable unique id. Required: the cache is only correct if the key
                     is a stable logical id that survives a data refresh (new item table, same
                     entity), so it is a conscious choice, not a default.
                     If not specified, looks up "id" field.
    containerProps - extra props merged into the wrapper element (flow, gap, size, rendObj, ...).
                     watch/children/onAttach/onDetach are owned by the helper and rejected here;
                     put your own lifecycle/watch on an element wrapping the list.

  Call at a once scope (module scope or a factory body), never inside a builder - otherwise the
  cache and subscription are rebuilt every rebuild, the very cost this avoids.
*/
function mkKeyedList(params) {
  let {source, mkItem, keyOf = @(v) v.id} = params
  let containerProps = params?.containerProps ?? const {}
  foreach (k in ["watch", "onAttach", "onDetach", "children"])
    assert(k not in containerProps, @() $"mkKeyedList: '{k}' is owned by the helper; put it on a wrapping element")

  let cache = {} // id -> component, built once per id

  function ensure(item) {
    let id = keyOf(item)
    assert(id != null, "mkKeyedList: key returned null; keys must be stable non-null ids")
    if (id not in cache)
      cache[id] <- mkItem(item)
    return id
  }

  function sync(items) { // prewarm cache + evict removed, off the build path
    let seen = {}
    foreach (item in items ?? const [])
      seen[ensure(item)] <- true
    foreach (id in cache.keys())
      if (id not in seen)
        cache.rawdelete(id)
  }

  sync(source.get()) // initial population before first build

  let onAttach = function() {
    source.subscribe(sync)
    sync(source.get()) // reconcile removals that might happen
  }

  let onDetach = @() source.unsubscribe(sync)

  return @() containerProps.__merge({
    watch = source
    onAttach
    onDetach
    children = (source.get() ?? const []).map(@(item) cache[ensure(item)])
  })
}

/*
  mkAsyncImage: image that shows a placeholder component while its picture is still loading.
  Complements rendObj fallbackImage, which covers load failure and can only be a picture.

  picture     - Picture instance (or null)
  placeholder - component shown while the load is in flight; it is mounted and unmounted, so an
                autoplay fade-in and a playFadeOut fade-out are all it needs
  imageProps  - props for the image element (size, keepAspect, fallbackImage, ...). 'watch' and
                'behavior' are merged with the helper's own, 'children' is not allowed.

  Call at a once scope, never inside a builder: the loading flag is per-picture state.
*/
function mkAsyncImage(picture, placeholder, imageProps = {}) {
  assert("children" not in imageProps,
    "mkAsyncImage: 'children' holds the placeholder; wrap the image if you need more")

  // seeded from prefetch(): a picture that resolves synchronously must not flash the placeholder
  let imageLoading = Watched(picture != null && picture.prefetch())

  local watch = imageProps?.watch ?? []
  if (type(watch) != "array")
    watch = [watch]
  local behavior = imageProps?.behavior ?? []
  if (type(behavior) != "array")
    behavior = [behavior]

  return @() imageProps.__merge({
    rendObj = ROBJ_IMAGE
    image = picture
    imageLoading
    behavior = [Behaviors.ImageLoadState].extend(behavior)
    watch = [imageLoading].extend(watch)
    children = imageLoading.get() ? placeholder : null
  })
}

/*
//===== DARG specific methods=====
*/
function isDargComponent(comp): bool {
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
    if (k in const ["size","rendObj","children","watch","behavior","halign","valign","flow","pos","hplace","vplace"].totable())
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

let wrapParams= const {width=0, flowElemProto={}, hGap=null, vGap=0, height=null, flow=FLOW_HORIZONTAL}
function wrap(elems, params=wrapParams) {
  //TODO: move this to native code
  let paddingLeft=params?.paddingLeft
  let paddingRight=params?.paddingRight
  let paddingTop=params?.paddingTop
  let paddingBottom=params?.paddingBottom
  let flow = params?.flow ?? FLOW_HORIZONTAL
  assert([FLOW_HORIZONTAL, FLOW_VERTICAL].contains(flow), "flow should be FLOW_VERTICAL or FLOW_HORIZONTAL")
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
  let flowElemProto = params?.flowElemProto ?? const {}
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
      if (i==0) {
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
  if (paddingBottom && isFlowHor)
    flowElems.append(paddingBottom)
  if (paddingRight && !isFlowHor)
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
function [pure] mul_color(color: int, mult: number, alpha_mult: number=1) {
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

let FLEX_H = const [flex(), SIZE_TO_CONTENT]
let flex_h = function [pure] (val=null) {
  if (val == null)
    return FLEX_H
  assert(typeof val in numerics, @() $"val can be only numerics, got {type(val)}")
  return [flex(val), SIZE_TO_CONTENT]
}

let FLEX_V = const [SIZE_TO_CONTENT, flex()]
let FLEX = const flex()
let flex_v = function [pure] (val=null) {
  if (val == null)
    return FLEX_V
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
  mkKeyedList
  mkAsyncImage
  isDargComponent
  fsh
  Behaviors
  getWatcheds
  Set
  FLEX_H
  FLEX_V
  FLEX
  flex_h
  flex_v
}))
