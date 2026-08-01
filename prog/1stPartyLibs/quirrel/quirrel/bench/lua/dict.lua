function bxor (a,b)
  local floor = math.floor
  local r = 0
  for i = 0, 31 do
    local x = a / 2 + b / 2
    if x ~= floor (x) then
      r = r + 2^i
    end
    a = floor (a / 2)
    b = floor (b / 2)
  end
  return r
end

local function clear(tab)
  for k,v in pairs(tab) do tab[k]=nil end--clear
end

local function dict(tab, src)
  local max = 0
  local n = 0
  for i,l in ipairs(src) do
    local tabL
  	if (tab[l]) then tabL = tab[l] + 1; tab[l] = tabL
  	else n, tab[l], tabL = n + 1, 1,1 end
  	max = max > tabL and max or tabL
  end
  return max
end

local tab = {}
local src = {}
local n = 500000
local modn = n
for i = 1, n do
    --local num = math.random(1,modn) * 271828183
    local num = bxor(271828183, i*119)%modn
    table.insert(src, string.format("_%d", num))
end

loadfile("profile.lua")()
io.write(string.format("\"dictionary\", %.8f, 20\n", profile_it(20, function () clear(tab); dict(tab, src); end)))
