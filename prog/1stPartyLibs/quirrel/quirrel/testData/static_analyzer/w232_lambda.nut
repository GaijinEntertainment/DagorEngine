
function foo() {}

let w = foo()

local vv = true
let wdata = foo()
local _u

if (type(w) == "table") {
  _u = w?.update ?? @(v) wdata(v)
  vv = w?.vv ?? vv
} else {
  _u = w
}


if (vv) {
    foo()
}