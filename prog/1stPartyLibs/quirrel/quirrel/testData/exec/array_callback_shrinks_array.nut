function show(a) {
  return ", ".join(a)
}

function mk() {
  return [1, 2, 3, 4, 5, 6]
}

let filtered = mk()
let filterRes = filtered.filter(function(_v, i) {
  if (i == 1)
    filtered.resize(3)
  return true
})
println($"filter: {show(filterRes)}")

let reduced = mk()
let reduceRes = reduced.reduce(function(acc, v, i) {
  if (i == 1)
    reduced.resize(3)
  return acc + v
})
println($"reduce: {reduceRes}")

let mapped = mk()
let mapRes = mapped.map(function(v, i) {
  if (i == 1)
    mapped.resize(3)
  return v * 10
})
println($"map: {show(mapRes)}")

// apply writes nothing past the new size, so only the call count shows the extra calls
let applied = mk()
local applyCalls = 0
applied.apply(function(v, i) {
  applyCalls++
  if (i == 1)
    applied.resize(3)
  return v * 10
})
println($"apply: {show(applied)} ({applyCalls} calls)")
