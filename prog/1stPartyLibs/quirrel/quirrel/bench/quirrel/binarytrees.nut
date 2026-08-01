// allocation and collection pressure (binary-trees style)
let { clock } = require("datetime")

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

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local sum = 0
  for (local i = 0; i < 10; i++)
    sum += checkTree(mkTree(13))
  println("BENCH binarytrees", ((clock() - t0) * 1000), $"ms (r={sum})")
}
