import { benchmark } from "./profile.js";

// no result: the sum overflows
function fibLoop(n) {
  let last = 0;
  let cur = 1;
  for (let i = 1; i < n; i++) {
    const next = last + cur;
    last = cur;
    cur = next;
  }
}

benchmark("fibonacci loop", 20, () => fibLoop(6511134));
