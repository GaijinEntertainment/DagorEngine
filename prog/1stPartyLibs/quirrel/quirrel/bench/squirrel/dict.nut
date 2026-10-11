dofile("profile.nut")

const KEYS = 500000
const PASSES = 10

local wordList = array(KEYS)
for (local i = 0; i < KEYS; i++)
  wordList[i] = "_" + (271828183 ^ (i * 119)) % KEYS

local function maxCount(words) {
  local counts = {}
  local best = 0
  foreach (w in words) {
    local n = w in counts ? counts[w] + 1 : 1
    counts[w] <- n
    if (n > best)
      best = n
  }
  return best
}

local function passes() {
  local best = 0
  for (local i = 0; i < PASSES; i++)
    best = maxCount(wordList)
  return best
}

benchmark("dictionary", 20, passes)
