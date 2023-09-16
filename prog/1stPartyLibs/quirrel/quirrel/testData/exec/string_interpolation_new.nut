

function foo() { return "foo" }
function bar(p) { return $"[bar {p}]"  }

let s = $"x1 = {foo()}, x2 = {bar($"qux = {foo()}")}"

print("#1: ")
println(s)

print("#2: ")
println($"{$"{2}"}");

print("#3: ")
local gg = 123
println($"\{ gg = {gg} \}")

print("#4: ")
println($"   foo bar ")

print("#5: ")
println($"{foo()}}}}}}")

print("#6: ")
println($//"x")
"{foo()}")