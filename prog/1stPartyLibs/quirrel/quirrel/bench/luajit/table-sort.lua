-- Public domain

local benchmark = require("profile").benchmark
local new_tab = require("table.new")
local sort = table.sort

local COUNT = 100000

-- lagged Fibonacci, same numbers in every port
local function createRng(seed)
  local ring = new_tab(17, 0)
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
    if j < 17 then
      j = j + 1
    else
      j = 1
    end
    return k
  end
end

local rand = createRng(12345)
local data = new_tab(COUNT, 0)
for i = 1, COUNT do
  data[i] = rand()
end

local function descending(a, b)
  return a > b
end

local function sortCopy()
  local arr = new_tab(COUNT, 0)
  for i = 1, COUNT do
    arr[i] = data[i]
  end
  sort(arr, descending)
  return arr[1]
end

benchmark("sort", 10, sortCopy)
