--!strict
-- recursive call overhead
local profile = require("./profile")

local function fib(n: number): number
  if n < 2 then
    return n
  end
  return fib(n - 1) + fib(n - 2)
end

profile.vmBench("fib", function()
  return fib(30)
end)
