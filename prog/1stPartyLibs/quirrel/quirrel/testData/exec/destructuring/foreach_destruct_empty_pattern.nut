// Empty `{}` and `[]` destructuring patterns: pattern matches without
// binding any field. Each iteration runs body once.
local n = 0
foreach ({} in [{a = 1}, {b = 2}, {}]) n++
println("e1:", n)

local m = 0
foreach ([] in [[1, 2, 3], [], [9]]) m++
println("e2:", m)

// Same with idx.
local sumIdx = 0
foreach (i, {} in [{a = 1}, {b = 2}]) sumIdx += i
println("e3:", sumIdx)

// Function-arg empty destructuring is now also accepted (shared
// parseDestructuringFields helper).
let f = function({}) { return "ok" }
println("e4:", f({a = 1}))

let g = function([]) { return "ok2" }
println("e5:", g([1, 2]))
