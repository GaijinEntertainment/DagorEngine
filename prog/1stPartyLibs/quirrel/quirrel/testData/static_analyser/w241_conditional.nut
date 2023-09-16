function foo() {}
let x = foo()
let d = foo()

let { _m } = require("m.nut")

let { _g } = d ? require("a.nut")
  : x.is_x ? require("b.nut")
  : x.is_s ? require("c.nut")
  : x.is_a ? require("d.nut")
  : require("a.nut")