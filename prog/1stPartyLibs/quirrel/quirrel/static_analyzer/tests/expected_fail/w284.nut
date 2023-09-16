//expect:w284

::y <- require("sq3_sa_test").y

let function fn(x) {
  return ::y(x)
}

return fn(1) != null ? fn(1) : null
