class P {
  x = 0
  constructor(x) { this.x = x }
}

class Q(P) {
  y = 0
  function constructor(x, y) {
    base.constructor(x)
    this.y = y
  }
}

class K {
  k = 0
  constructor = function(v) { this.k = v }
}

let q = Q(1, 2)
println($"derived: {q.x} {q.y}")
println($"method type: {typeof P.constructor}")
println($"member key in a class body: {K(3).k}")

function make(constructor) { return $"{constructor}!" }
let made = make("arg")
println($"param: {made}")

let constructor = "local"
let t = { constructor = "key", n = 1 }
let u = { constructor }
println($"names: {constructor} {t.constructor} {u.constructor}")
