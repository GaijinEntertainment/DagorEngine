let keys = getroottable().keys().sort()
foreach(k in keys)
  println($"{k}")
println(getroottable().len())

require("squirrel.native_modules").each(println)