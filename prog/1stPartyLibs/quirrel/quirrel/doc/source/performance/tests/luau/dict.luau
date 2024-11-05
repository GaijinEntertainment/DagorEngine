local function dict(tab, src)
  table.clear(tab)
  local max = 0
  for i,l in src do
    local tabL = (tab[l] or 0) + 1
    tab[l] = tabL
    max = math.max(max, tabL)
  end
  return max
end

local tab = {}
local src = {}
local n = 500000
local modn = n
for i = 1, n do
    --local num = math.random(1,modn) * 271828183
    local num = bit32.bxor(271828183, i*119)%modn
    table.insert(src, string.format("_%d", num))
end

function profile_it(profiles, fn)
  local res
  for i = 1, profiles do
    local start = os.clock()
    fn()
    local measured = os.clock() - start
    if i == 1 or res > measured then res = measured end
  end
  return res
end

print(string.format("\"dictionary\", %.8f, 20", profile_it(20, function () dict(tab, src); end)))
