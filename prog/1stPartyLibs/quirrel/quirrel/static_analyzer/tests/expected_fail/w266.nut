//expect:w266

::x <- require("sq3_sa_test").x

{
  ::x++
} while (::x)
::x--
