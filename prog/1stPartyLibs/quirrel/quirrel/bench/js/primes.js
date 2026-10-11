import { benchmark } from "./profile.js";

function isPrime(n) {
  for (let i = 2; i < n; i++)
    if (n % i === 0)
      return false;
  return true;
}

function countPrimes(n) {
  let count = 0;
  for (let i = 2; i <= n; i++)
    if (isPrime(i))
      count++;
  return count;
}

benchmark("primes loop", 20, () => countPrimes(14000));
