-- allocation and collection pressure (binary-trees style)
local vmBench <const> = require("profile").vmBench

local DEPTH <const> = 13
local TREES <const> = 10

local function mkTree(depth)
  if depth == 0 then
    return { left = false, right = false }
  end
  return { left = mkTree(depth - 1), right = mkTree(depth - 1) }
end

local function checkTree(t)
  if not t.left then
    return 1
  end
  return 1 + checkTree(t.left) + checkTree(t.right)
end

local function trees()
  local sum = 0
  for _ = 1, TREES do
    sum = sum + checkTree(mkTree(DEPTH))
  end
  return sum
end

vmBench("binarytrees", trees)
