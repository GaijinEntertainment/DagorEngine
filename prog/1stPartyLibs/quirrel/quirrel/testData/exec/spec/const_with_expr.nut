// Unary

const o = 4
const a = -o
const b = !false
const c = ~a
const d = typeof(c)

println(a, b, c, d)

// Binary and ternary

const tbl = {["integer"] = 5}
const x = tbl[typeof o] + (o | 0xFF) + (o % 3) - (b ? 6 : o)

println(x)

