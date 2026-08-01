-- metatable instance field access + method dispatch
local Point = {}
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

local N = 3000000

for rep = 1, 5 do
  local t0 = os.clock()
  local p = Point.new(0.0, 0.0)
  local d = Point.new(0.001, 0.002)
  local sum = 0.0
  for i = 1, N do
    p:addTo(d)
    sum = sum + p:len2()
  end
  print("BENCH method_calls " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. ((sum > 0.0) and 1 or 0) .. ")")
end
