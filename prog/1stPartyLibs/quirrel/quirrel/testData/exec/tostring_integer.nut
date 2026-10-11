function show(v) {
  println("<" + v + ">", v + "|", v.tostring(), $"[{v}]", "".concat(v, ":", v), ",".join([v, v]))
  print(v)
  print("\n")
}

let minInt = 0x8000000000000000
let maxInt = 0x7FFFFFFFFFFFFFFF
let edges = [0, 1, -1, 9, -9, 10, -10, 0x7FFFFFFF, 0x80000000, -0x7FFFFFFF, -0x80000000, 0xFFFFFFFF, 0x100000000,
  minInt, minInt + 1, maxInt, maxInt - 1]
foreach (v in edges)
  show(v)

local p = 1
for (local i = 0; i < 18; i++) {
  p *= 10
  foreach (v in [p - 1, p, p + 1, -p + 1, -p, -p - 1])
    println(v)
}

local seed = 88172645463325252
local mismatches = 0
local totalLen = 0
for (local i = 0; i < 20000; i++) {
  seed = seed ^ (seed << 13)
  seed = seed ^ (seed >>> 7)
  seed = seed ^ (seed << 17)
  let v = seed >> (i % 64)
  let s = v.tostring()
  if (s.tointeger() != v)
    mismatches++
  totalLen += s.len()
  if (i % 2000 == 0)
    println(s)
}
println(mismatches, totalLen)
