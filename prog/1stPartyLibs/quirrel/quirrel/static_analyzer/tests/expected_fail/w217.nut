//expect:w217

::a <- require("sq3_sa_test").a
::h <- require("sq3_sa_test").h


let function foo(x){ //-declared-never-used
  while (x) {
    if (::a == x)
      ::h(::a, x)

    return
  }
}
