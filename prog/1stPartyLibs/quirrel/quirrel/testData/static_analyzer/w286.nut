//expect:w286

function fn1() {}

return fn1 || ::fn2 // -undefined-global
