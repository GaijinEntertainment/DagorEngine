// class instance field access + method dispatch
let { clock } = require("datetime")

class Point {
  x = 0.0
  y = 0.0
  constructor(x_, y_) {
    this.x = x_
    this.y = y_
  }
  function len2() {
    return this.x * this.x + this.y * this.y
  }
  function addTo(o) {
    this.x += o.x
    this.y += o.y
  }
}

const N = 3000000

for (local rep = 0; rep < 5; rep++) {
  local t0 = clock()
  local p = Point(0.0, 0.0)
  local d = Point(0.001, 0.002)
  local sum = 0.0
  for (local i = 0; i < N; i++) {
    p.addTo(d)
    sum += p.len2()
  }
  println("BENCH method_calls", ((clock() - t0) * 1000), $"ms ( r =", (sum > 0.0 ? 1 : 0), ")")
}
