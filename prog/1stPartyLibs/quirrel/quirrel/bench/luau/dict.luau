--!strict
local profile = require("./profile")

local KEYS = 500000
local PASSES = 10

local wordList: { string } = table.create(KEYS)
for i = 0, KEYS - 1 do
  wordList[i + 1] = `_{bit32.bxor(271828183, i * 119) % KEYS}`
end

local function maxCount(words: { string }): number
  local counts: { [string]: number } = {}
  local best = 0
  for _, w in words do
    local n = (counts[w] or 0) + 1
    counts[w] = n
    if n > best then
      best = n
    end
  end
  return best
end

local function passes(): number
  local best = 0
  for _ = 1, PASSES do
    best = maxCount(wordList)
  end
  return best
end

profile.benchmark("dictionary", 20, passes)
