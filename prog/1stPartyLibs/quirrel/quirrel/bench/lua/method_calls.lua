-- metatable instance field access + method dispatch
local setmetatable <const> = setmetatable
local vmBench <const> = require("profile").vmBench

local N <const> = 3000000

local Point <const> = {}
Point.__index = Point

function Point.new(x, y)
  return setmetatable({ x = x, y = y }, Point)
end

function Point:len2()
  return self.x * self.x + self.y * self.y
end

function Point:addTo(o)
  self.x = self.x + o.x
  self.y = self.y + o.y
end

local function calls()
  local p = Point.new(0.0, 0.0)
  local d = Point.new(0.001, 0.002)
  local sum = 0.0
  for _ = 1, N do
    p:addTo(d)
    sum = sum + p:len2()
  end
  return sum > 0.0 and 1 or 0
end

vmBench("method_calls", calls)
