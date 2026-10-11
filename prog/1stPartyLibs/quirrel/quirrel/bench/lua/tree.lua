--Copyright and license: https://github.com/frol/completely-unscientific-benchmarks
local benchmark <const> = require("profile").benchmark

local OPS <const> = 200000

local seed = 42
local function random()
  seed = seed * 16807 % 2147483647
  return seed
end

local function merge(lower, greater)
  if lower == nil then
    return greater
  end
  if greater == nil then
    return lower
  end
  if lower.y < greater.y then
    lower.right = merge(lower.right, greater)
    return lower
  end
  greater.left = merge(lower, greater.left)
  return greater
end

local function splitBinary(orig, value)
  if orig == nil then
    return nil, nil
  end
  if orig.x < value then
    local lower, greater = splitBinary(orig.right, value)
    orig.right = lower
    return orig, greater
  end
  local lower, greater = splitBinary(orig.left, value)
  orig.left = greater
  return lower, orig
end

local function merge3(lower, equal, greater)
  return merge(merge(lower, equal), greater)
end

local function split(orig, value)
  local lower, equalGreater = splitBinary(orig, value)
  local equal, greater = splitBinary(equalGreater, value + 1)
  return lower, equal, greater
end

local function hasValue(tree, x)
  local lower, equal, greater = split(tree.root, x)
  tree.root = merge3(lower, equal, greater)
  return equal ~= nil
end

local function insert(tree, x)
  local lower, equal, greater = split(tree.root, x)
  if equal == nil then
    equal = { x = x, y = random(), left = nil, right = nil }
  end
  tree.root = merge3(lower, equal, greater)
end

local function erase(tree, x)
  local lower, _, greater = split(tree.root, x)
  tree.root = merge(lower, greater)
end

local function treeOps()
  local tree = { root = nil }
  local cur = 5
  local found = 0
  for i = 1, OPS do
    cur = (cur * 57 + 43) % 10007
    local op = i % 3
    if op == 0 then
      insert(tree, cur)
    elseif op == 1 then
      erase(tree, cur)
    elseif hasValue(tree, cur) then
      found = found + 1
    end
  end
  return found
end

benchmark("tree", 10, treeOps)
