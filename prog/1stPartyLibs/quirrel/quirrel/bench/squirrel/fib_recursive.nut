dofile("profile.nut")

// a local function cannot see itself
local fib
fib = function(n) {
  if (n < 2)
    return n
  return fib(n - 2) + fib(n - 1)
}

benchmark("fibonacci recursive", 20, @() fib(31))
