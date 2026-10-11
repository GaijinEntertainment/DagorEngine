from "profile.nut" import benchmark

const KEYS = 500000
const PASSES = 10

let wordList = array(KEYS)
for (local i = 0; i < KEYS; i++)
  wordList[i] = $"_{(271828183 ^ (i * 119)) % KEYS}"

function maxCount(words) {
  let counts = {}
  local best = 0
  foreach (w in words) {
    let n = (counts?[w] ?? 0) + 1
    counts[w] <- n
    if (n > best)
      best = n
  }
  return best
}

function passes() {
  local best = 0
  for (local i = 0; i < PASSES; i++)
    best = maxCount(wordList)
  return best
}

benchmark("dictionary", 20, passes)
