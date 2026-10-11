--!strict
local profile = require("./profile")

local function isPrime(n: number): boolean
  for i = 2, n - 1 do
    if n % i == 0 then
      return false
    end
  end
  return true
end

local function countPrimes(n: number): number
  local count = 0
  for i = 2, n do
    if isPrime(i) then
      count += 1
    end
  end
  return count
end

profile.benchmark("primes loop", 20, function()
  return countPrimes(14000)
end)
