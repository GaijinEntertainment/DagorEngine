//expect:w238

::table2 <- require("sq3_sa_test").table2
::a <- require("sq3_sa_test").a


let function x() { //-declared-never-used
  ::a.__merge(::table2);
}
