--!strict
local profile = require("./profile")

local function fib(n: number): number
  if n < 2 then
    return n
  end
  return fib(n - 2) + fib(n - 1)
end

profile.benchmark("fibonacci recursive", 20, function()
  return fib(31)
end)
