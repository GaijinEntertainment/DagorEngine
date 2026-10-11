dofile("profile.nut")

local function isPrime(n) {
  for (local i = 2; i < n; i++)
    if (n % i == 0)
      return false
  return true
}

local function countPrimes(n) {
  local count = 0
  for (local i = 2; i <= n; i++)
    if (isPrime(i))
      count++
  return count
}

benchmark("primes loop", 20, @() countPrimes(14000))
