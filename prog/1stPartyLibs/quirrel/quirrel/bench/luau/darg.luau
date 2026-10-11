--!strict
-- the shape of a daRg UI: builders that return component tables
local profile = require("./profile")

type Component = { [string]: any }
type Builder = () -> Component

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
local DEF_FLEX = table.freeze({})

local Watched = {}
Watched.__index = Watched

type Watched = typeof(setmetatable({} :: { value: any }, Watched))

function Watched.new(value: any): Watched
  return setmetatable({ value = value }, Watched)
end

function Watched.get(self: Watched): any
  return self.value
end

function Watched.modify(self: Watched, f: (any) -> any)
  self.value = f(self.value)
end

-- Park-Miller: same layout in every port
local seed = 42
local function rint(max: number): number
  seed = seed * 16807 % 2147483647
  return seed % max
end

local function sw(val: number): number
  return math.floor(val / 100.0 * SCREEN_WIDTH)
end

local function sh(val: number): number
  return math.floor(val / 100.0 * SCREEN_HEIGHT)
end

local function Color(r: number, g: number, b: number, a: number?): number
  return bit32.lshift(a or 255, 24) + bit32.lshift(r, 16) + bit32.lshift(g, 8) + b
end

local function hdpx(val: number): number
  return val * SCREEN_HEIGHT / sh(100)
end

local function flex(val: any?): any
  return if val == nil then DEF_FLEX else { val = val }
end

local BORDER_WIDTH = table.freeze({ 1, 1, 1, 1 })
local PADDING = table.freeze({ hdpx(4), hdpx(5) })
local MARGIN = table.freeze({ hdpx(5), hdpx(3) })
local BLACK = Color(0, 0, 0)

local withText = 0

local function simpleComponent(i: number, watch: Watched): Builder
  local pos = { sw(rint(80)), sh(rint(80)) }
  local size = { sh(rint(15) + 2), sh(rint(15) + 2) }
  local color = Color(rint(256), rint(256), rint(256))
  local textCanBePlaced = size[1] > hdpx(150)
  local addText = rint(5) == 1
  local children: { Builder } = {}
  if BORDERS then
    table.insert(children, function(): Component
      return {
        borderWidth = BORDER_WIDTH,
        rendObj = ROBJ_FRAME,
        size = flex(),
      }
    end)
    if textCanBePlaced then
      withText += 1
      table.insert(children, function(): Component
        return {
          watch = watch,
          rendObj = ROBJ_TEXT,
          text = if addText or not watch:get() then `{i}{watch:get()}` else nil,
        }
      end)
    end
  end
  return function(): Component
    return {
      rendObj = ROBJ_SOLID,
      size = size,
      key = {},
      pos = pos,
      valign = ALIGN_CENTER,
      halign = ALIGN_CENTER,
      padding = PADDING,
      margin = MARGIN,
      color = if watch:get() then color else BLACK,
      behavior = BEHAVIOR_BUTTON,
      onClick = function()
        watch:modify(function(v)
          return not v
        end)
      end,
      watch = watch,
      children = children,
    }
  end
end

local components: { Builder } = table.create(COMPONENTS)
for i = 0, COMPONENTS - 1 do
  components[i + 1] = simpleComponent(i, Watched.new(i % 3 == 0))
end

local function root(): Component
  return {
    size = flex(),
    flow = FLOW_VERTICAL,
    children = components,
  }
end

local function testUi(entry: any): boolean
  if entry == nil then
    return true
  end
  if type(entry) == "function" then
    entry = entry()
  end
  if type(entry) == "table" then
    local children = entry.children
    if children == nil then
      return true
    end
    if type(children) == "table" then
      for _, child in children do
        if not testUi(child) then
          return false
        end
      end
      return true
    end
    return testUi(children)
  end
  return false
end

local function frames(): number
  for _ = 1, FRAMES do
    testUi(root)
  end
  return withText
end

profile.benchmark("profile darg ui", 20, frames)
