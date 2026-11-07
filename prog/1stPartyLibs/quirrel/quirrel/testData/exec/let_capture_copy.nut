// A closure keeps a copy of a 'let' that has its final value, else a shared outer

// A default or the source of a destructuring sees the binding before the destructuring sets it
let {a = 1, b = @() a} = {a = 2}
println(b())
let {f} = {f = @() f}
println(f() == f)
let [x = 10, y = @() x] = [20]
println(y())

let {p} = {p = "p"}
let readP = @() p
println(readP())

// The middle function does not read the copy, it only passes it on.
// The inner function reads z, so closure hoisting cannot move it out of mk.
function nested() {
  let n = 1
  local m = 2
  let mk = @(z) @() n + m + z
  m = 3
  return mk(0)()
}
println(nested())

let fns = []
for (local i = 0; i < 3; i++) {
  let v = i * 10
  fns.append(@() v)
}
foreach (fn in fns)
  println(fn())

let late
late = "late"
let readLate = @() late
println(readLate())

let k = "kv"
let fv = (@() k).getfreevar(0)
println($"{fv.name} = {fv.value}")
