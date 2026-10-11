// the shape of a daRg UI: builders that return component tables
from "types" import Function, Array, Table, Class
from "profile.nut" import benchmark

const ROBJ_TEXT = 0
const ROBJ_FRAME = 1
const ROBJ_SOLID = 2
const ALIGN_CENTER = 0
const FLOW_VERTICAL = 0
enum Behaviors {
  Button = 0
}

const SCREEN_WIDTH = 1920.0
const SCREEN_HEIGHT = 1080.0
const COMPONENTS = 2000
const FRAMES = 100
const BORDERS = true
const DEF_FLEX = {}

class Watched {
  value = null
  constructor(value = null) {
    this.value = value
  }
  function get() {
    return this.value
  }
  function modify(f) {
    this.value = f(this.value)
  }
}

// Park-Miller integers, the same layout in every port
local seed = 42
function rint(max) {
  seed = seed * 16807 % 2147483647
  return seed % max
}

const function [pure] sw(val) { //-ident-hides-ident
  return (val / 100.0 * SCREEN_WIDTH).tointeger()
}

const function [pure] sh(val) { //-ident-hides-ident
  return (val / 100.0 * SCREEN_HEIGHT).tointeger()
}

const function [pure] Color(r, g, b, a = 255) { //-ident-hides-ident
  return (a << 24) + (r << 16) + (g << 8) + b
}

const function [pure] hdpx(val) { //-ident-hides-ident
  return val * SCREEN_HEIGHT / sh(100)
}

const function [pure] flex(val = null) { //-ident-hides-ident
  return val == null ? DEF_FLEX : { val }
}

local withText = 0

function simpleComponent(i, watch) {
  let pos = [sw(rint(80)), sh(rint(80))]
  let size = [sh(rint(15) + 2), sh(rint(15) + 2)]
  let color = Color(rint(256), rint(256), rint(256))
  let textCanBePlaced = size[0] > hdpx(150)
  let addText = rint(5) == 1
  let children = []
  if (BORDERS) {
    children.append(@() {
      borderWidth = const [1, 1, 1, 1]
      rendObj = ROBJ_FRAME
      size = flex()
    })
    if (textCanBePlaced) {
      withText++
      children.append(@() {
        watch
        rendObj = ROBJ_TEXT
        text = addText || !watch.get() ? "".concat(i, watch.get()) : null
      })
    }
  }
  return @() {
    rendObj = ROBJ_SOLID
    size
    key = {}
    pos
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    padding = const [hdpx(4), hdpx(5)]
    margin = const [hdpx(5), hdpx(3)]
    color = watch.get() ? color : Color(0, 0, 0)
    behavior = Behaviors.Button
    onClick = @() watch.modify(@(v) !v)
    watch
    children
  }
}

let components = []
for (local i = 0; i < COMPONENTS; i++)
  components.append(simpleComponent(i, Watched(i % 3 == 0)))

let root = @() {
  size = flex()
  flow = FLOW_VERTICAL
  children = components
}

function testUi(entry) {
  if (entry == null)
    return true
  if (entry instanceof Function)
    entry = entry()
  if (entry instanceof Table || entry instanceof Class) {
    if ("children" not in entry)
      return true
    let children = entry.children
    if (children instanceof Array) {
      foreach (child in children)
        if (!testUi(child))
          return false
      return true
    }
    return testUi(children)
  }
  return false
}

function frames() {
  for (local f = 0; f < FRAMES; f++)
    testUi(root)
  return withText
}

benchmark("profile darg ui", 20, frames)
