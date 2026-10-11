import { benchmark } from "./profile.js";

const KEYS = 500000;
const PASSES = 10;

const wordList = Array.from({ length: KEYS }, (_, i) => `_${(271828183 ^ (i * 119)) % KEYS}`);

function maxCount(words) {
  const counts = new Map();
  let best = 0;
  for (let i = 0, len = words.length; i < len; i++) {
    const w = words[i];
    const n = (counts.get(w) ?? 0) + 1;
    counts.set(w, n);
    if (n > best)
      best = n;
  }
  return best;
}

function passes() {
  let best = 0;
  for (let i = 0; i < PASSES; i++)
    best = maxCount(wordList);
  return best;
}

benchmark("dictionary", 20, passes);
