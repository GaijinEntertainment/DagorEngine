// Public domain, ported from Lua
import { benchmark } from "./profile.js";

const COUNT = 100000;

// lagged Fibonacci, same numbers in every port
function createRng(seed) {
  const ring = new Array(17).fill(0);
  for (let i = 16; i >= 0; i--) {
    seed = (seed * 9069) % 0x80000000;
    ring[i] = seed;
  }
  let j = 0;
  return () => {
    let h = j - 5;
    if (h < 0)
      h += 17;
    let k = ring[h] - ring[j];
    if (k < 0)
      k += 2147483647;
    ring[j] = k;
    j = j < 16 ? j + 1 : 0;
    return k;
  };
}

const rand = createRng(12345);
const data = Array.from({ length: COUNT }, () => rand());

const sortCopy = () => data.toSorted((a, b) => b - a)[0];

benchmark("sort", 10, sortCopy);
