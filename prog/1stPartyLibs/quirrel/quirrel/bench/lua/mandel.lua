-- float-heavy inner loop
local vmBench <const> = require("profile").vmBench

local W <const> = 320
local H <const> = 160
local MAXIT <const> = 60

local function mandel()
  local inside = 0
  for py = 0, H - 1 do
    local ci = py * 2.0 / H - 1.0
    for px = 0, W - 1 do
      local cr = px * 3.0 / W - 2.0
      local zr = 0.0
      local zi = 0.0
      local it = 0
      while it < MAXIT and zr * zr + zi * zi < 4.0 do
        local nzr = zr * zr - zi * zi + cr
        zi = 2.0 * zr * zi + ci
        zr = nzr
        it = it + 1
      end
      if it == MAXIT then
        inside = inside + 1
      end
    end
  end
  return inside
end

vmBench("mandel", mandel)
