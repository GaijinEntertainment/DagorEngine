-- Public domain
local create <const> = table.create
local move <const> = table.move
local sort <const> = table.sort
local benchmark <const> = require("profile").benchmark

local COUNT <const> = 100000

-- lagged Fibonacci, same numbers in every port
local function createRng(seed)
  local ring = create(17)
  for i = 17, 1, -1 do
    seed = (seed * 9069) % 0x80000000
    ring[i] = seed
  end
  local j = 1
  return function()
    local h = j - 5
    if h < 1 then
      h = h + 17
    end
    local k = ring[h] - ring[j]
    if k < 0 then
      k = k + 2147483647
    end
    ring[j] = k
    j = j < 17 and j + 1 or 1
    return k
  end
end

local rand <const> = createRng(12345)
local data <const> = create(COUNT)
for i = 1, COUNT do
  data[i] = rand()
end

local function descending(a, b)
  return a > b
end

local function sortCopy()
  local arr = move(data, 1, COUNT, 1, create(COUNT))
  sort(arr, descending)
  return arr[1]
end

benchmark("sort", 10, sortCopy)
