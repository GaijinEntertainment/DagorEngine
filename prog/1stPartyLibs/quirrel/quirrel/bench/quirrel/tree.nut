// treap from https://github.com/frol/completely-unscientific-benchmarks
from "profile.nut" import benchmark

const OPS = 200000

local seed = 42
function random() {
  seed = seed * 16807 % 2147483647
  return seed
}

function merge(lower, greater) {
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

function splitBinary(orig, value) {
  if (orig == null)
    return [null, null]
  if (orig.x < value) {
    let [lower, greater] = splitBinary(orig.right, value)
    orig.right = lower
    return [orig, greater]
  }
  let [lower, greater] = splitBinary(orig.left, value)
  orig.left = greater
  return [lower, orig]
}

function merge3(lower, equal, greater) {
  return merge(merge(lower, equal), greater)
}

function split(orig, value) {
  let [lower, equalGreater] = splitBinary(orig, value)
  let [equal, greater] = splitBinary(equalGreater, value + 1)
  return [lower, equal, greater]
}

function hasValue(tree, x) {
  let [lower, equal, greater] = split(tree.root, x)
  tree.root = merge3(lower, equal, greater)
  return equal != null
}

function insert(tree, x) {
  local [lower, equal, greater] = split(tree.root, x)
  if (equal == null)
    equal = { x, y = random(), left = null, right = null }
  tree.root = merge3(lower, equal, greater)
}

function erase(tree, x) {
  let [lower, _, greater] = split(tree.root, x)
  tree.root = merge(lower, greater)
}

function treeOps() {
  let tree = { root = null }
  local cur = 5
  local found = 0
  for (local i = 1; i <= OPS; i++) {
    cur = (cur * 57 + 43) % 10007
    let op = i % 3
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
