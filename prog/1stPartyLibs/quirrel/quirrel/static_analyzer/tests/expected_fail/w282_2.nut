//expect:w282

::fn <- require("sq3_sa_test").fn
::fn2 <- require("sq3_sa_test").fn2

local a = ::fn()
while (a := ::fn() || ::fn2())
  print(a)
