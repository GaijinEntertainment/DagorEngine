-- 2d array access, integer logic (Game of Life)
local W = 48
local H = 48
local GENS = 300

local function mkGrid()
  local g = {}
  for y = 0, H - 1 do
    local row = {}
    for x = 0, W - 1 do
      row[x] = ((x * 7 + y * 13) % 5 == 0) and 1 or 0
    end
    g[y] = row
  end
  return g
end

local function step(src, dst)
  for y = 0, H - 1 do
    local ym = (y + H - 1) % H
    local yp = (y + 1) % H
    for x = 0, W - 1 do
      local xm = (x + W - 1) % W
      local xp = (x + 1) % W
      local n = src[ym][xm] + src[ym][x] + src[ym][xp]
              + src[y][xm]               + src[y][xp]
              + src[yp][xm] + src[yp][x] + src[yp][xp]
      if n == 3 or (n == 2 and src[y][x] == 1) then
        dst[y][x] = 1
      else
        dst[y][x] = 0
      end
    end
  end
end

for rep = 1, 5 do
  local t0 = os.clock()
  local a = mkGrid()
  local b = mkGrid()
  for g = 1, GENS do
    step(a, b)
    a, b = b, a
  end
  local alive = 0
  for y = 0, H - 1 do
    for x = 0, W - 1 do
      alive = alive + a[y][x]
    end
  end
  print("BENCH life " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. alive .. ")")
end
