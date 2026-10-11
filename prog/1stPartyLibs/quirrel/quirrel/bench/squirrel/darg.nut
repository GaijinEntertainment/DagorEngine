dofile("profile.nut")

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

local DEF_FLEX = {}

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

// Park-Miller, same sequence in every port
local seed = 42
local function rint(max) {
  seed = seed * 16807 % 2147483647
  return seed % max
}

local function sw(val) {
  return (val / 100.0 * SCREEN_WIDTH).tointeger()
}

local function sh(val) {
  return (val / 100.0 * SCREEN_HEIGHT).tointeger()
}

local function Color(r, g, b, a = 255) {
  return (a << 24) + (r << 16) + (g << 8) + b
}

local function hdpx(val) {
  return val * SCREEN_HEIGHT / sh(100)
}

local function flex(val = null) {
  return val == null ? DEF_FLEX : { val = val }
}

local BORDER_WIDTH = [1, 1, 1, 1]
local PADDING = [hdpx(4), hdpx(5)]
local MARGIN = [hdpx(5), hdpx(3)]
local BLACK = Color(0, 0, 0)

local withText = 0

local function simpleComponent(i, watch) {
  local pos = [sw(rint(80)), sh(rint(80))]
  local size = [sh(rint(15) + 2), sh(rint(15) + 2)]
  local color = Color(rint(256), rint(256), rint(256))
  local textCanBePlaced = size[0] > hdpx(150)
  local addText = rint(5) == 1
  local children = []
  if (BORDERS) {
    children.append(@() {
      borderWidth = BORDER_WIDTH
      rendObj = ROBJ_FRAME
      size = flex()
    })
    if (textCanBePlaced) {
      withText++
      children.append(@() {
        watch = watch
        rendObj = ROBJ_TEXT
        text = addText || !watch.get() ? "" + i + watch.get() : null
      })
    }
  }
  return @() {
    rendObj = ROBJ_SOLID
    size = size
    key = {}
    pos = pos
    valign = ALIGN_CENTER
    halign = ALIGN_CENTER
    padding = PADDING
    margin = MARGIN
    color = watch.get() ? color : BLACK
    behavior = Behaviors.Button
    onClick = @() watch.modify(@(v) !v)
    watch = watch
    children = children
  }
}

local components = []
for (local i = 0; i < COMPONENTS; i++)
  components.append(simpleComponent(i, Watched(i % 3 == 0)))

local root = @() {
  size = flex()
  flow = FLOW_VERTICAL
  children = components
}

// a local function cannot see itself
local testUi
testUi = function(entry) {
  if (entry == null)
    return true
  if (typeof entry == "function")
    entry = entry()
  local entryType = typeof entry
  if (entryType == "table" || entryType == "class") {
    if (!("children" in entry))
      return true
    local children = entry.children
    if (typeof children == "array") {
      foreach (child in children)
        if (!testUi(child))
          return false
      return true
    }
    return testUi(children)
  }
  return false
}

local function frames() {
  for (local f = 0; f < FRAMES; f++)
    testUi(root)
  return withText
}

benchmark("profile darg ui", 20, frames)
