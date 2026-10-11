// allocation and collection pressure (binary-trees style)
from "profile.nut" import vmBench

const DEPTH = 13
const TREES = 10

function mkTree(depth) {
  if (depth == 0)
    return { left = null, right = null }
  return { left = mkTree(depth - 1), right = mkTree(depth - 1) }
}

function checkTree(t) {
  if (t.left == null)
    return 1
  return 1 + checkTree(t.left) + checkTree(t.right)
}

function trees() {
  local sum = 0
  for (local i = 0; i < TREES; i++)
    sum += checkTree(mkTree(DEPTH))
  return sum
}

vmBench("binarytrees", trees)
