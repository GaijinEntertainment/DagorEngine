-- closure creation and invocation with captured locals, the shape of daRg
-- @() builder components
local N = 1000000

for rep = 1, 5 do
  local t0 = os.clock()
  local sum = 0
  local kept = {}
  local nk = 0
  for i = 0, N - 1 do
    local a = i
    local b = i * 2
    local f = function() return a + b end
    sum = sum + f()
    if i % 1000 == 0 then
      nk = nk + 1
      kept[nk] = f
    end
  end
  for _, f in ipairs(kept) do
    sum = sum + f()
  end
  print("BENCH closure_storm " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. sum .. ")")
end
