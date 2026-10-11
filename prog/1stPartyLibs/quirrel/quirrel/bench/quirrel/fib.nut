// recursive call overhead
from "profile.nut" import vmBench

function fib(n) {
  if (n < 2)
    return n
  return fib(n - 1) + fib(n - 2)
}

vmBench("fib", @() fib(30))
