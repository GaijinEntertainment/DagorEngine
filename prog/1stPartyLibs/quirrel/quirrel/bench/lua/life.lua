-- 2d array access, integer logic (Game of Life)
local create <const> = table.create
local vmBench <const> = require("profile").vmBench

local W <const> = 48
local H <const> = 48
local GENS <const> = 300

local function mkGrid()
  local g = create(H)
  for y = 0, H - 1 do
    local row = create(W)
    for x = 0, W - 1 do
      row[x + 1] = (x * 7 + y * 13) % 5 == 0 and 1 or 0
    end
    g[y + 1] = row
  end
  return g
end

local function step(src, dst)
  for y = 1, H do
    local up = src[(y + H - 2) % H + 1]
    local mid = src[y]
    local down = src[y % H + 1]
    local out = dst[y]
    for x = 1, W do
      local xm = (x + W - 2) % W + 1
      local xp = x % W + 1
      local n = up[xm] + up[x] + up[xp]
              + mid[xm]         + mid[xp]
              + down[xm] + down[x] + down[xp]
      out[x] = (n == 3 or (n == 2 and mid[x] == 1)) and 1 or 0
    end
  end
end

local function life()
  local a = mkGrid()
  local b = mkGrid()
  for _ = 1, GENS do
    step(a, b)
    a, b = b, a
  end
  local alive = 0
  for y = 1, H do
    local row = a[y]
    for x = 1, W do
      alive = alive + row[x]
    end
  end
  return alive
end

vmBench("life", life)
