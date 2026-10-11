import { benchmark } from "./profile.js";

const COUNT = 10000;
const PASSES = 100;

const numberStrings = Array.from({ length: COUNT }, (_, i) => ((i + 1) * 1.0001).toFixed(4));

function total(strs) {
  const n = strs.length;
  let sum = 0.0;
  for (let pass = 0; pass < PASSES; pass++)
    for (let i = 0; i < n; i++)
      sum += +strs[i];
  return sum.toFixed(4);
}

benchmark("string2float", 20, () => total(numberStrings));
