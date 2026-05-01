let outer_factor = 10
function run() {
  let acc = []
  foreach ({n = (function(){ return outer_factor * 5 })()} in [{n = 1}, {}, {n = 3}]) {
    acc.append(n)
  }
  return acc
}
let r = run()
println(r[0], r[1], r[2])

function maker(scale) {
  let out = []
  foreach ({v = (@(s) s + 1)(scale)} in [{}, {v = 100}, {}]) {
    out.append(v)
  }
  return out
}
let m = maker(7)
println(m[0], m[1], m[2])
