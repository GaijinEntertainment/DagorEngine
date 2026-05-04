// Is this test correct?
// It tests for `already-required` warning

function foo() {}
let x = foo()
let d = foo()

let { _m=null } = require_optional("m.nut")

let { _g=null } = d ? require_optional("a.nut")
  : x?.is_x ? require_optional("b.nut")
  : x?.is_s ? require_optional("c.nut")
  : x?.is_a ? require_optional("d.nut")
  : require_optional("a.nut")
