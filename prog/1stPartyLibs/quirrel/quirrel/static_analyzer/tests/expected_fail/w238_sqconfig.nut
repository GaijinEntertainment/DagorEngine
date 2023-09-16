//expect:w238

::a <- require("sq3_sa_test").a
::table2 <- require("sq3_sa_test").table2

let function x() { //-declared-never-used
  ::a._must_be_utilized(::table2);
}
