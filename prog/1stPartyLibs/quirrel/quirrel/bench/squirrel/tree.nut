// treap from https://github.com/frol/completely-unscientific-benchmarks
dofile("profile.nut")

const OPS = 200000

local seed = 42
local function random() {
  seed = seed * 16807 % 2147483647
  return seed
}

// a local function cannot see itself
local merge
merge = function(lower, greater) {
  if (lower == null)
    return greater
  if (greater == null)
    return lower
  if (lower.y < greater.y) {
    lower.right = merge(lower.right, greater)
    return lower
  }
  greater.left = merge(lower, greater.left)
  return greater
}

local splitBinary
splitBinary = function(orig, value) {
  if (orig == null)
    return [null, null]
  if (orig.x < value) {
    local parts = splitBinary(orig.right, value)
    orig.right = parts[0]
    return [orig, parts[1]]
  }
  local parts = splitBinary(orig.left, value)
  orig.left = parts[1]
  return [parts[0], orig]
}

local function merge3(lower, equal, greater) {
  return merge(merge(lower, equal), greater)
}

local function split(orig, value) {
  local lowerSplit = splitBinary(orig, value)
  local upperSplit = splitBinary(lowerSplit[1], value + 1)
  return [lowerSplit[0], upperSplit[0], upperSplit[1]]
}

local function hasValue(tree, x) {
  local parts = split(tree.root, x)
  local equal = parts[1]
  tree.root = merge3(parts[0], equal, parts[2])
  return equal != null
}

local function insert(tree, x) {
  local parts = split(tree.root, x)
  local equal = parts[1]
  if (equal == null)
    equal = { x = x, y = random(), left = null, right = null }
  tree.root = merge3(parts[0], equal, parts[2])
}

local function erase(tree, x) {
  local parts = split(tree.root, x)
  tree.root = merge(parts[0], parts[2])
}

local function treeOps() {
  local tree = { root = null }
  local cur = 5
  local found = 0
  for (local i = 1; i <= OPS; i++) {
    cur = (cur * 57 + 43) % 10007
    local op = i % 3
    if (op == 0)
      insert(tree, cur)
    else if (op == 1)
      erase(tree, cur)
    else if (hasValue(tree, cur))
      found++
  }
  return found
}

benchmark("tree", 10, treeOps)
