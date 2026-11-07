--!strict
-- 2d array access, integer logic (Game of Life)
local profile = require("./profile")

type Grid = { { number } }

local W = 48
local H = 48
local GENS = 300

local function mkGrid(): Grid
  local g: Grid = table.create(H)
  for y = 0, H - 1 do
    local row = table.create(W, 0)
    for x = 0, W - 1 do
      row[x + 1] = if (x * 7 + y * 13) % 5 == 0 then 1 else 0
    end
    g[y + 1] = row
  end
  return g
end

local function step(src: Grid, dst: Grid)
  for y = 1, H do
    local up = src[(y - 2) % H + 1]
    local mid = src[y]
    local down = src[y % H + 1]
    local out = dst[y]
    for x = 1, W do
      local xm = (x - 2) % W + 1
      local xp = x % W + 1
      local n = up[xm] + up[x] + up[xp]
        + mid[xm] + mid[xp]
        + down[xm] + down[x] + down[xp]
      out[x] = if n == 3 or (n == 2 and mid[x] == 1) then 1 else 0
    end
  end
end

local function life(): number
  local a = mkGrid()
  local b = mkGrid()
  for _ = 1, GENS do
    step(a, b)
    a, b = b, a
  end
  local alive = 0
  for _, row in a do
    for _, cell in row do
      alive += cell
    end
  end
  return alive
end

profile.vmBench("life", life)
