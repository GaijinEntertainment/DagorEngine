local benchmark <const> = require("profile").benchmark

local KEYS <const> = 500000
local PASSES <const> = 10

local wordList <const> = table.create(KEYS)
for i = 0, KEYS - 1 do
  wordList[i + 1] = "_" .. (271828183 ~ (i * 119)) % KEYS
end

local function maxCount(words)
  local counts = {}
  local best = 0
  for i = 1, #words do
    local w = words[i]
    local n = (counts[w] or 0) + 1
    counts[w] = n
    if n > best then
      best = n
    end
  end
  return best
end

local function passes()
  local best = 0
  for _ = 1, PASSES do
    best = maxCount(wordList)
  end
  return best
end

benchmark("dictionary", 20, passes)
