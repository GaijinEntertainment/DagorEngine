local benchmark = require("profile").benchmark

local function fib(n)
  if n < 2 then
    return n
  end
  return fib(n - 2) + fib(n - 1)
end

benchmark("fibonacci recursive", 20, function() return fib(31) end)
