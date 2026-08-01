-- recursive call overhead
local function fib(n)
  if n < 2 then return n end
  return fib(n - 1) + fib(n - 2)
end

for rep = 1, 5 do
  local t0 = os.clock()
  local r = fib(30)
  print("BENCH fib " .. ((os.clock() - t0) * 1000) .. " ms (r=" .. r .. ")")
end
