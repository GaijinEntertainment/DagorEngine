// In a function, its own name is the function; another visible binding with that name is a compile error

from "math" import abs

let cases = [
  ["const field call", "const M = { abs }\nlet C = class { function M(v) { return M.abs(v) } }"],
  ["const field read", "const M = { x = 7 }\nlet C = class { function M() { return M.x } }"],
  ["const typed let", "const M = { x = 7 }\nlet C = class { function M() { let t: table = M; return t } }"],
  ["const initializer", "const M = { x = 7 }\nlet C = class { function M() { const N = M.x; return N } }"],
  ["import call", "let C = class { static function abs(x) { return abs(x) } }"],
  ["outer call", "local f = null\nf = function f(n) { return n == 0 ? 0 : f(n - 1) + 1 }"],
  ["outer typed let", "let M = 5\nlet C = class { function M() { let t: int = M; return t } }"],
  ["outer assign", "local f = null\nf = function f() { f = 5 }"],
  ["outer of an enclosing function", "function mk() { let upd = 1; return class { function upd() { return upd } } }"],
]

foreach (c in cases) {
  try {
    compilestring(c[1], c[0], { abs })
    println($"{c[0]}: compiled")
  } catch (e) println($"{c[0]}: {e}")
}

// With no other binding of that name, the own name is the function
let fact = function fact(n) { return n <= 1 ? 1 : n * fact(n - 1) }
println("recursion:", fact(5))
let C = class { static function rec(n) { return n == 0 ? 0 : rec(n - 1) + 1 } }
println("method recursion:", C.rec(3))
