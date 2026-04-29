function foo() {}

let x = foo()
local c = foo()
let a = foo()
let t = "skdhaljs"
local d = 20

while (c && (c?.o ?? "") != "") {  // c is non-null inside loop body
  c = a?[x.value?[t][c.o]]  // c.o safe because loop condition ensures c non-null
  if ((c?.d ?? 0) > d) {
    d = c.d  // No warning - c?.d > d means c is non-null (not using default 0)
  } else {
    d = d.xyz
  }
}
