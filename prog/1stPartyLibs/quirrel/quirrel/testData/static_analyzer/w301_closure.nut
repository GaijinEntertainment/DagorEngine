local c = null
local cr = null
function foo() {}
let d = {}
local dp = foo()
let tree = foo()
do {
  c = d[dp].o
  cr = tree.ci.f(@(v) v?.oc == c)
  if (cr){
    dp++
  }
} while (cr && dp < 1000)