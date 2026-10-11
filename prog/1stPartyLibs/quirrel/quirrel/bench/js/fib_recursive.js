import { benchmark } from "./profile.js";

function fib(n) {
  if (n < 2)
    return n;
  return fib(n - 2) + fib(n - 1);
}

benchmark("fibonacci recursive", 20, () => fib(31));
