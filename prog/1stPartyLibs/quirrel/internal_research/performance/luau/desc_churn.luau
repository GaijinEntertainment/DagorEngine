--!strict
-- component-description churn: build and discard small string-keyed tables
-- with nested arrays, the shape of a daRg builder re-evaluation
local profile = require("./profile")

local N = 100000

local function mkChild(i: number)
  return {
    rendObj = 2,
    size = { i % 100, 20 },
    color = 4278190080 + (i % 65536),
    text = "child",
  }
end

local function mkDesc(i: number)
  return {
    rendObj = 1,
    size = { 100, 50 },
    color = 0xFF102030,
    text = "component",
    padding = 4,
    margin = 2,
    flow = 1,
    halign = 0,
    valign = 1,
    gap = 3,
    opacity = 0.5,
    zOrder = i % 10,
    clipChildren = true,
    behavior = false,
    hotkeys = false,
    children = { mkChild(i), mkChild(i + 1), mkChild(i + 2) },
  }
end

local function churn(): number
  local sum = 0
  for i = 0, N - 1 do
    local d = mkDesc(i)
    sum += d.zOrder + #d.children
  end
  return sum
end

profile.vmBench("desc_churn", churn)
