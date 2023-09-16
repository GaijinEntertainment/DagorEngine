//expect:w218

::a <- require("sq3_sa_test").a
::h <- require("sq3_sa_test").h

let function foo(x) { //-declared-never-used
  do {
    if (::a == x)
      ::h(::a, x)
    continue;
    x--;
  } while (x)
}
