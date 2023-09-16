//expect:w292

::x <- require("sq3_sa_test").x

foreach (i, _j in ::x.y) {
  delete ::x.y[i]
  print(i)
  if (i)
    return
}
