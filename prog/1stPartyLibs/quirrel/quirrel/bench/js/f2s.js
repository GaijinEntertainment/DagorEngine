import { benchmark } from "./profile.js";

const COUNT = 10000;
const PASSES = 100;

const floats = Array.from({ length: COUNT }, (_, i) => (i + 1) * 1.0001);

// no result: float formatting differs per language
function totalLength(nums) {
  const n = nums.length;
  let total = 0;
  for (let pass = 0; pass < PASSES; pass++)
    for (let i = 0; i < n; i++)
      total += nums[i].toString().length;
}

benchmark("float2string", 20, () => totalLength(floats));
