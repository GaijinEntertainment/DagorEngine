--!strict
-- metatable instance field access + method dispatch
local profile = require("./profile")

local N = 3000000

local Point = {}
Point.__index = Point

type Point = typeof(setmetatable({} :: { x: number, y: number }, Point))

function Point.new(x: number, y: number): Point
  return setmetatable({ x = x, y = y }, Point)
end

function Point.len2(self: Point): number
  return self.x * self.x + self.y * self.y
end

function Point.addTo(self: Point, o: Point)
  self.x += o.x
  self.y += o.y
end

local function calls(): number
  local p = Point.new(0.0, 0.0)
  local d = Point.new(0.001, 0.002)
  local sum = 0.0
  for _ = 1, N do
    p:addTo(d)
    sum += p:len2()
  end
  return if sum > 0.0 then 1 else 0
end

profile.vmBench("method_calls", calls)
