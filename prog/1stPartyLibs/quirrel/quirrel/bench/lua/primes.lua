local benchmark <const> = require("profile").benchmark

local function isPrime(n)
  for i = 2, n - 1 do
    if n % i == 0 then
      return false
    end
  end
  return true
end

local function countPrimes(n)
  local count = 0
  for i = 2, n do
    if isPrime(i) then
      count = count + 1
    end
  end
  return count
end

benchmark("primes loop", 20, function() return countPrimes(14000) end)
