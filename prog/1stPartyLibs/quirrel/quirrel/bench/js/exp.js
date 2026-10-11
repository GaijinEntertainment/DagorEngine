import { benchmark } from "./profile.js";

const { exp } = Math;
const N = 10000000;

function expLoop(n) {
  let sum = 0.0;
  for (let i = 0; i < n; i++)
    sum += exp(1.0 / (1.0 + i));
  return sum;
}

benchmark("exp loop", 20, () => expLoop(N).toFixed(6));
