//expect:w283

::y <- require("sq3_sa_test").y

let function fn(x) { //-declared-never-used
  return ::y.cc ?? x ?? null
}
