from "%darg/ui_imports.nut" import *

let curAnimations = [] //id, comp
let generation = Watched(0)
let incGen = @() generation(generation.value + 1)

let compToCompAnimations = @() {
  watch = generation
  size = flex()
  children = curAnimations.map(@(a) a.comp)
}

let function removeCompAnim(id) {
  let idx = curAnimations.findindex(@(a) a.id == id)
  if (idx == null)
    return
  curAnimations.remove(idx)
  incGen()
}

let isAABB = @(o) type(o) == "table" && "l" in o && "r" in o && "b" in o && "t" in o

let getAABB = @(aabbOrKey) isAABB(aabbOrKey) ? aabbOrKey : gui_scene.getCompAABBbyKey(aabbOrKey)
let mkSize = @(aabb) [aabb.r - aabb.l, aabb.b - aabb.t]

//from, to - aabb or key of component to animate from position 'from' to position 'to'
//component - component to animate. will be child of animated object
let function addCompToCompAnim(from, to, component, easing = InOutQuad, duration = 1.0, fadeOutDuration = 0.1) {
  let fromBox = getAABB(from)
  let toBox = getAABB(to)
  if (fromBox == null || toBox == null) {
    logerr($"Not found aabb for objects. (from = {from}, to = {to})")
    return null
  }

  let id = generation.value
  let fromSize = mkSize(fromBox)
  let toSize = mkSize(toBox)
  let animations = []
  if (toSize[0] > 0 && toSize[1] > 0 && (toSize[0] != fromSize[0] || toSize[1] != fromSize[1]))
    animations.append({ prop = AnimProp.scale, duration, easing, play = true,
      from = fromSize.map(@(v, i) v.tofloat() / toSize[i]), to = [1, 1] })
  if (fromBox.l != toBox.l || fromBox.t != toBox.t)
    animations.append({ prop = AnimProp.translate, duration, easing, play = true,
      from = [fromBox.l - toBox.l, fromBox.t - toBox.t], to = [0, 0] })

  if (animations.len() == 0)
    return null

  animations.top().onFinish <- @() removeCompAnim(id)
  if (fadeOutDuration > 0)
    animations.append({ prop = AnimProp.opacity, easing = InOutQuad, playFadeOut = true,
      duration = fadeOutDuration,
      from = 1.0, to = 0.0
    })


  curAnimations.append({
    id
    comp = {
      key = id
      size = toSize
      pos = [toBox.l, toBox.t]
      transform = { pivot = [0, 0] }
      children = component
      animations
      onDetach = @() removeCompAnim(id)
    }
  })
  incGen()
  return id
}

return {
  addCompToCompAnim = kwarg(addCompToCompAnim)
  removeCompAnim
  compToCompAnimations
}