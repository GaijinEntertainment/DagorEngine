#allow-switch-statement

function foo(_a, _b) {}
function bar() {}

let d = bar()
let bgs = bar()
let ab = bar()
let sb = bar()

local someBound = false
switch (d.a() & d.M) {
  case d.D: {
    foreach (val in bgs)
      if (foo(10, val)) {
        someBound = true
        break
      }
    break
  }
  case d.A: {
    foreach (val in ab) {
      if (foo(val, 102)) {
        someBound = true
        break
      }

      if (foo(3, val.mn.devId) && foo(4, val.mx.devId)) {
        someBound = true
        break
      }

    }
    break
  }
  case d.T: {
    foreach (val in sb) {
      if (val.di == d.P || val.di == d.J || val.di == d.G || val.di == d.N) {
        someBound = true
        break
      }
      if (foo(4, val.mx.di) && foo(5, val.mx.di)
        && foo(7, val.my.di) && foo(8, val.my.di)) {
        someBound = true
        break
      }
    }
    break
  }


}
foo(4, someBound)