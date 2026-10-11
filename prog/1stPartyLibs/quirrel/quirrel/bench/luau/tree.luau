--!strict
--Copyright and license: https://github.com/frol/completely-unscientific-benchmarks
local profile = require("./profile")

type Node = { x: number, y: number, left: Node?, right: Node? }
type Tree = { root: Node? }

local OPS = 200000

local seed = 42
local function random(): number
  seed = seed * 16807 % 2147483647
  return seed
end

local function merge(lower: Node?, greater: Node?): Node?
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

local function splitBinary(orig: Node?, value: number): (Node?, Node?)
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

local function merge3(lower: Node?, equal: Node?, greater: Node?): Node?
  return merge(merge(lower, equal), greater)
end

local function split(orig: Node?, value: number): (Node?, Node?, Node?)
  local lower, equalGreater = splitBinary(orig, value)
  local equal, greater = splitBinary(equalGreater, value + 1)
  return lower, equal, greater
end

local function hasValue(tree: Tree, x: number): boolean
  local lower, equal, greater = split(tree.root, x)
  tree.root = merge3(lower, equal, greater)
  return equal ~= nil
end

local function insert(tree: Tree, x: number)
  local lower, equal, greater = split(tree.root, x)
  if equal == nil then
    equal = { x = x, y = random(), left = nil, right = nil }
  end
  tree.root = merge3(lower, equal, greater)
end

local function erase(tree: Tree, x: number)
  local lower, _, greater = split(tree.root, x)
  tree.root = merge(lower, greater)
end

local function treeOps(): number
  local tree: Tree = { root = nil }
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
      found += 1
    end
  end
  return found
end

profile.benchmark("tree", 10, treeOps)
