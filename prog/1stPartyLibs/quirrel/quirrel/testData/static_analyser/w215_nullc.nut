
function foo() {}

let o = {}
let s = foo()
let r = foo()

let _u = o?.u ?? s ? s / r : 0.0