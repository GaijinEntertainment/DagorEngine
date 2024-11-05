function foreach_show(a)
{
  foreach (val in a)
      println ($"value={val}")
  foreach (idx, val in a)
      println($"index={idx} value={val}")
}

foreach_show([10,23,33,41,589,56])
//tables
foreach_show({x = 10, y = 11.5, z = "str", w = true})

//generators
function range(n) {
  for (local i=0; i<n; ++i)
    yield i
}
foreach_show(range(3))

foreach (p in range(3)) {
  println(p)
}

foreach (p in range(0)) {
  println(p)
}
