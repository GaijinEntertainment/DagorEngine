function foo(_p) {}
let x = {}

let p = foo(1) ? x?.x : x.y

foo(p.z)



//-file:expr-cannot-be-null
