


function foo(_x) { return 20 }

let y = null
let x = {}


foo(x[y])
foo(x?[y])
foo(x?.z[y])
foo(x.y?[10].y?[y])
foo(x.y.z.u[y])
foo(x.y?.z.u[y])
