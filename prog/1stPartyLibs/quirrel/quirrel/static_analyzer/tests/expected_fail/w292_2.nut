//expect:w292

::x <- require("sq3_sa_test").x

foreach (i, _j in ::x.y) {
  ::x.y.append(i)
  print(i)
  if (i)
    break
}
