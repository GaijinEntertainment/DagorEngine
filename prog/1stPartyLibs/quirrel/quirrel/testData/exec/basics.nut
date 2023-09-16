//tests for local bindings, local variables and const
const A = 2
let a = 1
local b = 3
b = 10
let b_0 = b
b += a
b -= 1
b++
b--
b = b + A
b = b - A

b *= 2
b /= 2
b = b * 2
b = b / 2
b = (b + 1) * 2
b = b / 2 -1

assert(b == b_0)
