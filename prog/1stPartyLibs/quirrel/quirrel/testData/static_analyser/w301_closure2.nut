function foo() {}
let x = foo()
local a = false

function _sss() {
  if (a)
    return
  a = true
  x.st(function() {
    a = false
  })
}