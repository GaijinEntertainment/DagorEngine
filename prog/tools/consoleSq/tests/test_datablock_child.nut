import "DataBlock" as DataBlock
from "debug" import collectgarbage

function mkRoot() {
  let root = DataBlock()
  root.addNewBlock("sub").addStr("name", "value")
  return root
}

function churn() {
  for (local i = 0; i < 64; i++)
    DataBlock().addNewBlock($"b{i}").addStr("key", "xxxxxxxxxxxxxxxxxxxxxxxx")
}

let getters = {
  getBlock = @(root) root.getBlock(0)
  getBlockByName = @(root) root.getBlockByName("sub")
  getBlockByNameEx = @(root) root.getBlockByNameEx("sub")
  addBlock = @(root) root.addBlock("sub")
  get = @(root) root.sub
  modulo = @(root) (root % "sub")[0]
}

foreach (name, getChild in getters) {
  local root = mkRoot()
  let rootRef = root.weakref()
  let child = getChild(root)
  root = null
  collectgarbage()
  churn()
  assert(rootRef.ref() != null && child.getParamName(0) == "name", $"{name}: the child outlives its root")
}

local missRoot = mkRoot()
let missRootRef = missRoot.weakref()
assert(missRoot.getBlockByName("none") == null && missRoot.getBlock(99) == null, "a missing block is null")
let empty = missRoot.getBlockByNameEx("none")
missRoot = null
collectgarbage()
assert(missRootRef.ref() == null && empty.isEmpty(), "the empty block of a miss keeps no owner")
empty.setInt("x", 1)
assert(DataBlock().getBlockByNameEx("none").paramCount() == 0, "a write on a miss changes no other miss")

local deepRoot = mkRoot()
let deepRootRef = deepRoot.weakref()
let grandchild = deepRoot.sub.addNewBlock("deep")
deepRoot = null
grandchild.addInt("num", 5)
collectgarbage()
churn()
assert(deepRootRef.ref() != null && grandchild.getInt("num", 0) == 5, "a grandchild outlives its root")

println("test_datablock_child PASSED")
