--!strict
-- Public domain
local profile = require("./profile")

local COUNT = 100000

-- lagged Fibonacci, same numbers in every port
local function createRng(seed: number): () -> number
  local ring = table.create(17, 0)
  for i = 17, 1, -1 do
    seed = (seed * 9069) % 0x80000000
    ring[i] = seed
  end
  local j = 1
  return function(): number
    local h = j - 5
    if h < 1 then
      h += 17
    end
    local k = ring[h] - ring[j]
    if k < 0 then
      k += 2147483647
    end
    ring[j] = k
    j = if j < 17 then j + 1 else 1
    return k
  end
end

local rand = createRng(12345)
local data: { number } = table.create(COUNT)
for i = 1, COUNT do
  data[i] = rand()
end

local function sortCopy(): number
  local arr = table.clone(data)
  table.sort(arr, function(a: number, b: number): boolean
    return a > b
  end)
  return arr[1]
end

profile.benchmark("sort", 10, sortCopy)
