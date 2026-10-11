local benchmark = require("profile").benchmark
local new_tab = require("table.new")
local floor = math.floor
local setmetatable = setmetatable
local tostring = tostring
local type = type

local ROBJ_TEXT = 0
local ROBJ_FRAME = 1
local ROBJ_SOLID = 2
local ALIGN_CENTER = 0
local FLOW_VERTICAL = 0
local BEHAVIOR_BUTTON = 0

local SCREEN_WIDTH = 1920.0
local SCREEN_HEIGHT = 1080.0
local COMPONENTS = 2000
local FRAMES = 100
local BORDERS = true
local DEF_FLEX = {}

local Watched = {}
Watched.__index = Watched

function Watched.new(value)
  return setmetatable({ value = value }, Watched)
end

function Watched:get()
  return self.value
end

function Watched:modify(f)
  self.value = f(self.value)
end

local seed = 42
local function rint(max)
  seed = seed * 16807 % 2147483647
  return seed % max
end

local function sw(val)
  return floor(val / 100.0 * SCREEN_WIDTH)
end

local function sh(val)
  return floor(val / 100.0 * SCREEN_HEIGHT)
end

local function Color(r, g, b, a)
  a = a or 255
  return a * 16777216 + r * 65536 + g * 256 + b
end

local function hdpx(val)
  return val * SCREEN_HEIGHT / sh(100)
end

local function flex(val)
  if val == nil then
    return DEF_FLEX
  end
  return { val = val }
end

local FRAME_BORDER_WIDTH = { 1, 1, 1, 1 }
local SOLID_PADDING = { hdpx(4), hdpx(5) }
local SOLID_MARGIN = { hdpx(5), hdpx(3) }
local BLACK = Color(0, 0, 0)

local withText = 0

local function simpleComponent(i, watch)
  local pos = { sw(rint(80)), sh(rint(80)) }
  local size = { sh(rint(15) + 2), sh(rint(15) + 2) }
  local color = Color(rint(256), rint(256), rint(256))
  local textCanBePlaced = size[1] > hdpx(150)
  local addText = rint(5) == 1
  local children = {}
  if BORDERS then
    children[#children + 1] = function()
      return {
        borderWidth = FRAME_BORDER_WIDTH,
        rendObj = ROBJ_FRAME,
        size = flex(),
      }
    end
    if textCanBePlaced then
      withText = withText + 1
      children[#children + 1] = function()
        return {
          watch = watch,
          rendObj = ROBJ_TEXT,
          text = (addText or not watch:get()) and i .. tostring(watch:get()) or nil,
        }
      end
    end
  end
  return function()
    return {
      rendObj = ROBJ_SOLID,
      size = size,
      key = {},
      pos = pos,
      valign = ALIGN_CENTER,
      halign = ALIGN_CENTER,
      padding = SOLID_PADDING,
      margin = SOLID_MARGIN,
      color = watch:get() and color or BLACK,
      behavior = BEHAVIOR_BUTTON,
      onClick = function() watch:modify(function(v) return not v end) end,
      watch = watch,
      children = children,
    }
  end
end

local components = new_tab(COMPONENTS, 0)
for i = 0, COMPONENTS - 1 do
  components[i + 1] = simpleComponent(i, Watched.new(i % 3 == 0))
end

local function root()
  return {
    size = flex(),
    flow = FLOW_VERTICAL,
    children = components,
  }
end

local function testUi(entry)
  if entry == nil then
    return true
  end
  local kind = type(entry)
  if kind == "function" then
    entry = entry()
    kind = type(entry)
  end
  if kind == "table" then
    local children = entry.children
    if children == nil then
      return true
    end
    if type(children) == "table" and children[1] ~= nil then
      for k = 1, #children do
        if not testUi(children[k]) then
          return false
        end
      end
      return true
    end
    return testUi(children)
  end
  return false
end

local function frames()
  for _ = 1, FRAMES do
    testUi(root)
  end
  return withText
end

benchmark("profile darg ui", 20, frames)
