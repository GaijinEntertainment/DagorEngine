//expect:w262

::x <- require("sq3_sa_test").x
::y <- require("sq3_sa_test").y

if (::x)
  if (::y)
    print(1)
else
  print(2)
