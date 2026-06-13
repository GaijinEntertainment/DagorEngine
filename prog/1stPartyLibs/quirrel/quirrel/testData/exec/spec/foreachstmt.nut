function foreach_show(a)
{
  foreach (val in a)
      println ($"value={val}")

  println("---")

  foreach (idx, val in a)
      println($"index={idx} value={val}")

  println("---")
}

function foreach_show_sorted(a)
{
  local result = []
  foreach (val in a)
      result.append($"value={val}")

  foreach (line in result.sort())
      println(line)

  println("---")

  result = []
  foreach (idx, val in a)
      result.append($"index={idx} value={val}")

  foreach (line in result.sort())
      println(line)

  println("---")
}

foreach_show([10,23,33,41,589,56])
//tables
foreach_show_sorted({x = 10, y = 11.5, z = "str", w = true})

//generators
function range(n) {
  for (local i=0; i<n; ++i)
    yield i
}
foreach_show(range(3))

foreach (p in range(3)) {
  println(p)
}
println("---")

foreach (p, v in range(3)) {
  println(p, v)
}

println("---")

foreach (p in range(0)) {
  println(p)
}

println("---")
