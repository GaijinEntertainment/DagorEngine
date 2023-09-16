//expect:w221

::x <- require("sq3_sa_test").x

let function foo(y){ //-declared-never-used
  ::x == y
}