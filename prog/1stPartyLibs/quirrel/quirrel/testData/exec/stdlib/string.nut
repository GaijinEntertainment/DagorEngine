let { format, printf, split_by_chars, startswith, endswith, strip, lstrip, rstrip, escape } = require("string")

print("   strip ".strip())
println(";")
print("   lstrip ".lstrip())
println(";")
print("   rstrip ".rstrip())
println(";")
print("")

println("abcd".endswith("abcd*"))
println("abcd".endswith("cd"))
println("abcd".endswith("a"))
println("abcd".endswith(""))
println("")

println("abcd".startswith("*abcd"))
println("abcd".startswith("ab"))
println("abcd".startswith("d"))
println("abcd".startswith(""))
println("")

println(endswith("abcd", "d"))
println(startswith("abcd", "a"))
print(strip("   strip "))
println(";")
print(lstrip("   lstrip "))
println(";")
print(rstrip("   rstrip "))
println(";")

println("")

println("~\n\"\'\\~".escape())
println(escape("~\n\"\'\\~"))
println(format("str:%s; int:%d; hex:%x", "st", 255, 255))
printf("str:%s; int:%d; hex:%x\n", "st", 255, 255)

let arr = split_by_chars(":aaa,bb:c:::", ",:", true)
foreach(_, v in arr) {
  print(v)
  print(";")
}

println("")

let arr2 = ":aaa,bb:c:::".split_by_chars(",:", true)
foreach(_, v in arr2) {
  print(v)
  print(";")
}
