//expect:w238

::isLoggedIn <- require("sq3_sa_test").isLoggedIn


let function x() { //-declared-never-used
  ::isLoggedIn()
}
