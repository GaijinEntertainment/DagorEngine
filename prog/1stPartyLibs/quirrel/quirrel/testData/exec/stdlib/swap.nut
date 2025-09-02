class D {
  x = 1
  y = 2
}
local d = D()
local t = {"1": "123", "2": "asd"}
local a = [123, 456, 789]

d.swap("x", "y")
a.swap(1, 2)
t.swap("1", "2")

println($"{d["x"]} {d["y"]}")
println($"{t["1"]} {t["2"]}")
println($"{a[1]} {a[2]}")
