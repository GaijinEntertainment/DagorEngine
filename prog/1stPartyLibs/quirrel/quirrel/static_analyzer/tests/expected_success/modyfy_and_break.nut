::x <- require("sq3_sa_test").x

foreach (i, _j in ::x.y) {
  ::x.y.append(234)
  print(i)
  break
}

foreach (i, _j in ::x.y) {
  delete ::x.y[2]
  print(i)
  return
}
