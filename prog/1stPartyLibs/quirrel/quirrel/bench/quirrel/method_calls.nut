// class instance field access + method dispatch
from "profile.nut" import vmBench

const N = 3000000

class Point {
  x = 0.0
  y = 0.0
  constructor(x, y) {
    this.x = x
    this.y = y
  }
  function len2() {
    return this.x * this.x + this.y * this.y
  }
  function addTo(o) {
    this.x += o.x
    this.y += o.y
  }
}

function calls() {
  let p = Point(0.0, 0.0)
  let d = Point(0.001, 0.002)
  local sum = 0.0
  for (local i = 0; i < N; i++) {
    p.addTo(d)
    sum += p.len2()
  }
  return sum > 0.0 ? 1 : 0
}

vmBench("method_calls", calls)
