//expect:w286

::fn2 <- require("sq3_sa_test").fn2

local fn1 = @() true

return fn1 || ::fn2 //-const-in-bool-expr
