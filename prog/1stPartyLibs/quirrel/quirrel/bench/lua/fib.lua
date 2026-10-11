-- recursive call overhead
local vmBench <const> = require("profile").vmBench

local function fib(n)
  if n < 2 then
    return n
  end
  return fib(n - 1) + fib(n - 2)
end

vmBench("fib", function() return fib(30) end)
