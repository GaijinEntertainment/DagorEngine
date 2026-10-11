//-file:declared-never-used

let before = 1
const function f() { return 2 }
let after = f()
global const function g(x) { if (x) return 3 }
let last = g(1)
