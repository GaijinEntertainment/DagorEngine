-- float-heavy inner loop
local W = 320
local H = 160
local MAXIT = 60

for rep = 1, 5 do
  local t0 = os.clock()
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
      if it == MAXIT then inside = inside + 1 end
    end
  end
  print("BENCH mandel " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. inside .. ")")
end
