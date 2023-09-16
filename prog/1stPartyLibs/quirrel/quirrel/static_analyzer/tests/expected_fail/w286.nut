//expect:w286

::fn2 <- require("sq3_sa_test").fn2


let function fn1() {}

return fn1 || ::fn2
