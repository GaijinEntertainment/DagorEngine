function show(v) {
  println("<" + v + ">")
  println(v + "|")
  println($"[{v}]")
  println("({x})".subst({ x = v }))
  println("".concat(v, ":", v))
  println(",".join([v, v]))
  print(v)
  print("\n")
  println(v, v)
}

foreach (v in [true, false, null])
  show(v)

println("" + true, "" + false, "" + null)
println($"{true} {false} {null}")

let texts = [true, false].map(@(v) v.tostring()).append("" + null)
println(texts[0], texts[1], texts[2])
println(texts[0] == "true", texts[1] == "false", texts[2] == "null")
println(texts[0].len(), texts[1].len(), texts[2].len())
println({ ["true"] = 1 }?[texts[0]], { ["null"] = 2 }?[texts[2]])

try
  println(null.tostring())
catch (e)
  println(e)
