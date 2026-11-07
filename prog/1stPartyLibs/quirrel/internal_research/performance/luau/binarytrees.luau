--!strict
-- allocation and collection pressure (binary-trees style)
local profile = require("./profile")

type TreeNode = { left: TreeNode | false, right: TreeNode | false }

local DEPTH = 13
local TREES = 10

local function mkTree(depth: number): TreeNode
  if depth == 0 then
    return { left = false, right = false }
  end
  return { left = mkTree(depth - 1), right = mkTree(depth - 1) }
end

local function checkTree(t: TreeNode): number
  local left = t.left
  if not left then
    return 1
  end
  return 1 + checkTree(left :: TreeNode) + checkTree(t.right :: TreeNode)
end

local function trees(): number
  local sum = 0
  for _ = 1, TREES do
    sum += checkTree(mkTree(DEPTH))
  end
  return sum
end

profile.vmBench("binarytrees", trees)
