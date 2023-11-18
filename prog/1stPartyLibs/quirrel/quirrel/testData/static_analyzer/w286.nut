//expect:w286

let function fn1() {}

return fn1 || ::fn2 // -undefined-global
